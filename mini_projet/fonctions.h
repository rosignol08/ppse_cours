#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <ctime>
#include <chrono>

#include <unistd.h>
#include <fstream>
#include <getopt.h>
#include <arm_neon.h>//task 6

void source_generate(uint8_t * U_K, size_t K);

void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps);

void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N);

void channel_AWGN_add_noise(const int32_t *X_N, float *Y_N, size_t N, float sigma);

void modem_BPSK_demodulate(const float *Y_N, float *L_N, size_t N, float sigma);

void codec_repetition_hard_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps);

void codec_repetition_soft_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps);

void monitor_check_errors(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors);

void append_result(const std::string &filename, float eb_n0, float es_n0, float sigma, int be, int fe, int fn, float ber, float fer, double sim_time, double time_per_frame, float sim_thr);

void quantizer_transform8(const float *L_N, int8_t *L8_N, size_t N, size_t s, size_t f);

void codec_repetition_hard_decode8(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);

void codec_repetition_soft_decode8(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);

void codec_repetition_soft_decode8_neon(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);

void codec_repetition_hard_decode8_neon(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);

void source_generate_all_zeros(uint8_t *U_K, size_t K);

void montecarlo_simulation( float m_arg, float M_arg, float s_arg, uint e_arg, uint K_arg, uint N_arg, std::string D_arg,const std::string &filename, bool mod_all_ones,size_t s_quant,size_t f_quant, int src_all_zeros);