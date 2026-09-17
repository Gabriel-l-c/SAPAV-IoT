#!/usr/bin/env python3
"""
latencia_gateway_periodico.py
--------------------------------
Versao do gateway para o teste de latencia SEM acionamento manual.

Le os pacotes enviados a 1 Hz pelo latencia_firmware_teste.ino (cada um
com um numero de sequencia "seq" e o timestamp interno do ESP8266
"t_esp_ms"), encaminha cada um ao Orion-LD como no gateway normal, e
registra em log_periodico.csv:

  - seq             : numero de sequencia do pacote (vem do ESP8266)
  - t_esp_ms        : millis() do ESP8266 no instante do envio (clock
                       proprio da placa, nao sincronizado com o RPi —
                       serve so para conferir que o ESP esta mandando a
                       cada 1000 ms, nao para comparar com o relogio do RPi)
  - t_recv_ms       : timestamp do RPi (time.time()) quando a linha chegou
                       pela serial
  - intervalo_ms    : t_recv_ms atual - t_recv_ms do pacote anterior
                       (deveria ficar perto de 1000 ms; desvios revelam
                       atraso/jitter introduzido pela serial/USB/SO)
  - t_before_post   : timestamp (ms) antes do POST ao Orion-LD
  - t_after_post    : timestamp (ms) depois da resposta do Orion-LD
  - rtt_broker_ms   : t_after_post - t_before_post (latencia de rede+broker)
  - http_status     : codigo HTTP retornado

A deteccao de PACOTE PERDIDO e feita depois, no
analisar_latencia_periodica.py, simplesmente checando se algum "seq"
ficou faltando na sequencia — nao ha necessidade de casar timestamps
aproximados nem de qualquer acionamento humano.

Uso:
    python3 latencia_gateway_periodico.py --duracao 5
    (roda por 5 minutos e encerra sozinho; Ctrl+C tambem funciona)
"""

import argparse
import serial
import json
import time
import csv
import os
import requests

PORTA_USB = '/dev/ttyUSB0'
BAUD_RATE = 115200
ORION_URL = 'http://192.168.0.95:31330/ngsi-ld/v1/entityOperations/upsert?options=update'
LOG_PATH = 'log_periodico.csv'

HEADERS = {
    'Content-Type': 'application/ld+json',
    'NGSILD-Tenant': 'openiot',
}


def montar_payload(presence, distance):
    return [{
        "id": "urn:ngsi-ld:Device:LD2420",
        "type": "Device",
        "presence": {"type": "Property", "value": presence},
        "distance": {"type": "Property", "value": distance, "unitCode": "CMT"},
        "@context": ["https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld"],
    }]


def enviar_para_fiware(presence, distance):
    payload = montar_payload(presence, distance)
    t_before = time.time() * 1000
    try:
        resp = requests.post(ORION_URL, json=payload, headers=HEADERS, timeout=5)
        status = resp.status_code
    except requests.exceptions.RequestException as e:
        status = f"ERRO:{e}"
    t_after = time.time() * 1000
    return t_before, t_after, status


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--duracao', type=float, default=5, help='duracao do teste em minutos')
    args = parser.parse_args()

    print(f"Abrindo porta serial {PORTA_USB} @ {BAUD_RATE} bps...")
    ser = serial.Serial(PORTA_USB, BAUD_RATE, timeout=1)
    time.sleep(2)
    print(f"Serial conectada. Rodando por {args.duracao} min. Nenhuma acao manual necessaria.\n")

    fim = time.time() + args.duracao * 60
    t_recv_anterior = None

    with open(LOG_PATH, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'seq', 't_esp_ms', 't_recv_ms', 'intervalo_ms',
            't_before_post_ms', 't_after_post_ms', 'rtt_broker_ms', 'http_status'
        ])

        while time.time() < fim:
            try:
                if ser.in_waiting > 0:
                    linha = ser.readline().decode('utf-8', errors='replace').strip()
                    if not linha:
                        continue
                    t_recv = time.time() * 1000
                    try:
                        dados = json.loads(linha)
                    except json.JSONDecodeError:
                        print(f"[SERIAL RAW]: {linha}")
                        continue

                    if "seq" not in dados:
                        print(f"[LOG ESP8266]: {linha}")
                        continue

                    seq = dados["seq"]
                    t_esp = dados["t_esp_ms"]
                    presence = dados["presence"]
                    distance = dados["distance"]

                    intervalo = (t_recv - t_recv_anterior) if t_recv_anterior is not None else ''
                    t_recv_anterior = t_recv

                    t_before, t_after, status = enviar_para_fiware(presence, distance)
                    rtt = t_after - t_before

                    writer.writerow([
                        seq, t_esp, f"{t_recv:.1f}",
                        f"{intervalo:.1f}" if intervalo != '' else '',
                        f"{t_before:.1f}", f"{t_after:.1f}", f"{rtt:.1f}", status
                    ])
                    f.flush()
                    print(f"seq={seq}  intervalo={intervalo if intervalo=='' else f'{intervalo:.0f}ms'}  "
                          f"rtt_broker={rtt:.1f}ms  status={status}")

            except serial.SerialException:
                print("[ERRO HARDWARE] Cabo USB desconectado!")
                break
            except KeyboardInterrupt:
                print("\nEncerrado pelo usuario.")
                break

    print(f"\nConcluido. Dados salvos em {LOG_PATH}")


if __name__ == "__main__":
    main()
