import matplotlib.pyplot as plt

# Liste des fichiers à lire avec leurs labels et couleurs
simulations = [
    {"fichier": "classique_sim1_hard_128.csv", "label": "Hard N=128", "couleur": "red"},
    {"fichier": "classique_sim2_soft_128.csv", "label": "Soft N=128", "couleur": "blue"},
    {"fichier": "classique_sim3_soft_96.csv",  "label": "Soft N=96",  "couleur": "green"},
    {"fichier": "classique_sim4_soft_64.csv",  "label": "Soft N=64",  "couleur": "orange"},
    {"fichier": "classique_sim5_soft_32.csv",  "label": "Soft N=32",  "couleur": "purple"}
]

plt.figure(figsize=(12, 8))

for sim in simulations:
    snr_list = []
    ber_list = []
    fer_list = []
    
    try:
        with open(sim["fichier"], 'r') as f:
            next(f) # On ignore l'en-tête
            
            for line in f:
                if not line.strip():
                    continue
                parts = line.split(',')
                
                if len(parts) >= 10:
                    try:
                        snr = float(parts[0])
                        ber = float(parts[6])
                        fer = float(parts[7])
                        
                        # On ignore les valeurs à 0 pour éviter les bugs avec l'échelle logarithmique
                        if ber > 0 and fer > 0:
                            snr_list.append(snr)
                            ber_list.append(ber)
                            fer_list.append(fer)
                    except ValueError:
                        continue
                        
        # Tracé du BER (Ligne continue + ronds)
        plt.plot(snr_list, ber_list, marker='o', linestyle='-', color=sim["couleur"], label=f'BER - {sim["label"]}')
        
        # Tracé du FER (Ligne pointillée + carrés, avec un peu de transparence 'alpha' pour la lisibilité)
        plt.plot(snr_list, fer_list, marker='s', linestyle='--', color=sim["couleur"], alpha=0.5)

    except FileNotFoundError:
        print(f"Attention : Le fichier {sim['fichier']} n'a pas été trouvé.")

# Paramétrage de la grille et des axes
plt.yscale('log')
plt.xlabel('SNR (dB)')
plt.ylabel('Taux d\'erreur (Échelle Log)')
plt.title('Comparaison Codec Répétition : Décodage Hard vs Soft et tailles de N')
plt.grid(True, which="both", ls="--", alpha=0.4)
plt.legend()

# Sauvegarde et affichage
plt.savefig('comparaison_globale.png')
print("Graphique généré et sauvegardé sous 'comparaison_globale.png'")
plt.show()