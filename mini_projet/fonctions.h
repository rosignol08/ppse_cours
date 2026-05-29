#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <ctime>
#include <chrono>

#include <threads.h>
#include <atomic>
#include <unistd.h>
#include <fstream>
#include <getopt.h>
#include <arm_neon.h>

void source_generate(uint8_t * U_K, size_t K);

void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps);

void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N);
void modem_BPSK_modulate_all_ones(const uint8_t * C_N, int32_t *X_N, size_t K); //TODO faut la definir
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

//////////////TASK 1

typedef struct args_comm_chain {
	size_t K;
	size_t N;
	float sigma;
	std::string decode_method;
	bool src_all_zeros;
	bool mod_all_ones;
	size_t s_quant;
	size_t f_quant;
	std::atomic<int> *n_bit_errors;
	std::atomic<int> *n_trames_errors;
	std::atomic<int> *nb_simulation;
	int n_max_errors;
} Args_comm_chain;

Args_comm_chain init_args_comm_chain(size_t K, size_t N, float sigma, std::string decode_method, bool src_all_zeros, bool mod_all_ones, size_t s_quant, size_t f_quant, std::atomic<int> *n_bit_errors, std::atomic<int> *n_trames_errors, std::atomic<int> *nb_simulation, int n_max_errors);
void* communication_chain(void* args_void);

//////////////FIN TASK 1

//////////////TASK 2
void modem_BPSK_modulate_vectorisee_neon(const uint8_t *C_N, int32_t *X_N, size_t N);
void modem_BPSK_demodulate_neon(const float *Y_N, float *L_N, size_t N, float sigma);
void monitor_check_errors_neon(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors);
void montecarlo_simulation_task2( float m_arg, float M_arg, float s_arg, uint e_arg, uint K_arg, uint N_arg, std::string D_arg,const std::string &filename, bool mod_all_ones,size_t s_quant,size_t f_quant, int src_all_zeros);
void source_generate_task2(uint8_t * U_K, size_t K);
void codec_repetition_encode_task2(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps);
void modem_BPSK_modulate_vectorisee_neon_task2(const uint8_t *C_N, int32_t *X_N, size_t N);
void monitor_check_errors_neon_task2(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors);
void codec_repetition_hard_decode8_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);
void codec_repetition_soft_decode8_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);
void codec_repetition_soft_decode8_neon_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);
void codec_repetition_hard_decode8_neon_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps);
void codec_repetition_hard_decode_task2(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps);
void codec_repetition_soft_decode_task2(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps);

//////////////FIN TASK 2





