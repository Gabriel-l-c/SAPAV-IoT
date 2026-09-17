#!/usr/bin/env python3
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

t_rel, presence, distance = [], [], []

with open('serie_temporal.csv', newline='') as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            t_rel.append(float(row['t_rel_s']))
        except ValueError:
            continue
        p = row['presence']
        presence.append(1 if p == 'True' else 0)
        try:
            distance.append(float(row['distance']))
        except (TypeError, ValueError):
            distance.append(None)

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

ax1.step(t_rel, presence, where='post', color='tab:blue')
ax1.set_ylabel('Presença (0/1)')
ax1.set_yticks([0, 1])
ax1.set_title('Série temporal de presença e distância — corredor')
ax1.grid(True, alpha=0.3)

dist_validos_t = [t for t, d in zip(t_rel, distance) if d is not None]
dist_validos_d = [d for d in distance if d is not None]
ax2.plot(dist_validos_t, dist_validos_d, color='tab:orange', marker='.', linestyle='-')
ax2.set_ylabel('Distância (cm)')
ax2.set_xlabel('Tempo (s)')
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('serie_temporal.png', dpi=150)
print("Gráfico salvo em serie_temporal.png")

