#!/usr/bin/env python3
"""
analisar_latencia_periodica.py
---------------------------------
Le log_periodico.csv (gerado pelo latencia_gateway_periodico.py) e calcula:

  - perda de pacote: numeros de sequencia ("seq") que ficaram faltando
    entre o menor e o maior seq observado — deteccao exata, sem
    necessidade de casar timestamps aproximados;
  - jitter de chegada: estatisticas do intervalo entre pacotes
    consecutivos recebidos no RPi (deveria ficar perto de 1000 ms;
    desvios revelam atraso introduzido pela serial/USB/SO);
  - latencia de rede/broker: estatisticas do rtt_broker_ms (POST ->
    resposta do Orion-LD), que e a mesma metrica precisa do roteiro
    anterior, agora sem nenhuma dependencia de acionamento humano.

Uso:
    python3 analisar_latencia_periodica.py
"""

import csv
import statistics as st

INTERVALO_ESPERADO_MS = 1000


def main():
    with open('log_periodico.csv', newline='') as f:
        linhas = list(csv.DictReader(f))

    if not linhas:
        print("log_periodico.csv esta vazio.")
        return

    seqs = sorted(int(l['seq']) for l in linhas)
    seq_min, seq_max = seqs[0], seqs[-1]
    esperados = set(range(seq_min, seq_max + 1))
    recebidos = set(seqs)
    faltando = sorted(esperados - recebidos)

    total_esperado = len(esperados)
    total_recebido = len(recebidos)
    taxa_perda = 100 * len(faltando) / total_esperado if total_esperado else 0

    intervalos = [float(l['intervalo_ms']) for l in linhas if l['intervalo_ms'] not in ('', None)]
    rtts = [float(l['rtt_broker_ms']) for l in linhas]

    print("=" * 62)
    print("PERDA DE PACOTE (por numero de sequencia)")
    print("-" * 62)
    print(f"Sequencias esperadas (seq_min..seq_max): {seq_min}..{seq_max}  (n={total_esperado})")
    print(f"Sequencias recebidas:                    {total_recebido}")
    print(f"Sequencias perdidas:                      {len(faltando)}  ({taxa_perda:.1f}%)")
    if faltando:
        print(f"  seq perdidas: {faltando}")
    print("=" * 62)
    print("JITTER DE CHEGADA (intervalo entre pacotes consecutivos no RPi)")
    print(f"  esperado = {INTERVALO_ESPERADO_MS} ms")
    if intervalos:
        print(f"  media    = {st.mean(intervalos):.1f} ms")
        print(f"  mediana  = {st.median(intervalos):.1f} ms")
        print(f"  min/max  = {min(intervalos):.1f} / {max(intervalos):.1f} ms")
        if len(intervalos) > 1:
            print(f"  desvio padrao (jitter) = {st.stdev(intervalos):.1f} ms")
    print("=" * 62)
    print("LATENCIA DE REDE/BROKER (POST -> resposta Orion-LD)")
    if rtts:
        print(f"  media    = {st.mean(rtts):.1f} ms")
        print(f"  mediana  = {st.median(rtts):.1f} ms")
        print(f"  min/max  = {min(rtts):.1f} / {max(rtts):.1f} ms")
        if len(rtts) > 1:
            print(f"  desvio padrao = {st.stdev(rtts):.1f} ms")
    print("=" * 62)


if __name__ == "__main__":
    main()
