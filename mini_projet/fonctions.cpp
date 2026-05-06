#include "fonctions.h"
void source_generate(uint8_t * U_K, size_t K){
	//check de Securitée
	//if(sizeof(U_K)<K){
	//	perror("k trop grand");
	//	return;
	//}
	uint8_t b = 0;
	for(size_t i = 0; i < K;++i){
		b = rand() % 2;
		if(b==0){
			U_K[i] = 0;
		}else{
			U_K[i] = 1;
		}
	}
}


void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps){
	for(size_t i = 0; i<K*n_reps;i++){
		C_N[i] = U_K[i%K];
	}
	return;
}

void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N){
	for(size_t i = 0; i < N; i++){
		if(C_N[i] == 0){
			X_N[i] = 1;
		}else{
			X_N[i] = -1;
		}
	}
}


void channel_AWGN_add_noise(const int32_t *X_N, float *Y_N, size_t N, float sigma){
	static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	std::normal_distribution<double> distribution(0.0, sigma);
	//std::default_random_engine generator;
	for(size_t i = 0; i < N; i++){
		Y_N[i] = (float)X_N[i] + distribution(generator);
	}
}

void modem_BPSK_demodulate(const float *Y_N, float *L_N, size_t N, float sigma){
	/*
	LLR exact pour canal AWGN et BPSK (+1/-1) :
	LLR(y) = ln( P(x=+1 | y) / P(x=-1 | y) ) = 2 * y / sigma^2
	*/
	float variance = sigma * sigma;
	float facteur = 2.0f / variance;
	
	for(size_t i = 0; i < N; i++){
		L_N[i] = Y_N[i] * facteur;//faut commenter * facteur pour avoir la fonction de base
	}
	return;
}

void codec_repetition_hard_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps){
	for(size_t i = 0; i < K;i++){
		int vote = 0;//le buffer qui va stoquer les valeur du vote pour chaque nombre
		for(size_t j = 0; j < n_reps;j++){
			if(L_N[(j*K)+i] >= 0.0f){
				vote++;
			}else{
				vote--;
			}
			//std::cout << (j*K)+i << "eme element = " << L_N[(j*K)+i] << std::endl;
		}
		V_K[i] = (vote > 0) ? 0 : 1;
		vote = 0; //remet à 0
	}
}

void codec_repetition_soft_decode(const float *L_N, uint8_t *V_K, size_t K, size_t n_reps){
	for(size_t i = 0; i < K;i++){
		float vote = 0.0f;
		for(size_t j = 0; j < n_reps;j++){
			//faut faire la somme des valeurs
			vote += L_N[(j*K)+i];
			//std::cout << (j*K)+i << "eme element = " << L_N[(j*K)+i] << std::endl;//debut
		}
		V_K[i] = (vote > 0.0f) ? 0 : 1;
		vote = 0.0f; //remet à 0
	}
}


void monitor_check_errors(const uint8_t *U_K, const uint8_t *V_K, size_t K, uint64_t *n_bit_errors, uint64_t *n_frame_errors){
	bool frame_error = false; //obligé de faire un booleen psk si jai 2 bit faux c'est 1 trame fausse
	for(size_t i = 0; i<K;i++){
		if(U_K[i]!=V_K[i]){
			(*n_bit_errors)++;
			frame_error = true; //la frame est en erreur si au moins un bit est faux
		}
	}
	if(frame_error){
		(*n_frame_errors)++;
	}
	//std::cout <<"il y a : " << *n_bit_errors << " erreurs et "<< *n_frame_errors << " trames fausses "<< std::endl;
}

void append_result(const std::string &filename, float eb_n0, float es_n0, float sigma, int be, int fe, int fn, float ber, float fer, double sim_time, double time_per_frame, float sim_thr) {
    // std::ios::app permet d'ajouter à la fin du fichier sans l'écraser
    std::ofstream file(filename, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier '" << filename << "'.\n";
        return;
    }
    
    //valeurs ecrites dans le csv
    file << eb_n0 << "," << es_n0 << "," << sigma << "," 
        << be << "," << fe << "," << fn << "," 
        << ber << "," << fer << "," << sim_time << "," << time_per_frame << "," << sim_thr << "\n";

    file.close();
}


void quantizer_transform8(const float *L_N, int8_t *L8_N, size_t N, size_t s, size_t f){
	//verification de s 
	if (s == 0) return;
	//clamp s entre [1,8] pour eviter les int8 overflow
	if (s > 8) s = 8;
	size_t frac = f;
	//les valeur min et max
	int32_t min_val = - (1 << (s - 1));
	int32_t max_val =  (1 << (s - 1)) - 1;
	//le scaling
	float scale = std::pow(2.0f, (float)f);

	for (size_t i = 0; i < N; ++i) {
		float scaled = L_N[i] * scale;
		//arondit à l'entier le plus proche
		long rounded = (long)roundf(scaled);
		//[min_val, max_val]
		if (rounded < min_val) rounded = min_val;
		if (rounded > max_val) rounded = max_val;
		L8_N[i] = (int8_t)rounded;
	}

}

void codec_repetition_hard_decode8(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
	for(size_t i = 0; i < K;i++){
		int vote = 0;//le buffer qui va stoquer les valeur du vote pour chaque nombre
		for(size_t j = 0; j < n_reps;j++){
			if(L8_N[(j*K)+i] >= 0.0f){
				vote++;
			}else{
				vote--;
			}
		}
		V_K[i] = (vote > 0) ? 0 : 1;
		vote = 0; //remet à 0
	}
}

void codec_repetition_soft_decode8(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
	for(size_t i = 0; i < K;i++){
		float vote = 0.0f;
		for(size_t j = 0; j < n_reps;j++){
			//faut faire la somme des valeurs
			vote += L8_N[(j*K)+i];
		}
		V_K[i] = (vote > 0.0f) ? 0 : 1;
		vote = 0.0f; //remet à 0
	}
}

void codec_repetition_soft_decode8_neon(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
    // On avance de 16 en 16 dans le tableau d'information
    for(size_t i = 0; i < K; i += 16) {
        
        //registre accumulateur (16 valeurs à 0)
        // vdupq_n_s8 : pour dupliquer une val dans un reg de 128 bits (q) d'entiers signés de 8 bits
        int8x16_t vote_acc = vdupq_n_s8(0);
        
        //somme des rep
        for(size_t j = 0; j < n_reps; j++) {
            //index dans L8_N est un peu différent psk data entrelacées
            //bit num i de la j-eme rep est ici (j * K) + i
            const int8_t* ptr = &L8_N[(j * K) + i];
            
            //load 16 LLRs en oneshot coup depuis la mem vers un reg
            int8x16_t llr_vec = vld1q_s8(ptr);
            
            //add les 16 LLR
            //!!!si on ajoute 100 + 50, au lieu de déborder et de devenir négatif, ça restera bloqué à 127. selon gemini
            vote_acc = vqaddq_s8(vote_acc, llr_vec);
        }
        
        //la décision (Soft -> Hard)
        //Si la somme LLR positif, V_K = 0 si LLR negatif, V_K = 1 (0xFF)
        uint8x16_t decision_mask_u8 = vcltzq_s8(vote_acc);
        
        //on veut stocker 1 (au lieu de 0xFF) quand c'est negatif
        //vandq_u8 (u8 pour le mask) pour faire un ET logique avec 1
        // vdupq_n_u8(1) gen un vecteur plein de 1.
        int8x16_t decision_mask = (int8x16_t)decision_mask_u8; // les 1 c'est des FF = 1111 1111 = 255 
		int8x16_t ones = vdupq_n_s8(1);
        int8x16_t final_decision = vandq_s8(decision_mask, ones);//transforme les 255 en 1
        
        //stock le res en mem
        //vst1q_u8 ça stocke le contenu du reg de 128 bits à l'@ indique
        vst1q_s8((int8_t*)&V_K[i], final_decision);
    }
}

void codec_repetition_hard_decode8_neon(const int8_t *L8_N, uint8_t *V_K, size_t K, size_t n_reps){
    for(size_t i = 0; i < K; i += 16) {
        
        int8x16_t vote_acc = vdupq_n_s8(0);
        int8x16_t ones = vdupq_n_s8(1);
        
        for(size_t j = 0; j < n_reps; j++) {
            const int8_t* ptr = &L8_N[(j * K) + i];
            int8x16_t llr_vec = vld1q_s8(ptr);
            
            //mask : 0xFF (-1) si positif, 0x00 (0) si positif ou nul -1 0 0 -1 + 1 
            int8x16_t is_neg_mask = (int8x16_t)vcltzq_s8(llr_vec);
            //mask if negatif 1 if positif 0
            // mask + mask + 1 -> donne -1 ou +1 
            int8x16_t temp = vaddq_s8(is_neg_mask, is_neg_mask);
            int8x16_t votes = vaddq_s8(temp, ones);
            
            //on accumule les votes
            vote_acc = vqaddq_s8(vote_acc, votes);
        }
        
        //prise de la décision finale comme pour le soft
        int8x16_t decision_mask = (int8x16_t)vcltzq_s8(vote_acc);
        int8x16_t final_decision = vandq_s8(decision_mask, ones);
        
        vst1q_s8((int8_t*)&V_K[i], final_decision);
    }
}

void source_generate_all_zeros(uint8_t *U_K, size_t K){
  for (int i = 0; i < K; i++){
    U_K[i] = 0;
  }
}

void montecarlo_simulation( float m_arg, float M_arg, float s_arg, uint e_arg, uint K_arg, uint N_arg, std::string D_arg,const std::string &filename, bool mod_all_ones,size_t s_quant,size_t f_quant, int src_all_zeros){
	/*
	-m [min_SNR float] the first included Eb/N0 SNR to simulate (in dB),
	-M [max_SNR float] the last included Eb/N0 SNR to simulate (in dB),
	-s [step_val float] the constant step between two SNR points,
	-e [f_max uint] the number of frame errors to reach to explore one SNR point,
	-K [info_bits uint] the number of information bits,
	-N [codeword_size uint] the codeword size (has to be a multiple of K otherwise the program should return an error),
	-D ["rep-hard"|"rep-soft" string] select the decoder type.
	*/

	static size_t K = K_arg;
	size_t n_reps = N_arg / K_arg;
	
	uint8_t * U_K = (uint8_t *)calloc(K, sizeof(uint8_t));
	uint8_t * C_N = (uint8_t *)calloc((K * n_reps) , sizeof(uint8_t));
	uint8_t * V_K = (uint8_t *)calloc(K , sizeof(uint8_t));
	
	int32_t * X_N = (int32_t *)calloc((K * n_reps) , sizeof(int32_t));
	

	float * Y_N = (float *)calloc((K * n_reps) , sizeof(float));
	float * L_N = (float *)calloc((K * n_reps) , sizeof(float));
	int8_t * L8_N = (int8_t *)calloc((K * n_reps) , sizeof(int8_t));
	
	float sigma = 0.5f;
	float Ber = 0.0f;
	float Fer = 0.0f;
	uint64_t n_bit_errors = 0;
	uint64_t n_trames_errors = 0;
	float sim_thr = 0.0f;
	// l'algo de monte carlo qui fait le lancement en boucle du programme
	int nb_erreurs, nb_bits_erreurs, nb_simulation;

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
	for(float i = m_arg; i <= M_arg; i += s_arg){
		auto start_snr = std::chrono::high_resolution_clock::now(); //debut mesure
		nb_bits_erreurs = 0;
		nb_simulation = 0;
		nb_erreurs = 0;

		float snr_symb = i + 10*log10f((float)K/N_arg); //sinon ça donne 0 et ça fait bugger tout le programme
		sigma = sqrt(1/(2 * powf(10, snr_symb/10)));


		while(nb_erreurs < e_arg){
			nb_simulation++;
			n_bit_errors = 0;
			n_trames_errors = 0;

			if(!mod_all_ones){
				#ifdef ENABLE_STATS
				auto source_start = std::chrono::high_resolution_clock::now();
				#endif
				if (src_all_zeros) source_generate_all_zeros(U_K, K);
      			else source_generate(U_K, K);
				#ifdef ENABLE_STATS
				auto source_end = std::chrono::high_resolution_clock::now();
				record_block(BLOCK_SOURCE, source_start, source_end);
				#endif

				#ifdef ENABLE_STATS
				auto encoder_start = std::chrono::high_resolution_clock::now();
				#endif
				codec_repetition_encode(U_K,C_N,K,n_reps);
				#ifdef ENABLE_STATS
				auto encoder_end = std::chrono::high_resolution_clock::now();
				record_block(BLOCK_ENCODER, encoder_start, encoder_end);
				#endif

				#ifdef ENABLE_STATS
				auto modulator_start = std::chrono::high_resolution_clock::now();
				#endif
				modem_BPSK_modulate(C_N,X_N,n_reps * K);
				#ifdef ENABLE_STATS
				auto modulator_end = std::chrono::high_resolution_clock::now();
				record_block(BLOCK_MODULATOR, modulator_start, modulator_end);
				#endif
			}else{
				#ifdef ENABLE_STATS
				auto modulator_start = std::chrono::high_resolution_clock::now();
				#endif
				modem_BPSK_modulate_all_ones(C_N,X_N, n_reps * K);
				#ifdef ENABLE_STATS
				auto modulator_end = std::chrono::high_resolution_clock::now();
				record_block(BLOCK_MODULATOR, modulator_start, modulator_end);
				#endif
			}

			#ifdef ENABLE_STATS
			auto channel_start = std::chrono::high_resolution_clock::now();
			#endif
			channel_AWGN_add_noise(X_N,Y_N,K*n_reps,sigma);
			#ifdef ENABLE_STATS
			auto channel_end = std::chrono::high_resolution_clock::now();
			record_block(BLOCK_CHANNEL, channel_start, channel_end);
			#endif

			#ifdef ENABLE_STATS
			auto demodulator_start = std::chrono::high_resolution_clock::now();
			#endif
			modem_BPSK_demodulate(Y_N,L_N,K*n_reps,sigma);
			#ifdef ENABLE_STATS
			auto demodulator_end = std::chrono::high_resolution_clock::now();
			record_block(BLOCK_DEMODULATOR, demodulator_start, demodulator_end);
			#endif

			if(D_arg == "rep-hard"){
				codec_repetition_hard_decode(L_N,V_K,K,n_reps);
			}else if(D_arg == "rep-soft"){
				codec_repetition_soft_decode(L_N,V_K,K,n_reps);
			}else if(D_arg == "rep-hard8-neon"){
			    quantizer_transform8(L_N, L8_N, K*n_reps, s_quant, f_quant);
			    codec_repetition_hard_decode8_neon(L8_N, V_K, K, n_reps);
			}else if(D_arg == "rep-soft8-neon"){
			    quantizer_transform8(L_N, L8_N, K*n_reps, s_quant, f_quant);
			    codec_repetition_soft_decode8_neon(L8_N, V_K, K, n_reps);
			}else if(D_arg == "rep-hard8"){
				quantizer_transform8(L_N, L8_N, K*n_reps, s_quant, f_quant);
				codec_repetition_hard_decode8(L8_N,V_K,K,n_reps);
			}else{
				//sinon c'est rep-soft8
				quantizer_transform8(L_N, L8_N, K*n_reps, s_quant, f_quant);
				codec_repetition_soft_decode8(L8_N,V_K,K,n_reps);
			}

			#ifdef ENABLE_STATS
			auto monitor_start = std::chrono::high_resolution_clock::now();
			#endif
			monitor_check_errors(U_K,V_K,K,&n_bit_errors,&n_trames_errors);
			#ifdef ENABLE_STATS
			auto monitor_end = std::chrono::high_resolution_clock::now();
			record_block(BLOCK_MONITOR, monitor_start, monitor_end);
			#endif

			//if(n_trames_errors > 0){
			nb_erreurs+= n_trames_errors;
			//}
			//if(n_bit_errors > 0){
			nb_bits_erreurs += n_bit_errors;
			//}
			
		}
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
			(double)nb_simulation * (double)K,
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)(K * n_reps),
			(double)nb_simulation * (double)K
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
	free(U_K);
	free(C_N);
	free(V_K);
	free(X_N);
	free(L_N);
	free(Y_N);
	free(L8_N);
}


