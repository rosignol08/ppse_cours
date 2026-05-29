#include "fonctions.h"


void modem_BPSK_modulate_vectorisee_neon(const uint8_t *C_N, int32_t *X_N, size_t N) {
    size_t i = 0;

    //vecteur pour les comparaison etc
    int32x4_t v_un = vdupq_n_s32(1);
    int32x4_t v_deux = vdupq_n_s32(2);
    int32x4_t v_c;

    //4 elem par iteration
    for (; i <= N - 4; i += 4) {
        
        //load dans un reg 32 bits avec vset_lane 
        v_c = vdupq_n_s32(0);
        v_c = vsetq_lane_s32(C_N[i],   v_c, 0);
        v_c = vsetq_lane_s32(C_N[i+1], v_c, 1);
        v_c = vsetq_lane_s32(C_N[i+2], v_c, 2);
        v_c = vsetq_lane_s32(C_N[i+3], v_c, 3);

        //calul 1 - (2 * C_N)
        //la multiplication
        int32x4_t v_mul = vmulq_s32(v_c, v_deux);
        
        //la soustrac
        int32x4_t v_res = vsubq_s32(v_un, v_mul);

        //stock en mem
        vst1q_s32(&X_N[i], v_res);
    }

    //clean si des elements sont pas traites
    for (; i < N; i++) {
        X_N[i] = (C_N[i] == 0) ? 1 : -1;
    }
}

void modem_BPSK_modulate_vectorisee_neon_task2(const uint8_t *C_N, int32_t *X_N, size_t N) {
    size_t i = 0;// pour les octets C_N
    size_t j = 0;// pour les symboles X_N

    //vecteur pour les comparaison etc
    int32x4_t v_un = vdupq_n_s32(1);
    int32x4_t v_deux = vdupq_n_s32(2);
    //int32x4_t v_c;
    //int8x16_t octet;
    //4 elem par iteration
    for (; j <= N - 8; j += 8, i++) {
        uint8_t octet = C_N[i];
        
        //load dans un reg 32 bits avec vset_lane 
        int32x4_t v_c1;
        v_c1 = vdupq_n_s32(0);

        v_c1 = vsetq_lane_s32(((octet >> 0)) & 1, v_c1, 0);
        v_c1 = vsetq_lane_s32(((octet >> 1)) & 1, v_c1, 1);
        v_c1 = vsetq_lane_s32(((octet >> 2)) & 1, v_c1, 2);
        v_c1 = vsetq_lane_s32(((octet >> 3)) & 1, v_c1, 3);

        //calul 1 - (2 * C_N)
        //la multiplication
        int32x4_t v_mul1 = vmulq_s32(v_c1, v_deux);
        
        //la soustrac
        int32x4_t v_res1 = vsubq_s32(v_un, v_mul1);

        //stock en mem des 4 prem res
        vst1q_s32(&X_N[j], v_res1);

        int32x4_t v_c2 = vdupq_n_s32(0);
        //les 4 derniers bits
        v_c2 = vsetq_lane_s32((octet >> 4) & 1, v_c2, 0);
        v_c2 = vsetq_lane_s32((octet >> 5) & 1, v_c2, 1);
        v_c2 = vsetq_lane_s32((octet >> 6) & 1, v_c2, 2);
        v_c2 = vsetq_lane_s32((octet >> 7) & 1, v_c2, 3);

        //calcul 1 - (2 * c)
        int32x4_t v_mul2 = vmulq_s32(v_c2, v_deux);
        int32x4_t v_res2 = vsubq_s32(v_un, v_mul2);
        
        //stock en mem des 4 prem res (j+4)
        vst1q_s32(&X_N[j + 4], v_res2);
    }

    //clean si des elements sont pas traites
    if (j < N) {
        uint8_t octet_restant = C_N[i];
        int bit_idx = 0;
        for (; j < N; j++) {
            int bit_val = (octet_restant >> bit_idx) & 1;
            X_N[j] = (bit_val == 0) ? 1 : -1;
            bit_idx++;
        }
    }
}

void modem_BPSK_demodulate_neon(const float *Y_N, float *L_N, size_t N, float sigma) {
    //calcul scalaire
    float variance = sigma * sigma;
    float facteur = 2.0f / variance;
    
    size_t i = 0;

    //dupliquation du facteur dans les 4 cases d'un reg vu qu'on fait 4 float par iteration
    float32x4_t v_facteur = vdupq_n_f32(facteur);

    //4 floats par iteration
    for (; i <= N - 4; i += 4) {
        
        //load des 4 floats depuis tab Y_N
        float32x4_t v_y = vld1q_f32(&Y_N[i]);
        
        //mul 4 valeurs par facteur
        float32x4_t v_l = vmulq_f32(v_y, v_facteur);
        
        //stock 4 res dans tab L_N
        vst1q_f32(&L_N[i], v_l);
    }

    //clean si des elements sont pas traites genre si N %4 != 0
    for (; i < N; i++) {
        L_N[i] = Y_N[i] * facteur;
    }
}

void monitor_check_errors_neon(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors) {
    
    //nombre d'erreur de base
    uint64_t initial_bit_errors = *n_bit_errors;
    size_t i = 0;

    //toujour les vecteur
    uint8x16_t v_un  = vdupq_n_u8(1);
    uint8x16_t err_acc = vdupq_n_u8(0); //cot erreurs

    //16 par 16
    for (; i <= K - 16; i += 16) {
        
        //load les donness
        uint8x16_t u = vld1q_u8(&U_K[i]);
        uint8x16_t v = vld1q_u8(&V_K[i]);

        //compar 0xFF si == 0x00 si !=
        uint8x16_t cmp_mask = vceqq_u8(u, v);

        //recup les erreurs avec AND NOT (vbic) 
        //si cmp_mask = 0xFF c'est psk (1 AND NOT 0xFF) = 0
        //si cmp_mask = 0x00 c'est psk (1 AND NOT 0x00) = 1 erreur
        uint8x16_t err_bits = vbicq_u8(v_un, cmp_mask);

        //incr des erreurs avec sature pour eviter les depassement de capacitees
        err_acc = vqaddq_u8(err_acc, err_bits);
    }
    
    //recup les erreurs
    uint8_t T[16];
    vst1q_u8(T, err_acc);
    for (int j = 0; j < 16; j++) {
        *n_bit_errors += T[j];
    }

    //clean si des elements sont pas traites
    for (; i < K; i++) {
        if (U_K[i] != V_K[i]) {
            (*n_bit_errors)++;
        }
    }

    //la tramme fausse
    if (*n_bit_errors > initial_bit_errors) {
        (*n_frame_errors)++;
    }
}

void monitor_check_errors_neon_task2(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors) {
    
    //nombre d'erreur de base
    uint64_t initial_bit_errors = *n_bit_errors;
    size_t i = 0;

    //toujour les vecteur
    uint8x16_t err_acc = vdupq_n_u8(0); //cot erreurs
    uint8_t T[16];
    
    //16 octets par 16
    for (; i <= K/8 - 16; i += 16) {
        //load les donness
        uint8x16_t u = vld1q_u8(&U_K[i]);
        uint8x16_t v = vld1q_u8(&V_K[i]);
        

        //compar 0xFF si == 0x00 si !=
        //uint8x16_t cmp_mask = vceqq_u8(u, v);//pour comparer les octets on met les differences dans masque_egalite_ou_pas
        uint8x16_t xor_diff = veorq_u8(u, v);//pareil avec xor

        uint8x16_t bit_errors = vcntq_u8(xor_diff);

        //recup les erreurs avec AND NOT (vbic) 
        //si cmp_mask = 0xFF c'est psk (1 AND NOT 0xFF) = 0
        //si cmp_mask = 0x00 c'est psk (1 AND NOT 0x00) = 1 erreur
        //uint8x16_t err_bits = vbicq_u8(v_un, cmp_mask);

        //incr des erreurs avec sature pour eviter les depassement de capacitees
        //err_acc = vqaddq_u8(err_acc, err_bits);
        err_acc = vqaddq_u8(err_acc, bit_errors);

        //si on a une erreur
        if ((i / 16) % 255 == 254) {
            vst1q_u8(T, err_acc);//page 20
            for (int j = 0; j < 16; j++) {
                *n_bit_errors += T[j]; //si on a une erreur
            }
            err_acc = vdupq_n_u8(0); //pour vider au cas ou mais pas sur que ca soit super utile dans notre cas
        }

    }
    
    //recup les erreurs
    vst1q_u8(T, err_acc);
    for (int j = 0; j < 16; j++) {
        *n_bit_errors += T[j];
    }

    //clean si des elements sont pas traites
    for (; i < K/8; i++) {
        //if (U_K[i] != V_K[i]) {
        uint8_t difference = U_K[i] ^ V_K[i];
        uint8_t erreurs_dans_octet = 0;
        for (int b = 0; b < 8; b++) {
            erreurs_dans_octet += (difference >> b) & 1;
        }
        *n_bit_errors += erreurs_dans_octet;
    }

    //la tramme fausse
    if (*n_bit_errors > initial_bit_errors) {
        (*n_frame_errors)++;
    }
}

void codec_repetition_hard_decode8_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
    //8 elem par iteration
    for(size_t i = 0; i < K; i += 8){
        uint8_t octet = 0; //l'octet qui va contenir nos 8 res
        
        for(int b = 0; b < 8; b++) {
            if ((i + b) >= K) break; //clean si deborde
            
            int vote = 0;
            for(size_t j = 0; j < n_reps; j++){
                if(L8_N[(j*K) + (i+b)] >= 0) vote++;
                else vote--;
            }
            
            //si vote > 0 c'est 0 sinon c'est 1
            uint8_t bit_res = (vote > 0) ? 0 : 1;
            
            //decale le bit et l'insere dans l'octet
            octet |= (bit_res << b);
        }
        V_K[i / 8] = octet; //stock l'octet
    }
}

void codec_repetition_soft_decode8_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
    //8 elem par iteration
    for(size_t i = 0; i < K; i += 8){
        uint8_t octet = 0; //l'octet qui va contenir nos 8 res
        
        for(int b = 0; b < 8; b++) {
            if ((i + b) >= K) break; //clean si deborde
            
            int vote = 0; 
            for(size_t j = 0; j < n_reps; j++){
                vote += L8_N[(j*K) + (i+b)];
            }
            
            //si vote > 0 c'est 0 sinon c'est 1
            uint8_t bit_res = (vote > 0) ? 0 : 1;
            
            //decale le bit et l'insere dans l'octet
            octet |= (bit_res << b);
        }
        V_K[i / 8] = octet; //stock l'octet
    }
}

void codec_repetition_soft_decode8_neon_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
    size_t i = 0;
    
    //vecteurs pour les valeurs fixes
    int8x16_t v_un = vdupq_n_s8(1);
    
    //16 elem par iteration
    for(; i <= K - 16; i += 16) {
        int8x16_t vote_acc = vdupq_n_s8(0);
        
        //somme des rep
        for(size_t j = 0; j < n_reps; j++) {
            const int8_t* ptr = &L8_N[(j * K) + i];
            //load 16 LLRs
            int8x16_t llr_vec = vld1q_s8(ptr);
            //add les 16 LLR
            vote_acc = vqaddq_s8(vote_acc, llr_vec);
        }
        
        //la decision 
        int8x16_t decision_mask = (int8x16_t)vcltzq_s8(vote_acc);
        int8x16_t final_decision = vandq_s8(decision_mask, v_un);
        
        //compression (bit-packing)
        uint8_t T[16];
        vst1q_u8(T, (uint8x16_t)final_decision); //extrait les 16 bits non-compresses
        
        uint8_t octet1 = 0, octet2 = 0;
        for(int b = 0; b < 8; b++) {
            octet1 |= (T[b] << b);       //compresse les 8 premiers bits
            octet2 |= (T[b+8] << b);     //compresse les 8 suivants
        }
        
        //stock aux index
        V_K[i / 8]     = octet1;
        V_K[(i / 8) + 1] = octet2;
    }
    
    //clean si des elements sont pas traites
    for(; i < K; i++){
        int vote = 0;
        for(size_t j = 0; j < n_reps; j++) vote += L8_N[(j*K)+i];
        
        uint8_t bit_res = (vote > 0) ? 0 : 1;
        
        //manipulation de bit
        if (bit_res == 1) V_K[i/8] |=  (1 << (i % 8)); //met a 1
        else              V_K[i/8] &= ~(1 << (i % 8)); //met a 0
    }
}

void codec_repetition_hard_decode8_neon_task2(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
    size_t i = 0;
    
    //vecteurs pour les valeurs fixes
    int8x16_t v_un = vdupq_n_s8(1);
    
    //16 elem par iteration
    for(; i <= K - 16; i += 16) {
        int8x16_t vote_acc = vdupq_n_s8(0);
        
        //somme des rep
        for(size_t j = 0; j < n_reps; j++) {
            const int8_t* ptr = &L8_N[(j * K) + i];
            //load 16 LLRs
            int8x16_t llr_vec = vld1q_s8(ptr);
            
            //mask : 0xFF si negatif
            int8x16_t is_neg_mask = (int8x16_t)vcltzq_s8(llr_vec);
            //calcul pour avoir -1 ou +1
            int8x16_t temp = vaddq_s8(is_neg_mask, is_neg_mask);
            int8x16_t votes = vaddq_s8(temp, v_un);
            
            //accumule les votes
            vote_acc = vqaddq_s8(vote_acc, votes);
        }
        
        //la decision
        int8x16_t decision_mask = (int8x16_t)vcltzq_s8(vote_acc);
        int8x16_t final_decision = vandq_s8(decision_mask, v_un);
        
        //compression (bit-packing)
        uint8_t T[16];
        vst1q_u8(T, (uint8x16_t)final_decision);
        
        uint8_t octet1 = 0, octet2 = 0;
        for(int b = 0; b < 8; b++) {
            octet1 |= (T[b] << b);
            octet2 |= (T[b+8] << b);
        }
        
        //stock aux index
        V_K[i / 8]     = octet1;
        V_K[(i / 8) + 1] = octet2;
    }
    
    //clean si des elements sont pas traites
    for(; i < K; i++){
        int vote = 0;
        for(size_t j = 0; j < n_reps; j++){
            if(L8_N[(j*K)+i] >= 0) vote++;
            else vote--;
        }
        
        uint8_t bit_res = (vote > 0) ? 0 : 1;
        
        //manipulation de bit
        if (bit_res == 1) V_K[i/8] |=  (1 << (i % 8));
        else              V_K[i/8] &= ~(1 << (i % 8));
    }
}

void codec_repetition_hard_decode_task2(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps){
    //8 elem par iteration
    for(size_t i = 0; i < K; i += 8){
        uint8_t octet = 0; //octet compresse
        
        for(int b = 0; b < 8; b++) {
            if ((i + b) >= K) break; //clean si deborde
            
            int vote = 0;
            for(size_t j = 0; j < n_reps; j++){
                if(L_N[(j*K) + (i+b)] >= 0.0f){
                    vote++;
                }else{
                    vote--;
                }
            }
            
            //decision : 0 si positif, 1 si negatif
            uint8_t bit_res = (vote > 0) ? 0 : 1;
            
            //decale et insere
            octet |= (bit_res << b);
        }
        
        //stock l'octet complet
        V_K[i / 8] = octet;
    }
}

void codec_repetition_soft_decode_task2(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps){
    //8 elem par iteration
    for(size_t i = 0; i < K; i += 8){
        uint8_t octet = 0; //octet compresse
        
        for(int b = 0; b < 8; b++) {
            if ((i + b) >= K) break; //clean si deborde
            
            float vote = 0.0f;
            for(size_t j = 0; j < n_reps; j++){
                //somme des LLR
                vote += L_N[(j*K) + (i+b)];
            }
            
            //decision : 0 si positif, 1 si negatif
            uint8_t bit_res = (vote > 0.0f) ? 0 : 1;
            
            //decale et insere
            octet |= (bit_res << b);
        }
        
        //stock l'octet complet
        V_K[i / 8] = octet;
    }
}

void montecarlo_simulation_task2(float m_arg, float M_arg, float s_arg, uint e_arg, uint K_arg, uint N_arg, std::string D_arg,const std::string &filename, bool mod_all_ones,size_t s_quant,size_t f_quant, bool src_all_zeros){
	/*
	-m [min_SNR float] the first included Eb/N0 SNR to simulate (in dB),
	-M [max_SNR float] the last included Eb/N0 SNR to simulate (in dB),
	-s [step_val float] the constant step between two SNR points,
	-e [f_max uint] the number of frame errors to reach to explore one SNR point,
	-K [info_bits uint] the number of information bits,
	-N [codeword_size uint] the codeword size (has to be a multiple of K otherwise the program should return an error),
	-D ["rep-hard"|"rep-soft" string] select the decoder type.
	*/

  // Déclarations
	std::atomic<int> n_bit_errors, n_trames_errors=0, nb_erreurs, nb_simulation, nb_bits_erreurs;

  // Initialisations
	static size_t K = K_arg;
	size_t N = N_arg;
	float sigma = 0.5f;
	float Ber = 0.0f;
	float Fer = 0.0f;
	float sim_thr = 0.0f;
	int n_max_errors = (int) e_arg;
	const int nb_threads = 6;

	#ifdef ENABLE_STATS
	enum BlockId {
		BLOCK_SOURCE = 0,
		BLOCK_ENCODER = 1,
		BLOCK_MODULATOR = 2,
		BLOCK_CHANNEL = 3,
		BLOCK_DEMODULATOR = 4,
		BLOCK_MONITOR = 5,
		BLOCK_COUNT = 6
	};

	const char *block_labels[BLOCK_COUNT] = {
		"Source generate",
		"Encoder",
		"Modulator",
		"Channel",
		"Demodulator",
		"Monitor"
	};

	double total_duration[BLOCK_COUNT] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	double min_duration[BLOCK_COUNT] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	double max_duration[BLOCK_COUNT] = {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0};
	size_t block_calls[BLOCK_COUNT] = {0, 0, 0, 0, 0, 0};

	auto record_block = [&](int block,
				const std::chrono::high_resolution_clock::time_point &start,
				const std::chrono::high_resolution_clock::time_point &end) {
		double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
		total_duration[block] += duration_ms;
		block_calls[block]++;
		if(max_duration[block] < duration_ms || max_duration[block] < 0.0) max_duration[block] = duration_ms;
		if(min_duration[block] > duration_ms || min_duration[block] == 0.0) min_duration[block] = duration_ms;
	};
	#endif

  	// Simule pour chaque SNR
	for(float i = m_arg; i <= M_arg; i += s_arg){
		auto start_snr = std::chrono::high_resolution_clock::now(); //debut mesure
		nb_bits_erreurs = 0;
		nb_simulation = 0;
		nb_erreurs = 0;
		n_trames_errors = 0;
		n_bit_errors = 0;

		float snr_symb = i + 10*log10f((float)K/N_arg); //sinon ça donne 0 et ça fait bugger tout le programme
		sigma = sqrt(1/(2 * powf(10, snr_symb/10)));
		
		Args_comm_chain thread_args = init_args_comm_chain(K, N, sigma, D_arg, src_all_zeros, mod_all_ones, s_quant, f_quant, &n_bit_errors, &n_trames_errors, &nb_simulation, n_max_errors);
		pthread_t threads[nb_threads]; // déclare un tableau de thread

		// lancement des threads
		for (int i = 0; i < nb_threads; i++){ // lance les threads un par un
			int code = pthread_create(threads+i, NULL, communication_chain, &thread_args);
			if (code != 0) {
				printf("EAGAIN = %d, EINVAL = %d, EPERM = %d, code = %d\n", EAGAIN, EINVAL, EPERM, code);
				printf("ERREUR lors de la création du thread %d, code d'erreur = %d\n", i, code);
				exit(1);
			}
		}
		for (int i = 0; i < nb_threads; i++){
			pthread_join(threads[i], NULL); // attend que chaque thread soit terminé
		}

    // Boucle jusqu'a obtenir un certain nombre d'erreur pour ce SNR
		// nb_erreurs = n_trames_errors;
		nb_erreurs.store(n_trames_errors);
		nb_bits_erreurs += n_bit_errors;
		Ber = (float)nb_bits_erreurs / (nb_simulation * K);
		Fer = (float)nb_erreurs / nb_simulation;
		//std::cout << "Ber : " << Ber << std::endl;
		//std::cout << "Fer : " << Fer << std::endl;
		auto end_snr = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> diff = end_snr - start_snr;//vu que c'est des chrono on est obligé de faire ça selon internet
		double sim_time = diff.count(); //temps total pour ce SNR (en millisecondes)
		double time_per_frame = sim_time / nb_simulation; //temps moyen par trame en millisecondes
		sim_thr = float(nb_simulation * K) / (sim_time * 1e3); //débit de simulation en Mbps

		std::cout << "SNR : " << i << " | Ber : " << Ber << " | Fer : " << Fer << " | Trames simulees : " << nb_simulation << " | Sim_thr : " << sim_thr << " Mbps" << std::endl;

		//ça ajoute les résultats dans le fichier à chaque itération
		append_result(filename, i, snr_symb, sigma, nb_bits_erreurs, nb_erreurs, nb_simulation, Ber, Fer, sim_time, time_per_frame, sim_thr);

		#ifdef ENABLE_STATS
		double avg_duration[BLOCK_COUNT] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
		double percent_duration[BLOCK_COUNT] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
		double throughput[BLOCK_COUNT] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
		double output_bits[BLOCK_COUNT] = {
			(double)nb_simulation * (double) K,
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double) K
		};

		for(int block = 0; block < BLOCK_COUNT; ++block){
			if(block_calls[block] > 0){
				avg_duration[block] = total_duration[block] / (double)block_calls[block];
				throughput[block] = (output_bits[block] / total_duration[block]) * 1e-3;
			}
			if(sim_time > 0.0){
				percent_duration[block] = (total_duration[block] / sim_time) * 100.0;
			}
		}

		std::cout << "----- Stats de SNR=" << i << " -----" << std::endl;
		std::cout << "-- Temps totale --" << std::endl;
		std::cout << "NB de bits transféré: " << (K * nb_simulation) << " bits" << std::endl;
		std::cout << "Durée totale: " << sim_time << " ms" << std::endl;
		std::cout << "Durée moyenne: " << time_per_frame << " ms" << std::endl;
		std::cout << "Throughput de la communication: " << sim_thr << " Mbps" << std::endl;
		std::cout << std::endl;

		for(int block = 0; block < BLOCK_COUNT; ++block){
			std::cout << "-- " << block_labels[block] << " --" << std::endl;
			std::cout << "Durée moyenne: " << avg_duration[block] << " ms" << std::endl;
			std::cout << "Durée minimum: " << min_duration[block] << " ms" << std::endl;
			std::cout << "Durée maximum: " << max_duration[block] << " ms" << std::endl;
			std::cout << "Throughput moyen : " << throughput[block] << " Mbps" << std::endl;
			std::cout << "Pourcentage de la durée: " << percent_duration[block] << " %" << std::endl;
			std::cout << std::endl;
		}

		for(int block = 0; block < BLOCK_COUNT; ++block){
			total_duration[block] = 0.0;
			min_duration[block] = 0.0;
			max_duration[block] = -1.0;
			block_calls[block] = 0;
		}
		#endif
	}
}

