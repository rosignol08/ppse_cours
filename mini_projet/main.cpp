#include "fonctions.h"

int main(int argc, char* argv[]){
	int opt,src_all_zeros=0;
	float m_arg = 0, M_arg = 0, s_arg = 1;
	uint e_arg = 100, K_arg = 32, N_arg = 128;
	std::string D_arg = "rep-hard";
	
	std::string output_filename = "results.csv"; //si pas d'arguments
	bool mod_all_ones= false;
	size_t f_quant = 0;
    size_t s_quant = 8; // Valeur par défaut demandée
    bool use_quantizer = false;
	
	//pour les options longues ("nom", a_un_argument?, flag, code_de_retour)
    struct option long_options[] = {
        {"mod-all-ones", no_argument,       0, 'O'}, // Renvoie 'O'
        {"qf",           required_argument, 0, 'F'}, // Renvoie 'F'
        {"qs",           required_argument, 0, 'S'}, // Renvoie 'S'
        {0, 0, 0, 0} // Toujours terminer par des zéros
    };

	int option_index = 0;

	while ((opt = getopt_long(argc, argv, "m:M:s:e:K:N:D:o:", long_options, &option_index)) != -1) {
		switch (opt) {
		case 'm':
			m_arg = atof(optarg);
			break;
		case 'M':
			M_arg = atof(optarg);
			break;
		case 's':
			s_arg = atof(optarg);
			break;
		case 'e':
			e_arg = atoi(optarg);
			break;
		case 'K':
			K_arg = atoi(optarg);
			break;
		case 'N':
			N_arg = atoi(optarg);
			break;
		case 'D':
			D_arg = optarg;
			break;
		case 'o':
		 	output_filename = optarg;
			break;
		case 'O': 
            //si getopt_long renvoie 'O' alors --mod-all-ones est présent
            mod_all_ones = true; 
            break;
        case 'F': 
            //si getopt_long renvoie 'F' alors --qf est présent
            f_quant = atoi(optarg); 
            use_quantizer = true; //on doit utiliser la quantification
            break;
        case 'S': 
            //si getopt_long renvoie 'S' alors --qs est présent
            s_quant = atoi(optarg); 
            break;
        case '?':
            //getopt_long affiche une erreur par défaut
            break;
        }
    }
	
	srand(time(NULL));

	auto start = std::chrono::high_resolution_clock::now();
	

	//std::string output_filename = "results.csv";

	std::ofstream clear_file(output_filename);
	clear_file << "Eb/N0,Es/N0,sigma,be,fe,fn,BER,FER,sim_time_s,time_per_frame_s,Sim_thr_Mbps\n";
	clear_file.close();
	
	montecarlo_simulation(m_arg, M_arg, s_arg, e_arg, K_arg, N_arg, D_arg, output_filename,mod_all_ones,s_quant,f_quant, src_all_zeros);
	auto fin = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(fin - start);
	std::cout << "ça a pris " << duration.count() << " ms" << std::endl;
	return 0;
}