#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <ctime>


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
	if(sizeof(U_K)<K){
		perror("k trop grand");
		return;
	}
	uint8_t b = 0;
	srand(time(NULL));
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
	for(int i = 0; i<K*n_reps;i++){
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
	for(int i = 0; i < N; i++){
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
	std::normal_distribution<double> distribution(0.0, sigma);
	std::default_random_engine generator;
	for(size_t i = 0; i < N; i++){
		Y_N[i] = (float)X_N[i] + distribution(generator);
	}
}
// demodulator, just copies Y_N in L_N for now il converti les symboles en LLR
void modem_BPSK_demodulate(const float *Y_N, float *L_N, size_t N, float sigma){
	/*
	LLR = log10((P(b=0|y))/(P(b=1|y)))
	positif = 0
	negatif = -1
	pour l'instant on va juste copire Y_N dans L_N
	*/
	//on dit que N est egale a la taille des deux tableaux
	for(int i = 0; i < N;i++){
		L_N[i] = Y_N[i]; // *sigma ? pour l'instant je sait pas
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
			std::cout << (j*K)+i << "eme element = " << L_N[(j*K)+i] << std::endl;
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
			std::cout << (j*K)+i << "eme element = " << L_N[(j*K)+i] << std::endl;//debut
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
	std::cout <<"il y a : " << *n_bit_errors << " erreurs et "<< *n_frame_errors << " trames fausses "<< std::endl;
}



#include <chrono>
int main(void){
	auto start = std::chrono::high_resolution_clock::now();
	
	size_t K = 4;
	size_t n_reps = 3;
	
	uint8_t * U_K = (uint8_t *)calloc(K, sizeof(uint8_t));
	uint8_t * C_N = (uint8_t *)calloc((K * n_reps) , sizeof(uint8_t));
	uint8_t * V_K = (uint8_t *)calloc(K , sizeof(uint8_t));
	
	int32_t * X_N = (int32_t *)calloc((K * n_reps) , sizeof(int32_t));
	
	uint64_t n_bit_errors = 0;
	uint64_t n_trames_errors = 0;

	float * Y_N = (float *)calloc((K * n_reps) , sizeof(float));
	float * L_N = (float *)calloc((K * n_reps) , sizeof(float));
	
	float sigma = 0.5f;

	printf("U_K avant :\n");
	for(int i = 0; i < K; i++){
		printf("%d",U_K[i]);
	}

	printf("\n");
	
	source_generate(U_K,K);
	
	printf("U_K après :\n");
	for(int i = 0; i < K; i++){
		printf("%d",U_K[i]);
	}
	
	printf("\n");
	printf("C_N avant :\n");
	for(int i = 0; i < K * n_reps; i++){
		printf("%d",C_N[i]);
	}
	codec_repetition_encode(U_K,C_N,K,n_reps);

	printf("\n");
	printf("C_N après :\n");
	for(int i = 0; i < K * n_reps; i++){
		//std::cout << U_K[i] << std::endl;
		printf("%d",C_N[i]);
	}
	
	printf("\n");
	printf("X_N avant :\n");
	for(int i = 0; i < K * n_reps; i++){
		printf("%d",X_N[i]);
	}
	modem_BPSK_modulate(C_N,X_N,n_reps * K);

	printf("\n");
	printf("X_N après :\n");
	for(int i = 0; i < K * n_reps; i++){
		printf("%d",X_N[i]);
	}
	printf("\n");

	channel_AWGN_add_noise(X_N,Y_N,K*n_reps,sigma);
	printf("Y_N après :\n");
	for(int i = 0; i < K * n_reps; i++){
		printf("%f",Y_N[i]);
	}

	printf("\n");

	modem_BPSK_demodulate(Y_N,L_N,K*n_reps,sigma);
	printf("V_N après :\n");
	for(int i = 0; i < K * n_reps; i++){
		printf("%f",L_N[i]);
	}

	printf("\n");


	codec_repetition_hard_decode(L_N,V_K,K,n_reps);
	monitor_check_errors(U_K,V_K,K,&n_bit_errors,&n_trames_errors);
	codec_repetition_soft_decode(L_N,V_K,K,n_reps);
	free(U_K);
	free(C_N);
	free(V_K);
	free(X_N);
	free(L_N);
	free(Y_N);
	auto fin = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(fin - start);
	std::cout << "ça prend " << duration.count() << " ms" << std::endl;
	return 0;
}
