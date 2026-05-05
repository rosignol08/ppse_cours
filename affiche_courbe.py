import csv

import matplotlib.pyplot as plt

# Fonction pour extraire les données utiles d'un CSV
def lire_csv(nom_fichier):
    ebn0_list, ber_list, fer_list, thr_list = [], [], [], []
    try:
        with open(nom_fichier, 'r', newline='') as fichier_csv:
            lecteur = csv.DictReader(fichier_csv)

            for ligne in lecteur:
                try:
                    ebn0 = float(ligne['Eb/N0'])
                    ber = float(ligne['BER']) if float(ligne['BER']) > 0 else float('nan')
                    fer = float(ligne['FER']) if float(ligne['FER']) > 0 else float('nan')
                    thr = float(ligne['Sim_thr_Mbps']) if float(ligne['Sim_thr_Mbps']) > 0 else float('nan')

                    ebn0_list.append(ebn0)
                    ber_list.append(ber)
                    fer_list.append(fer)
                    thr_list.append(thr)
                except (KeyError, ValueError):
                    continue
    except FileNotFoundError:
        print(f"⚠️ Le fichier '{nom_fichier}' n'a pas été trouvé.")
        
    return ebn0_list, ber_list, fer_list, thr_list

# 1. Chargement des données
fichiers = [
    ("results_hard_ref.csv", "Hard ref", "blue", "-"),
    ("results_hard8_neon.csv", "Hard 8 neon", "cyan", "--"),
    ("results_hard8.csv", "Hard 8", "navy", ":"),
    ("results_soft_ref.csv", "Soft ref", "red", "-"),
    ("results_soft8_neon.csv", "Soft 8 neon", "orange", "--"),
    ("results_soft8.csv", "Soft 8", "green", ":"),
]

donnees = {}
for nom_fichier, etiquette, couleur, style in fichiers:
    donnees[etiquette] = {
        "ebn0": [],
        "ber": [],
        "fer": [],
        "thr": [],
        "color": couleur,
        "style": style,
    }
    ebn0, ber, fer, thr = lire_csv(nom_fichier)
    donnees[etiquette]["ebn0"] = ebn0
    donnees[etiquette]["ber"] = ber
    donnees[etiquette]["fer"] = fer
    donnees[etiquette]["thr"] = thr

# 2. Création du graphique
fig, axes = plt.subplots(3, 1, figsize=(12, 12), sharex=True)

ax_ber, ax_fer, ax_thr = axes

for etiquette, data in donnees.items():
    ax_ber.plot(data["ebn0"], data["ber"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)
    ax_fer.plot(data["ebn0"], data["fer"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)
    ax_thr.plot(data["ebn0"], data["thr"], marker='o', linestyle=data["style"], color=data["color"], label=etiquette)

ax_ber.set_yscale('log')
ax_fer.set_yscale('log')
ax_ber.set_ylabel('BER')
ax_fer.set_ylabel('FER')
ax_thr.set_ylabel('Sim_thr_Mbps')
ax_thr.set_xlabel('Eb/N0 (dB)')
ax_ber.set_title('Comparaison des résultats pour les fichiers hard et soft')
ax_ber.grid(True, which='both', ls='-', alpha=0.3)
ax_fer.grid(True, which='both', ls='-', alpha=0.3)
ax_thr.grid(True, which='both', ls='-', alpha=0.3)
ax_ber.legend(fontsize=8)
ax_fer.legend(fontsize=8)
ax_thr.legend(fontsize=8)

# 3. Sauvegarde et affichage
fig.tight_layout()
nom_image = 'comparaison_hard_soft.png'
plt.savefig(nom_image)
print(f"Graphique généré avec succès : {nom_image}")
plt.show()