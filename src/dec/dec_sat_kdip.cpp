

#include "dec/dec_sat.h"

namespace dec_ns {

using namespace utl;

void dec_sat_kdip::build_mitter(const circuit& cir) {

	circuit kupl_cir = cir;
	uint keysize = cir.nKeys();

	//kupl_cir.write_verilog();

	kupl_cir.add_circuit(OPT_OUT + OPT_INP, cir, "$0");
	iovecckt = kupl_cir;

	// use special char $ to avoid naming aliases with _num
	iovecckt.add_circuit(OPT_OUT + OPT_INP, kupl_cir, "$1");

	//std::cout << "\n\n IOVECT circuit for kdip\n\n";
	//iovecckt.write_verilog();

	id kdip_mitt = build_diff_ckt(kupl_cir, kupl_cir, mitt_ckt, OPT_INP);

	// getting the 4 subkeys
	idvec keyvec[4];
	idvec tmp(mitt_ckt.keys().begin(),
			mitt_ckt.keys().end());
	for (uint i = 0; i < 4; i++) {
		keyvec[i] = idvec(tmp.begin() + i*keysize,
				tmp.begin() + (i+1)*keysize);
	}

	// creating inequality between (k0, k2), and (k1, k3)
	id key_ineq_0 =
			equality_comparator(keyvec[0], keyvec[2], mitt_ckt, "0");
	id key_ineq_1 =
			equality_comparator(keyvec[1], keyvec[3], mitt_ckt, "1");

	mitt_out = mitt_ckt.join_outputs({key_ineq_0, key_ineq_1, kdip_mitt}, fnct::AND);

	//	std::cout << "\n\n MITTER circuit \n\n";
	//	mitt_ckt.write_verilog();

	// Add the mitter circuit to the SAT
	add_to_sat(Fi, mitt_ckt, mitt_maps);

	mitt_tip = mitt_maps.at(mitt_out);

	// duplicate-circuit-key-names to ID mapping
	for (auto kid : mitt_ckt.keys()) {
		// std::cout << mitt_ckt.getwire_name(x) << "\n";
		mitt_keynames_map[mitt_ckt.wname(kid)] = kid;
	}

}


void dec_sat_kdip::solve() {

	//duplicate circuit for DI
	_init_handlers();
	_prepare_sat_attack();

	//initialization phase
	int iter = 0;

	//Decamouflage Loop
	while (true) {

		iter++;
		print_stats(iter);

		iopair_t dp;
		int ret = solve_for_dip(dp);

		//Record observations in SAT
		if (ret == 0) {
			break;
		}

		dp.y = query_oracle(dp.x);
		create_ioconstraint(dp, iovecckt);

		// check deadline
	}

	// some randomization for better simulation resutls
	Fi.reseed_solver();

	//Extract key from SAT
	solve_key();
	return;
}


}
