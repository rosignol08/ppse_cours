#!/bin/bash

# Paramètres de quantification optimaux (Task 5)
F=1
S=5

echo "====================================================="
echo "  Lancement des validations SIMD NEON (Task 6)       "
echo "====================================================="

# ---------------------------------------------------------
# SÉRIE 1 : Décodeurs Hard (jusqu'à SNR = 14)
# ---------------------------------------------------------
echo -e "\n---> 1/5 : rep-hard8 (N=8192)"
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 8192 -D "rep-hard8" --qf $F --qs $S -o results_hard8_8192.csv

echo -e "\n---> 2/5 : rep-hard8-neon (N=8192)"
./simulator -m 0 -M 14 -s 1 -e 100 -K 32 -N 8192 -D "rep-hard8-neon" --qf $F --qs $S -o results_hard8_neon_8192.csv


# ---------------------------------------------------------
# SÉRIE 2 : Décodeurs Soft (jusqu'à SNR = 11) avec --mod-all-ones
# ---------------------------------------------------------
echo -e "\n---> 3/5 : rep-soft (Flottant de référence)"
./simulator -m 0 -M 11 -s 1 -e 100 --mod-all-ones -K 32 -N 8192 -D "rep-soft" -o results_soft_ref_8192.csv

echo -e "\n---> 4/5 : rep-soft8 (Quantifié)"
./simulator -m 0 -M 11 -s 1 -e 100 --mod-all-ones -K 32 -N 8192 -D "rep-soft8" --qf $F --qs $S -o results_soft8_8192.csv

echo -e "\n---> 5/5 : rep-soft8-neon (Quantifié + SIMD)"
./simulator -m 0 -M 11 -s 1 -e 100 --mod-all-ones -K 32 -N 8192 -D "rep-soft8-neon" --qf $F --qs $S -o results_soft8_neon_8192.csv

echo "====================================================="
echo "  Toutes les simulations sont terminées !            "
echo "  Vérifie les fichiers .csv générés.                 "
echo "====================================================="