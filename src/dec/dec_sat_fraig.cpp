

#include "dec/dec_sat.h"
#include "sat/prove.h"
#include "main/main_util.h"

namespace dec_ns {

using namespace prove;
using namespace utl;

int dec_sat_fraig::parse_args(int argc, char** argv) {

	namespace po = boost::program_options;
	po::options_description op;
	dec::get_opt_parser(*this, op);

	if (mnf::parse_args(argc, argv, op) || print_help) {
		std::cout << "usage: neos -d frg <sim_cir/enc_cir> <enc_cir/corrkey> [iterations]\n";
		return 1;
	}

	decol::read_sim_enc_key_args(pos_args, !oracle_binary.empty());

	if (pos_args.size() >= 1) {
    	iteration_limit = stoi(pos_args[0]);
	}

    return 0;
}


void dec_sat_fraig::solve() {

	_prepare_sat_attack();

	fi_aig = mitt_aig;
	fi_out = mitt_out;

	for ( auto oid : utl::to_vec(fi_aig.outputs()) ) {
		if (oid != fi_out) {
			fi_aig.remove_node(oid);
		}
	}


	//SAT attack Loop
	for (iteration = 1; ; iteration++) {
		print_stats(iteration);

		iopair_t dp;

		if (solve_for_dip(dp) == sl_False) {
			break;
		}

		dp.y = query_oracle(dp.x);

		//Record observations in SAT
		add_ioconstraint(dp);

		// check iteration cap
		if (iteration_limit != -1
				&& iteration >= iteration_limit)
			break;
	}

	//Extract key from SAT
	solve_key();
	return;

}

void dec_sat_fraig::add_ioconstraint(iopair_t& dp) {
	circuit kccir;
	_get_kc_ckt_mult(dp, kccir, iovecckt, 2);

	aig_t kcaig = kccir;
	id kcaigout = *kcaig.outputs().begin();

	id2alitmap conMap;
	fi_aig.add_aig(kcaig, conMap);

	id2alitmap kcconMap;
	kc_aig.add_aig(kcaig, kcconMap);

	id new_iovec_out = conMap.at(kcaigout).lid();
	assert(!conMap.at(kcaigout).sgn());
	//std::cout << "added output " << fi_aig.ndname(new_iovec_out) << "\n";
	fi_out = fi_aig.join_outputs(fnct::AND);
	// fi_aig.write_bench();
}


int dec_sat_fraig::solve_for_dip(iopair_t& dp) {

	id2boolHmap cex;
	dp.clear();

	aig_t fi_aig_simp;
	int status = prove_fraig0(fi_aig, fi_aig_simp, {fi_out}, cex);
	assert(status != sl_Undef);

	if (status == sl_False) {
		std::cout << "done with mitter loop\n";
		return sl_False;
	}

	id2boolmap cex_sim = cex;
	fi_aig.simulate_comb(cex_sim);
	std::cout << "out: " << cex_sim.at(fi_out) << "\n";

	dp.x.resize(num_ins);

	std::cout << "x=";
	for (auto xid : mitt_aig.nonkey_inputs()) {
		const string& nm = mitt_aig.ndname(xid);
		id fi_xid = fi_aig.find_wcheck(nm);
		assert(fi_xid != -1);
		// std::cout << fi_aig.ndname(fi_xid);
		bool sv = cex.at(fi_xid);
		dp.x[ewid2ind.at(enc_cir->find_wcheck(nm))] = sv;
		std::cout << sv;
	}
	std::cout << "\n";

	fi_aig = fi_aig_simp;

	return sl_True;
}

void dec_sat_fraig::solve_key() {
	std::cout << "solving for key\n";
	id kc_out = kc_aig.join_outputs(fnct::AND, "kcout");
	id2boolHmap keyMap;
	int status = prove_fraig0(kc_aig, {kc_out}, keyMap);
	if (status == sl_False) {
		neos_error("key-condition not satisfiable\n");
	}

	boolvec key;
	for (auto ekid : enc_cir->keys()) {
		id kid = kc_aig.find_node(enc_cir->wname(ekid));
		if (kid != -1)
			key.push_back(keyMap.at(kid));
		else
			key.push_back(0);
	}

	std::cout << "key=" << utl::to_delstr(key) << "\n";
	check_key(key);
}

}
