import matplotlib.pyplot as plt

snr_list = []
ber_list = []
fer_list = []

# On lit le fichier généré par le programme C++
try:
    with open('results.csv', 'r') as f:
        # 1. On ignore la première ligne car c'est l'en-tête (Eb/N0,Es/N0,...)
        next(f) 
        
        for line in f:
            # On ignore les lignes vides
            if not line.strip():
                continue
                
            # 2. On coupe la ligne à chaque virgule
            parts = line.split(',')
            
            # 3. On vérifie qu'on a bien nos 10 colonnes
            if len(parts) == 10:
                try:
                    # On récupère directement les colonnes qui nous intéressent
                    # Index 0 = Eb/N0 (SNR), Index 6 = BER, Index 7 = FER
                    snr = float(parts[0])
                    ber = float(parts[6])
                    fer = float(parts[7])
                    
                    # Petite sécurité : log(0) fait planter le graphique, on ignore si c'est 0
                    if ber > 0 and fer > 0:
                        snr_list.append(snr)
                        ber_list.append(ber)
                        fer_list.append(fer)
                except ValueError:
                    continue
except FileNotFoundError:
    print("Le fichier 'results.csv' n'a pas été trouvé. Assurez-vous d'avoir lancé le programme C++ d'abord.")
    exit(1)

# Création du graphique
plt.figure(figsize=(10, 6))

# On trace le BER et le FER en fonction du SNR
# marker='o' ajoute des points sur la courbe du BER
plt.plot(snr_list, ber_list, marker='o', linestyle='-', color='b', label='BER (Bit Error Rate)')
# marker='s' (square) ajoute des carrés sur la courbe du FER
plt.plot(snr_list, fer_list, marker='s', linestyle='--', color='r', label='FER (Frame Error Rate)')

# On met l'axe Y en échelle logarithmique (très standard pour les courbes BER/FER : 10^-1, 10^-2, etc.)
plt.yscale('log')

# Décoration et affichage
plt.xlabel('SNR (dB)')
plt.ylabel('Taux d\'erreur (log scale)')
plt.title('Performances du code de répétition (BER & FER vs SNR)')
plt.grid(True, which="both", ls="-", alpha=0.3)
plt.legend()

# On sauvegarde l'image 
plt.savefig('courbe_erreurs.png')
print("Graphique sauvegardé sous 'courbe_erreurs.png'")

# On l'affiche à l'écran
plt.show()
