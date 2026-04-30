import matplotlib.pyplot as plt

# Fonction pour extraire les données d'un CSV
def lire_csv(nom_fichier):
    snr_list, ber_list, fer_list = [], [], []
    try:
        with open(nom_fichier, 'r') as f:
            next(f) # Ignorer l'en-tête
            for line in f:
                if not line.strip():
                    continue
                parts = line.split(',')
                if len(parts) >= 8:
                    try:
                        snr = float(parts[0])
                        # On récupère BER (index 6) et FER (index 7)
                        ber = float(parts[6]) if float(parts[6]) > 0 else float('nan')
                        fer = float(parts[7]) if float(parts[7]) > 0 else float('nan')
                        
                        snr_list.append(snr)
                        ber_list.append(ber)
                        fer_list.append(fer)
                    except ValueError:
                        continue
    except FileNotFoundError:
        print(f"⚠️ Le fichier '{nom_fichier}' n'a pas été trouvé.")
        
    return snr_list, ber_list, fer_list

# 1. Chargement des données
snr_ref, ber_ref, fer_ref = lire_csv("results_reference.csv")
snr_q8, ber_q8, fer_q8 = lire_csv("results_8.csv")

# 2. Création du graphique
plt.figure(figsize=(10, 6))

# --- Tracé de la Référence (Float 32) ---
# Lignes pleines et marqueurs ronds
plt.plot(snr_ref, ber_ref, marker='o', linestyle='-', color='blue', label='Ref (Float) - BER')
plt.plot(snr_ref, fer_ref, marker='o', linestyle='-', color='red', label='Ref (Float) - FER')

# --- Tracé du Quantifié (s=5, f=1) ---
# Lignes pointillées et marqueurs croix 'x' pour bien voir si ça se superpose
plt.plot(snr_q8, ber_q8, marker='x', linestyle='--', color='cyan', label='Quantifié (s=5, f=1) - BER')
plt.plot(snr_q8, fer_q8, marker='+', linestyle='--', color='orange', label='Quantifié (s=5, f=1) - FER')

# 3. Décoration
plt.yscale('log') # Indispensable pour voir la cascade
plt.xlabel('Eb/N0 (dB)')
plt.ylabel('Taux d\'erreur (log scale)')
plt.title('Validation de la Quantification (Float vs Fixed-Point s=5 f=1)')
plt.grid(True, which="both", ls="-", alpha=0.3)
plt.legend()

# 4. Sauvegarde et affichage
nom_image = 'comparaison_quantification.png'
plt.savefig(nom_image)
print(f"Graphique généré avec succès : {nom_image}")
plt.show()