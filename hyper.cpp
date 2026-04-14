#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <fstream>
#include <omp.h>   // <-- OpenMP

/**
 * @brief Simule la source d'information avec un générateur local (thread-safe).
 *
 * On passe maintenant le générateur en paramètre pour éviter les données
 * partagées entre threads.
 */
void source_generate(uint8_t *U_K, size_t K, std::mt19937 &rng) {
    std::uniform_int_distribution<int> dist(0, 1);
    for (size_t i = 0; i < K; ++i) {
        U_K[i] = (uint8_t)dist(rng);
    }
}

void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps) {
    for (size_t i = 0; i < K * n_reps; i++) {
        C_N[i] = U_K[i % K];
    }
}

void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N) {
    for (size_t i = 0; i < N; i++) {
        X_N[i] = (C_N[i] == 0) ? 1 : -1;
    }
}

/**
 * @brief Canal AWGN thread-safe : le générateur est passé en paramètre.
 */
void channel_AWGN_add_noise(const int32_t *X_N, float *Y_N, size_t N, float sigma,
                             std::mt19937 &rng) {
    std::normal_distribution<float> distribution(0.0f, sigma);
    for (size_t i = 0; i < N; i++) {
        Y_N[i] = (float)X_N[i] + distribution(rng);
    }
}

void modem_BPSK_demodulate(const float *Y_N, float *L_N, size_t N, float sigma) {
    float facteur = 2.0f / (sigma * sigma);
    for (size_t i = 0; i < N; i++) {
        L_N[i] = Y_N[i] * facteur;
    }
}

void codec_repetition_hard_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps) {
    for (size_t i = 0; i < K; i++) {
        int vote = 0;
        for (size_t j = 0; j < n_reps; j++) {
            vote += (L_N[j * K + i] >= 0.0f) ? 1 : -1;
        }
        V_K[i] = (vote > 0) ? 0 : 1;
    }
}

void codec_repetition_soft_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps) {
    for (size_t i = 0; i < K; i++) {
        float vote = 0.0f;
        for (size_t j = 0; j < n_reps; j++) {
            vote += L_N[j * K + i];
        }
        V_K[i] = (vote > 0.0f) ? 0 : 1;
    }
}

void monitor_check_errors(const uint8_t *U_K, const uint8_t *V_K, size_t K,
                           uint64_t *n_bit_errors, uint64_t *n_frame_errors) {
    bool frame_error = false;
    for (size_t i = 0; i < K; i++) {
        if (U_K[i] != V_K[i]) {
            (*n_bit_errors)++;
            frame_error = true;
        }
    }
    if (frame_error) (*n_frame_errors)++;
}

void append_result(const std::string &filename, float eb_n0, float es_n0, float sigma,
                   int64_t be, int64_t fe, int64_t fn, float ber, float fer,
                   double sim_time, double time_per_frame) {
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir '" << filename << "'.\n";
        return;
    }
    file << eb_n0 << "," << es_n0 << "," << sigma << ","
         << be << "," << fe << "," << fn << ","
         << ber << "," << fer << "," << sim_time << "," << time_per_frame << "\n";
    file.close();
}


void montecarlo_simulation(float m_arg, float M_arg, float s_arg, uint32_t e_arg,
                            uint32_t K_arg, uint32_t N_arg, std::string D_arg,
                            const std::string &filename) {
    size_t K      = K_arg;
    size_t n_reps = N_arg / K_arg;
    size_t N      = K * n_reps;

    int n_threads = omp_get_max_threads(); // utilise tous les cœurs disponibles
    std::cout << "Simulation multithreads avec " << n_threads << " threads." << std::endl;

    for (float snr_db = m_arg; snr_db <= M_arg; snr_db += s_arg) {
        auto start_snr = std::chrono::high_resolution_clock::now();

        float snr_symb = snr_db + 10.0f * log10f((float)K / N_arg);
        float sigma    = sqrtf(1.0f / (2.0f * powf(10.0f, snr_symb / 10.0f)));

        // Compteurs globaux (accumulés par tous les threads)
        int64_t total_bit_errors   = 0;
        int64_t total_frame_errors = 0;
        int64_t total_simulations  = 0;

        /*
         * Chaque thread tourne sa propre boucle Monte Carlo jusqu'à ce que
         * le total de frame errors atteigne e_arg.
         *
         * On utilise une variable partagée `total_frame_errors` protégée par
         * des atomics OpenMP (reduction + flush) pour arrêter proprement.
         */
        #pragma omp parallel reduction(+: total_bit_errors, total_frame_errors, total_simulations)
        {
            // Buffers locaux à chaque thread → pas de contention mémoire
            uint8_t  *U_K = (uint8_t  *)malloc(K * sizeof(uint8_t));
            uint8_t  *C_N = (uint8_t  *)malloc(N * sizeof(uint8_t));
            uint8_t  *V_K = (uint8_t  *)malloc(K * sizeof(uint8_t));
            int32_t  *X_N = (int32_t  *)malloc(N * sizeof(int32_t));
            float    *Y_N = (float    *)malloc(N * sizeof(float));
            float    *L_N = (float    *)malloc(N * sizeof(float));

            // Chaque thread a son propre RNG, seedé différemment
            unsigned int seed = (unsigned int)(std::chrono::high_resolution_clock::now()
                                    .time_since_epoch().count())
                                ^ ((unsigned int)omp_get_thread_num() * 2654435761u);
            std::mt19937 rng(seed);

            int64_t local_bit_errors   = 0;
            int64_t local_frame_errors = 0;
            int64_t local_simulations  = 0;

            // Chaque thread travaille jusqu'à ce que le total global soit atteint.
            // On lit total_frame_errors de façon non-atomique (lecture approx.)
            // → on peut dépasser légèrement e_arg, c'est acceptable en Monte Carlo.
            while (true) {
                // Vérification condition d'arrêt (lecture partagée, flush OpenMP)
                int64_t current_fe;
                #pragma omp atomic read
                current_fe = total_frame_errors;
                if (current_fe + local_frame_errors >= (int64_t)e_arg) break;

                uint64_t bit_err   = 0;
                uint64_t frame_err = 0;

                source_generate(U_K, K, rng);
                codec_repetition_encode(U_K, C_N, K, n_reps);
                modem_BPSK_modulate(C_N, X_N, N);
                channel_AWGN_add_noise(X_N, Y_N, N, sigma, rng);
                modem_BPSK_demodulate(Y_N, L_N, N, sigma);

                if (D_arg == "rep-hard")
                    codec_repetition_hard_decode(L_N, V_K, K, n_reps);
                else
                    codec_repetition_soft_decode(L_N, V_K, K, n_reps);

                monitor_check_errors(U_K, V_K, K, &bit_err, &frame_err);

                local_bit_errors   += (int64_t)bit_err;
                local_frame_errors += (int64_t)frame_err;
                local_simulations  += 1;

                // Met à jour le compteur global régulièrement (tous les 64 frames)
                // pour que les autres threads puissent voir la progression.
                if (local_simulations % 64 == 0) {
                    #pragma omp atomic update
                    total_frame_errors += local_frame_errors;
                    #pragma omp atomic update
                    total_bit_errors   += local_bit_errors;
                    #pragma omp atomic update
                    total_simulations  += local_simulations;
                    local_bit_errors   = 0;
                    local_frame_errors = 0;
                    local_simulations  = 0;
                }
            }

            // Flush final des compteurs locaux restants
            #pragma omp atomic update
            total_frame_errors += local_frame_errors;
            #pragma omp atomic update
            total_bit_errors   += local_bit_errors;
            #pragma omp atomic update
            total_simulations  += local_simulations;

            free(U_K); free(C_N); free(V_K);
            free(X_N); free(Y_N); free(L_N);
        } // fin section parallèle

        float BER = (total_simulations > 0)
                    ? (float)total_bit_errors / ((float)total_simulations * (float)K)
                    : 0.0f;
        float FER = (total_simulations > 0)
                    ? (float)total_frame_errors / (float)total_simulations
                    : 0.0f;

        auto end_snr = std::chrono::high_resolution_clock::now();
        double sim_time       = std::chrono::duration<double>(end_snr - start_snr).count();
        double time_per_frame = sim_time / (double)total_simulations;

        std::cout << "SNR: " << snr_db
                  << " | BER: " << BER
                  << " | FER: " << FER
                  << " | Trames: " << total_simulations
                  << " | Temps: " << sim_time << "s"
                  << std::endl;

        append_result(filename, snr_db, snr_symb, sigma,
                      total_bit_errors, total_frame_errors, total_simulations,
                      BER, FER, sim_time, time_per_frame);
    }
}


int main(int argc, char *argv[]) {
    int   opt;
    float m_arg = 0.0f, M_arg = 5.0f, s_arg = 0.5f;
    uint32_t e_arg = 100, K_arg = 1, N_arg = 3;
    std::string D_arg = "rep-soft";
    std::string output_filename = "results.csv";

    while ((opt = getopt(argc, argv, "m:M:s:e:K:N:D:o:t:")) != -1) {
        switch (opt) {
        case 'm': m_arg = atof(optarg); break;
        case 'M': M_arg = atof(optarg); break;
        case 's': s_arg = atof(optarg); break;
        case 'e': e_arg = atoi(optarg); break;
        case 'K': K_arg = atoi(optarg); break;
        case 'N': N_arg = atoi(optarg); break;
        case 'D': D_arg = optarg;       break;
        case 'o': output_filename = optarg; break;
        case 't':
            // Permet de forcer le nombre de threads via -t <n>
            omp_set_num_threads(atoi(optarg));
            break;
        case '?':
            std::cerr << "Option inconnue: " << (char)optopt << "\n";
            break;
        }
    }

    if (N_arg % K_arg != 0) {
        std::cerr << "Erreur: N doit etre un multiple de K.\n";
        return 1;
    }

    srand((unsigned int)time(NULL));

    std::ofstream clear_file(output_filename);
    clear_file << "Eb/N0,Es/N0,sigma,be,fe,fn,BER,FER,sim_time_s,time_per_frame_s\n";
    clear_file.close();

    auto start = std::chrono::high_resolution_clock::now();
    montecarlo_simulation(m_arg, M_arg, s_arg, e_arg, K_arg, N_arg, D_arg, output_filename);
    auto fin      = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(fin - start);
    std::cout << "Temps total : " << duration.count() << " ms" << std::endl;

    return 0;
}