#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>

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
	for(int i = 0; i < K;++i){
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

int main(void){
	size_t K = 4;
	size_t n_reps = 3;
	uint8_t * U_K = (uint8_t *)calloc(K, sizeof(uint8_t));
	uint8_t * C_N = (uint8_t *)calloc((K * n_reps) , sizeof(uint8_t));
	int32_t * X_N = (int32_t *)calloc((K * n_reps) , sizeof(int32_t));
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


	free(U_K);
	free(C_N);
	free(X_N);
	return 0;
}
