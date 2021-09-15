
/*
 * main.h
 *
 *  Created on: Nov 8, 2016
 *      Author: kaveh
 */
#ifndef SRC_MAIN_MAIN_H_
#define SRC_MAIN_MAIN_H_

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>

// #include <getopt.h>
#include "main/main_data.h"
#include "enc/enc.h"
#include "base/circuit.h"

//using namespace std;

namespace mnf {

using namespace ckt;
using namespace enc;
using namespace utl;

boost::program_options::options_description get_top_options(main_data& mn);
int parse_main_options(main_data& mn, int argc, char** argv);

int dec_main(int argc, char** argv);
int enc_main(const main_data& mn);
int equiv_main(const main_data& mn);
int pstats_main(const main_data& mn);
void print_key(std::vector<bool>& key);
int misc_main(int argc, char** argv);

boolvec main_get_keyvec (const std::vector<std::string>& pos_args, int defsize, int width);
boolvec parse_key_arg(const std::string& arg);
string parse_simbin_arg(const std::string& arg);

int main_dec_seq(int argc, char** argv);
int main_dec_comb(int argc, char** argv);

void main_enc_antisat (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_sfll_point (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_xor (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_xorf (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_xorprob (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_lut (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_shfk (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_manymux (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_manymuxcg (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_and (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_interconnect(const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_wirecost (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_degree (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_univ (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_xmux (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_crosslock (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_cyclic (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_cyclic2 (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);
void main_enc_parity (const std::vector<std::string>& pos_args, circuit& enc_ckt, boolvec& corrkey);

}

#endif /* MAIN_H_ */
