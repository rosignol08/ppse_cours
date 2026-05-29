#!/bin/bash

# 1. Nettoyage des anciens fichiers CSV
echo "Suppression des anciens fichiers CSV..."
rm -f base_rep-soft.csv 
rm -f base_rep-soft8_qs5_qf1.csv 
rm -f base_rep-soft8-neon_qs5_qf1.csv
rm -f travail_rep-soft.csv 
rm -f travail_rep-soft8_qs5_qf1.csv 
rm -f travail_rep-soft8-neon_qs5_qf1.csv

# 2. Lancement des simulations de base
echo "Lancement des simulations de Base..."
./programme -m 0 -M 8 -s 2 -e 50 --mod-all-ones -K 32 -N 8192 -D "rep-soft" -o base_rep-soft.csv
./programme -m 0 -M 8 -s 2 -e 50 --mod-all-ones -K 32 -N 8192 -D "rep-soft8" --qf 1 --qs 5 -o base_rep-soft8_qs5_qf1.csv
./programme -m 0 -M 8 -s 2 -e 50 --mod-all-ones -K 32 -N 8192 -D "rep-soft8-neon" --qf 1 --qs 5 -o base_rep-soft8-neon_qs5_qf1.csv

# 3. Lancement des simulations optimisées (Task 2)
echo "Lancement des simulations Task 2 (Bit-packing)..."
./programme_travail -m 0 -M 8 -s 2 -e 50 --mod-all-ones -K 32 -N 8192 -D "rep-soft" -o travail_rep-soft.csv
./programme_travail -m 0 -M 8 -s 2 -e 50 --mod-all-ones -K 32 -N 8192 -D "rep-soft8" --qf 1 --qs 5 -o travail_rep-soft8_qs5_qf1.csv
./programme_travail -m 0 -M 8 -s 2 -e 50 --mod-all-ones -K 32 -N 8192 -D "rep-soft8-neon" --qf 1 --qs 5 -o travail_rep-soft8-neon_qs5_qf1.csv

# 4. Affichage des résultats
echo "Génération des courbes avec Python..."
python3 affiche_courbe.py

echo "Script terminé avec succès !"