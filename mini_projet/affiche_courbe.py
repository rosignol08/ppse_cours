import csv
import math
import os

import matplotlib.pyplot as plt

# Fonction pour extraire les données utiles d'un CSV
def lire_csv(nom_fichier):
    ebn0_list, ber_list, fer_list, thr_list, sim_time_list = [], [], [], [], []
    try:
        with open(nom_fichier, 'r', newline='') as fichier_csv:
            lecteur = csv.DictReader(fichier_csv)

            for ligne in lecteur:
                try:
                    ebn0 = float(ligne['Eb/N0'])
                    ber = float(ligne['BER']) if float(ligne['BER']) > 0 else float('nan')
                    fer = float(ligne['FER']) if float(ligne['FER']) > 0 else float('nan')
                    thr = float(ligne['Sim_thr_Mbps']) if float(ligne['Sim_thr_Mbps']) > 0 else float('nan')
                    sim_time = float(ligne['sim_time_s']) if float(ligne['sim_time_s']) > 0 else float('nan')

                    ebn0_list.append(ebn0)
                    ber_list.append(ber)
                    fer_list.append(fer)
                    thr_list.append(thr)
                    sim_time_list.append(sim_time)
                except (KeyError, ValueError):
                    continue
    except FileNotFoundError:
        print(f"⚠️ Le fichier '{nom_fichier}' n'a pas été trouvé.")
        
    return ebn0_list, ber_list, fer_list, thr_list, sim_time_list

# 1. Chargement des données
fichiers = [
    ("base_rep-soft.csv", "Base soft", "red", "-"),
    ("travail_rep-soft.csv", "Task2 soft", "darkred", "--"),
    ("base_rep-soft8_qs5_qf1.csv", "Base soft8", "green", "-"),
    ("travail_rep-soft8_qs5_qf1.csv", "Task2 soft8", "darkgreen", "--"),
    ("base_rep-soft8-neon_qs5_qf1.csv", "Base soft8 neon", "blue", "-"),
    ("travail_rep-soft8-neon_qs5_qf1.csv", "Task2 soft8 neon", "navy", "--"),
]

donnees = {}
for nom_fichier, etiquette, couleur, style in fichiers:
    if not os.path.exists(nom_fichier):
        print(f"⚠️ Le fichier '{nom_fichier}' n'a pas été trouvé, on l'ignore.")
        continue

    donnees[etiquette] = {
        "ebn0": [],
        "ber": [],
        "fer": [],
        "thr": [],
        "sim_time": [],
        "color": couleur,
        "style": style,
    }
    ebn0, ber, fer, thr, sim_time = lire_csv(nom_fichier)
    donnees[etiquette]["ebn0"] = ebn0
    donnees[etiquette]["ber"] = ber
    donnees[etiquette]["fer"] = fer
    donnees[etiquette]["thr"] = thr
    donnees[etiquette]["sim_time"] = sim_time

print("\nRésumé des temps de simulation:")
for etiquette, data in donnees.items():
    valid_times = [value for value in data["sim_time"] if not math.isnan(value)]
    total_time = sum(valid_times)
    average_time = total_time / len(valid_times) if valid_times else float('nan')
    print(f"- {etiquette}: total = {total_time:.3f} ms, moyen par SNR = {average_time:.3f} ms")

# 2. Création du graphique
fig, axes = plt.subplots(4, 1, figsize=(12, 16), sharex=True)

ax_ber, ax_fer, ax_thr, ax_time = axes

for etiquette, data in donnees.items():
    ax_ber.plot(data["ebn0"], data["ber"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)
    ax_fer.plot(data["ebn0"], data["fer"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)
    ax_thr.plot(data["ebn0"], data["thr"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)
    ax_time.plot(data["ebn0"], data["sim_time"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)

ax_ber.set_yscale('log')
ax_fer.set_yscale('log')
ax_time.set_yscale('log')
ax_ber.set_ylabel('BER')
ax_fer.set_ylabel('FER')
ax_thr.set_ylabel('Sim_thr_Mbps')
ax_time.set_ylabel('sim_time_s')
ax_thr.set_xlabel('Eb/N0 (dB)')
ax_ber.set_title('Comparaison base vs task2 : précision, débit et temps de simulation')
ax_ber.grid(True, which='both', ls='-', alpha=0.3)
ax_fer.grid(True, which='both', ls='-', alpha=0.3)
ax_thr.grid(True, which='both', ls='-', alpha=0.3)
ax_time.grid(True, which='both', ls='-', alpha=0.3)
ax_ber.legend(fontsize=8)
ax_fer.legend(fontsize=8)
ax_thr.legend(fontsize=8)
ax_time.legend(fontsize=8)

# 3. Sauvegarde et affichage
fig.tight_layout()
nom_image = 'comparaison_base_task2.png'
plt.savefig(nom_image)
print(f"Graphique généré avec succès : {nom_image}")
plt.show()