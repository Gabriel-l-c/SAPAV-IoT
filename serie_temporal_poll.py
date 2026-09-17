#!/usr/bin/env python3
import argparse
import csv
import time
import requests

ORION_URL = 'http://192.168.0.95:31330/ngsi-ld/v1/entities/urn:ngsi-ld:Device:LD2420'
HEADERS = {
    'NGSILD-Tenant': 'openiot',
    'Accept': 'application/ld+json',
}
LOG_PATH = 'serie_temporal.csv'

def consultar_entidade():
    try:
        resp = requests.get(ORION_URL, headers=HEADERS, timeout=3)
        if resp.status_code == 200:
            dados = resp.json()
            presence = dados.get('presence', {}).get('value')
            distance = dados.get('distance', {}).get('value')
            return presence, distance, resp.status_code
        return None, None, resp.status_code
    except requests.exceptions.RequestException as e:
        return None, None, f"ERRO:{e}"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--duracao', type=float, default=20, help='duracao em minutos')
    parser.add_argument('--intervalo', type=float, default=1.5, help='intervalo em segundos')
    args = parser.parse_args()

    fim = time.time() + args.duracao * 60
    t0 = time.time()

    print(f"Iniciando polling por {args.duracao} min, a cada {args.intervalo} s...")
    print("Comece a caminhar no corredor agora. Ctrl+C para encerrar antes do tempo.\n")

    with open(LOG_PATH, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['iso_time', 't_rel_s', 'presence', 'distance', 'http_status'])

        while time.time() < fim:
            try:
                presence, distance, status = consultar_entidade()
                t_rel = time.time() - t0
                writer.writerow([
                    time.strftime('%Y-%m-%dT%H:%M:%S'), f"{t_rel:.1f}",
                    presence, distance, status
                ])
                f.flush()
                print(f"t={t_rel:6.1f}s  presence={presence}  distance={distance}  status={status}")
                time.sleep(args.intervalo)
            except KeyboardInterrupt:
                print("\nEncerrado pelo usuario antes do tempo total.")
                break

    print(f"\nConcluido. Dados salvos em {LOG_PATH}")

if __name__ == "__main__":
    main()
