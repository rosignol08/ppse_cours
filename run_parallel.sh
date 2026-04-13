#!/bin/bash

echo "=== Lancement de la double suite de simulations ==="

# ---------------------------------------------------------
# BLOC 1 : La suite Classique (limitée au Cœur 0)
# ---------------------------------------------------------
(
    echo "[Classique] Démarrage de la file d'attente (Core 0)..."
    
    taskset -c 0 ./simulator_classique -m 0 -M 15 -s 1 -e 100 -K 32 -N 128 -D "rep-hard" -o "classique_sim1_hard_128.csv"
    taskset -c 0 ./simulator_classique -m 0 -M 12 -s 1 -e 100 -K 32 -N 128 -D "rep-soft" -o "classique_sim2_soft_128.csv"
    taskset -c 0 ./simulator_classique -m 0 -M 12 -s 1 -e 100 -K 32 -N 96 -D "rep-soft" -o "classique_sim3_soft_96.csv"
    taskset -c 0 ./simulator_classique -m 0 -M 12 -s 1 -e 100 -K 32 -N 64 -D "rep-soft" -o "classique_sim4_soft_64.csv"
    taskset -c 0 ./simulator_classique -m 0 -M 12 -s 1 -e 100 -K 32 -N 32 -D "rep-soft" -o "classique_sim5_soft_32.csv"
    
    echo "[Classique] File d'attente terminée !"
) &
PID_CLASSIQUE=$!

# ---------------------------------------------------------
# BLOC 2 : La suite Multithread (sur les Cœurs 1 à 5)
# ---------------------------------------------------------
(
    echo "[Hyper] Démarrage de la file d'attente (Cores 1 à 5, avec 5 threads)..."
    
    # On utilise taskset pour allouer les 5 coeurs, et -t 5 pour dire à ton code de créer 5 threads
    taskset -c 1-5 ./simulator_hyper -m 0 -M 15 -s 1 -e 100 -K 32 -N 128 -D "rep-hard" -t 5 -o "hyper_sim1_hard_128.csv"
    taskset -c 1-5 ./simulator_hyper -m 0 -M 12 -s 1 -e 100 -K 32 -N 128 -D "rep-soft" -t 5 -o "hyper_sim2_soft_128.csv"
    taskset -c 1-5 ./simulator_hyper -m 0 -M 12 -s 1 -e 100 -K 32 -N 96 -D "rep-soft" -t 5 -o "hyper_sim3_soft_96.csv"
    taskset -c 1-5 ./simulator_hyper -m 0 -M 12 -s 1 -e 100 -K 32 -N 64 -D "rep-soft" -t 5 -o "hyper_sim4_soft_64.csv"
    taskset -c 1-5 ./simulator_hyper -m 0 -M 12 -s 1 -e 100 -K 32 -N 32 -D "rep-soft" -t 5 -o "hyper_sim5_soft_32.csv"
    
    echo "[Hyper] File d'attente terminée !"
) &
PID_HYPER=$!

# ---------------------------------------------------------
# BLOC 3 : Attente
# ---------------------------------------------------------
echo "Les deux processeurs tournent à plein régime. On attend la fin..."
wait $PID_CLASSIQUE
wait $PID_HYPER

echo "=== TOUTES LES SIMULATIONS SONT TERMINÉES ! Tu as 10 fichiers CSV prêts. ==="