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