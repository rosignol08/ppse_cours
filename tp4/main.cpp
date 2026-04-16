#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <ctime>
#include <chrono>

#include <unistd.h>
#include <fstream>


/**
 * @brief Simule la source d'information. Elle doit remplir un tableau de bits aléatoires.
 *
 * La fonction genère K bits aléatoires (0 ou 1) et les stoques dans le tampon U_K.
 * Sert de message source dans le systeme de communication.
 *
 * @param U_K Pointeur vers un tampon/buffer où les bits générés vont etre stoqués.
 *            le tampons doit avoir une allocation d'au moin K bytes.
 * @param K   Le nombre de bits d'information à generer (la taille du message).
 *
 * @note Cette fonction utilise un generateur pseudo aléatoire (PRNG) pour generer des bits.
 *       rand() % 2 sera utilisé ici.
 *
 * @return void
 */
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

// read from the buffer U_K and write into the buffer C_N
/**
 * @brief code à repetition.
 *
 * on repete l'envoie du message
 *
 * @param U_K Pointeur vers un tampon/buffer où les bits du message sont stoqués.
 * @param n_reps   Le nombre de repetition du message(le nombre de fois qu'on copie U_K dans C_N).
 *
 * @return void
 */
void codec_repetition_encode(const uint8_t *U_K, uint8_t *C_N, size_t K, size_t n_reps){
	for(size_t i = 0; i<K*n_reps;i++){
		C_N[i] = U_K[i%K];
	}
	return;
}

// read from C_N, write into X_N
/**
 * @brief change les 0 en +1 et 1 en -1.
 *
 * on prépare les valeurs symboliques. 
 * Le BPSK (Binary Phase-Shift Keying) transforme les bits en niveaux de tension.
 *
 * @param C_K Pointeur vers un tampon/buffer où les bits du message sont stoqués.
 * @param X_N Le resultat de la transformation.
 * @param N taille de C_K
 *
 * @return void
 */
void modem_BPSK_modulate(const uint8_t *C_N, int32_t *X_N, size_t N){
	for(size_t i = 0; i < N; i++){
		if(C_N[i] == 0){
			X_N[i] = 1;
		}else{
			X_N[i] = -1;
		}
	}
}


/* add white Gaussian noise
cette fonction prend des X et renvoie des Y = X+N N etant le bruit
*/
void channel_AWGN_add_noise(const int32_t *X_N, float *Y_N, size_t N, float sigma){
	static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
	std::normal_distribution<double> distribution(0.0, sigma);
	//std::default_random_engine generator;
	for(size_t i = 0; i < N; i++){
		Y_N[i] = (float)X_N[i] + distribution(generator);
	}
}
// demodulator, convertit les symboles reçus en LLR
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

// hard decoder: first hard decides each LLR and then makes a majority vote
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
// soft decoder: computes the mean of each LLR to hard decide the bits
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


// update `n_bit_errors` and `n_frame_errors` variables depending on `U_K` and `V_K`

/*
Il calcule le nombre de trames erronée notée Fe
permet de définir le taux d’erreurs trames c’est sur le nombre totales de trames transmisses le nombre de trames erronées.
Be = bit erronée
Fn le nombre de trames tout cours.
FER = Fe/Fn
Bit error rate = Be/ (nombre totales de trames transmises Fn * K)
(K nombre de bits d’information)

*/
//n_bit_errorsça fait K taille et n_frame_errors ça fait n_reps taille
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

//faut une fonction pour ajouter les resultats dans un fichier csv
//j'ai trouvé ça sur internet
void append_result(const std::string &filename, float eb_n0, float es_n0, float sigma, int be, int fe, int fn, float ber, float fer, double sim_time, double time_per_frame, float debit) {
    // std::ios::app permet d'ajouter à la fin du fichier sans l'écraser
    std::ofstream file(filename, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier '" << filename << "'.\n";
        return;
    }
    
    //valeurs ecrites dans le csv
    file << eb_n0 << "," << es_n0 << "," << sigma << "," 
        << be << "," << fe << "," << fn << "," 
        << ber << "," << fer << "," << sim_time << "," << time_per_frame << "," << debit << "\n";

    file.close();
}


void montecarlo_simulation( float m_arg, float M_arg, float s_arg, uint e_arg, uint K_arg, uint N_arg, std::string D_arg,const std::string &filename ){
	/*
	-m [min_SNR float] the first included Eb/N0 SNR to simulate (in dB),
	-M [max_SNR float] the last included Eb/N0 SNR to simulate (in dB),
	-s [step_val float] the constant step between two SNR points,
	-e [f_max uint] the number of frame errors to reach to explore one SNR point,
	-K [info_bits uint] the number of information bits,
	-N [codeword_size uint] the codeword size (has to be a multiple of K otherwise the program should return an error),
	-D ["rep-hard"|"rep-soft" string] select the decoder type.
	*/

	size_t K = K_arg;
	size_t n_reps = N_arg / K_arg;
	
	uint8_t * U_K = (uint8_t *)calloc(K, sizeof(uint8_t));
	uint8_t * C_N = (uint8_t *)calloc((K * n_reps) , sizeof(uint8_t));
	uint8_t * V_K = (uint8_t *)calloc(K , sizeof(uint8_t));
	
	int32_t * X_N = (int32_t *)calloc((K * n_reps) , sizeof(int32_t));
	

	float * Y_N = (float *)calloc((K * n_reps) , sizeof(float));
	float * L_N = (float *)calloc((K * n_reps) , sizeof(float));
	
	float sigma = 0.5f;
	float Ber = 0.0f;
	float Fer = 0.0f;
	uint64_t n_bit_errors = 0;
	uint64_t n_trames_errors = 0;

	// l'algo de monte carlo qui fait le lancement en boucle du programme
	int nb_erreurs, nb_bits_erreurs, nb_simulation;
	float debit = 0;
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
			source_generate(U_K,K);
			codec_repetition_encode(U_K,C_N,K,n_reps);
			modem_BPSK_modulate(C_N,X_N,n_reps * K);
			channel_AWGN_add_noise(X_N,Y_N,K*n_reps,sigma);
			modem_BPSK_demodulate(Y_N,L_N,K*n_reps,sigma);
			if(D_arg == "rep-hard"){
				codec_repetition_hard_decode(L_N,V_K,K,n_reps);
			}else{
				codec_repetition_soft_decode(L_N,V_K,K,n_reps);
			}
			monitor_check_errors(U_K,V_K,K,&n_bit_errors,&n_trames_errors);
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
		
		std::cout << "SNR : " << i << " | Ber : " << Ber << " | Fer : " << Fer << " | Trames simulees : " << nb_simulation << std::endl;

		auto end_snr = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end_snr - start_snr;
		debit = (nb_simulation * K) / diff.count();//debit
        double sim_time = diff.count(); //temps total pour ce SNR (en secondes)
        double time_per_frame = sim_time / nb_simulation; //temps moyen par trame

		//ça ajoute les résultats dans le fichier à chaque itération
		append_result(filename, i, snr_symb, sigma, nb_bits_erreurs, nb_erreurs, nb_simulation, Ber, Fer, sim_time, time_per_frame, debit);
	}
	free(U_K);
	free(C_N);
	free(V_K);
	free(X_N);
	free(L_N);
	free(Y_N);
}


int main(int argc, char* argv[]){
	int opt;
	float m_arg, M_arg, s_arg;
	uint e_arg, K_arg, N_arg;
	std::string D_arg;

	std::string output_filename = "results.csv"; //si pas d'arguments

	while ((opt = getopt(argc, argv, "m:M:s:e:K:N:D:o:")) != -1) {
		switch (opt) {
		case 'm':
			m_arg = atof(optarg);
			//std::cout << "la valeur c'est \n" << m_arg << std::endl;
			break;
		case 'M':
			//std::cout << "Option -b selected with value: " << optarg << "\n";
			M_arg = atof(optarg);
			break;
		case 's':
			//std::cout << "Option -c selected\n";
			s_arg = atof(optarg);
			break;
		case 'e':
			//std::cout << "Option -c selected\n";
			e_arg = atoi(optarg);
			break;
		case 'K':
			//std::cout << "Option -c selected\n";
			K_arg = atoi(optarg);
			break;
		case 'N':
			//std::cout << "Option -c selected\n";
			N_arg = atoi(optarg);
			break;
		case 'D':
			//std::cout << "Option -c selected\n";
			D_arg = optarg;
			break;
		case 'o':
		 	output_filename = optarg;
			break;
		case '?':
			std::cerr << "Unknown option: " << static_cast<char>(optopt) << "\n";
			break;
		}
	}
	
	srand(time(NULL));

	auto start = std::chrono::high_resolution_clock::now();
	

	//std::string output_filename = "results.csv";

	std::ofstream clear_file(output_filename);
	clear_file << "Eb/N0,Es/N0,sigma,be,fe,fn,BER,FER,sim_time_s,time_per_frame_s\n";
	clear_file.close();
	
	montecarlo_simulation(m_arg, M_arg, s_arg, e_arg, K_arg, N_arg, D_arg, output_filename);
	auto fin = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(fin - start);
	std::cout << "ça a pris " << duration.count() << " ms" << std::endl;
	return 0;
}
	//printf("U_K avant :\n");
	//for(size_t i = 0; i < K; i++){
	//	printf("%d",U_K[i]);
	//}

	//printf("\n");
	
	//source_generate(U_K,K);
	
	//printf("U_K après :\n");
	//for(size_t i = 0; i < K; i++){
	//	printf("%d",U_K[i]);
	//}
	
	//printf("\n");
	//printf("C_N avant :\n");
	//for(size_t i = 0; i < K * n_reps; i++){
	//	printf("%d",C_N[i]);
	//}
	//codec_repetition_encode(U_K,C_N,K,n_reps);

	//printf("\n");
	//printf("C_N après :\n");
	//for(size_t i = 0; i < K * n_reps; i++){
	//	//std::cout << U_K[i] << std::endl;
	//	printf("%d",C_N[i]);
	//}
	
	//printf("\n");
	//printf("X_N avant :\n");
	//for(size_t i = 0; i < K * n_reps; i++){
	//	printf("%d",X_N[i]);
	//}
	//modem_BPSK_modulate(C_N,X_N,n_reps * K);

	//printf("\n");
	//printf("X_N après :\n");
	//for(size_t i = 0; i < K * n_reps; i++){
	//	printf("%d",X_N[i]);
	//}
	//printf("\n");

	//channel_AWGN_add_noise(X_N,Y_N,K*n_reps,sigma);

	//printf("Y_N après :\n");
	//for(size_t i = 0; i < K * n_reps; i++){
	//	printf("%f",Y_N[i]);
	//}

	//printf("\n");

	//modem_BPSK_demodulate(Y_N,L_N,K*n_reps,sigma);
	
	//printf("V_N après :\n");
	//for(size_t i = 0; i < K * n_reps; i++){
	//	printf("%f",L_N[i]);
	//}

	//printf("\n");


	//codec_repetition_hard_decode(L_N,V_K,K,n_reps);
	//printf("check des erreurs de hard decode \n");
	//monitor_check_errors(U_K,V_K,K,&n_bit_errors,&n_trames_errors);
	//printf("\n");
	//codec_repetition_soft_decode(L_N,V_K,K,n_reps);
	//printf("check des erreurs de soft decode \n");
	//monitor_check_errors(U_K,V_K,K,&n_bit_errors,&n_trames_errors);
	//free(U_K);
	//free(C_N);
	//free(V_K);
	//free(X_N);
	//free(L_N);
	//free(Y_N);
	
