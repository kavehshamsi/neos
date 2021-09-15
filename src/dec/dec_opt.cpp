/*
 * stat.cpp
 *
 *  Created on: Apr 23, 2018
 *      Author: kaveh
 */

#include <dec/dec_opt.h>
#include "dec/mcdec.h"
#include "stat/stat.h"
#include "misc/bboptimize.h"
#include "main/main_util.h"

namespace dec_ns {

using namespace stat_ns;

namespace global {
	dec_opt* stat_obj_ptr = nullptr;
}

dec_opt::dec_opt(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corrkey) : dec(sim_cir, enc_cir, corrkey) {
	opt.method = dec_opt_options::PATTERNS;
}

void dec_opt::get_opt_parser(dec_opt& dc, boost::program_options::options_description& op) {

	dec::get_opt_parser((dec&)dc, op);
	dec_opt::append_opt_parser(dc, op);

}

void dec_opt::append_opt_parser(dec_opt& dc, boost::program_options::options_description& op) {

}

void dec_opt::print_usage_message() {

	boost::program_options::options_description op;
	dec_opt::get_opt_parser(*this, op);
	std::cout << op << "\n";

}

int dec_opt::parse_args(int argc, char** argv) {

	boost::program_options::options_description op;
	dec_opt::get_opt_parser(*this, op);

	if ( mnf::parse_args(argc, argv, op) || print_help) {
		print_usage_message();
		return 1;
	}

	decol::read_sim_enc_key_args(pos_args, !oracle_binary.empty());

	std::string help_message =
	"comb usage: neos -d bbo <sim_cir/enc_cir> <enc_cir/corrkey> <strategy: hill, sa, exp> [num_input_patts] [iteration_limit]\n"
	"seq  usage: neos -d bbo <sim_cir/enc_cir> <enc_cir/corrkey> <strategy: hill, sa, exp> [num_input_patts] [max_depth] [iteration_limit]\n";


	if (pos_args.size() == 0) {
		std::cout << help_message << "\n";
		return 1;
	}

	string bbo_scheme_str = pos_args[0];
	pos_args.erase(pos_args.begin());

	if (pos_args.size() >= 1) {
		opt.num_enc_in_patts = stoi(pos_args[0]);
	}
	if (pos_args.size() >= 2) {
		if (is_sequential)
			opt.max_sim_depth = stoi(pos_args[1]);
		iteration_limit = stoi(pos_args[1]);
	}
	if (pos_args.size() >= 3) {
		if (is_sequential)
			iteration_limit = stoi(pos_args[2]);
		else {
			std::cout << help_message;
			return 1;
		}
	}

	if (bbo_scheme_str == "hill") {
		opt.bboscheme = dec_opt_options::HILL;
	}
	else if (bbo_scheme_str == "sa") {
		opt.bboscheme = dec_opt_options::SA;
	}
	else if (bbo_scheme_str == "exp") {
		opt.bboscheme = dec_opt_options::EXP;
	}
	else {
		std::cout << "unrecognized bbopt method: " << bbo_scheme_str << "\n";
		std::cout <<  help_message;
		return 1;
	}

    return 0;
}

void dec_opt::solve() {

	_init_handlers();

	// TODO: get rid of hill. integrate with bbo
	if (opt.bboscheme == dec_opt_options::HILL) {
		hill_climb();
	}
	else {
		dec_bbo();
	}

}

double dec_opt::get_metric(const boolvec& key, bool print_output) {
	return get_key_error(key, print_output, opt.num_enc_in_patts, opt.max_sim_depth);
}

void dec_opt::_update_key(const boolvec& key, boolvec& newKey, double mt) {

	boolvec curKey = key;
	double flip_prob = std::pow(2, (-8 * (1 - mt)));

	for( uint i = 0; i < key.size(); i++ ) {
		double rdoub = ((double)rand() / RAND_MAX);
		bool switchp = rdoub < flip_prob;

		if (switchp) {
			newKey[i] = !curKey[i];
		}
		else {
			newKey[i] = curKey[i];
		}
	}

}

void dec_opt::hill_climb(const circuit& sim_cir, const circuit& enc_cir, uint num_enc_patts, int time_deadline) {

	dec_opt st(sim_cir, enc_cir, boolvec());
	st.opt.num_enc_in_patts = num_enc_patts;
	st.opt.time_deadline = time_deadline;

	st.hill_climb();

}

void dec_opt::get_active_key(const circuit& cir, boolvec& key) {

	auto curkey = boolvec(cir.nKeys(), 0);
	boolvec candKey = curkey;

	double metric = 0;

	uint step = 0;
	std::cout << "\n";
	while (true) {

		_update_key(curkey, candKey, metric);
		//std::cout << "\r" << utl::_to_str(candKey) << std::flush;

		double new_metric = _get_activity(cir, candKey);

		if (new_metric > metric) {
			std::cout << "step " << step++ << " error: " << metric << "\n";
			metric = new_metric;
			curkey = candKey;
		}

		if (step++ > 100) {
			std::cout << "reached error goal\n";
			std::cout << "key=" << utl::to_str(curkey) << "\n";
			std::cout << "error: " << metric << "\n";
			key = curkey;
			return;
		}
	}
}


double dec_opt::_get_activity(const circuit& cir, const boolvec& key) {
	assert(cir.nKeys() == key.size());

	uint num_patts = 1000;
	id2boolHmap inputMap;
	id2boolmap simMap_new, simMap;

	uint hd = 0;

	for (uint n = 0; n < num_patts; n++) {

		for (auto xid : cir.inputs()) {
			bool r = (rand() % 2 == 0);
			inputMap[xid] = r;
		}

		// set keys accordingly
		int i = 0;
		for (auto kid : cir.keys())
			inputMap[kid] = key[i++];

		cir.simulate_comb(inputMap, simMap_new);

		if (n > 0) {
			for (auto oid : cir.outputs()) {
				if ( simMap_new.at(oid) != simMap.at(oid) ) {
					hd++;
				}
			}
		}

		simMap = simMap_new;
	}

	return ((double)hd / (double)((num_patts-1) * cir.nOutputs()));
}

struct err_cost_fn {
	dec& dc;
	uint num_patts = 100;
	uint max_depth = 30;
	id2idHmap enc2sim;

	err_cost_fn(dec& dc) : dc(dc) {}

	double operator() (const boolvec& candkey) {
		return dc.get_key_error(candkey, false, num_patts, max_depth);
	}
};


void dec_opt::dec_bbo() {

	err_cost_fn costfn(*this);
	costfn.num_patts = opt.num_enc_in_patts;
	//bbopt::bb_opt_boolvec_bf bbo(costfn, utl::random_boolvec(num_keys));
	std::shared_ptr<bbopt::bb_opt_boolvec> bboptr;
	if (opt.bboscheme == dec_opt_options::SA) {
		bboptr = std::make_shared<bbopt::bb_opt_boolvec_sa>(costfn, utl::random_boolvec(num_keys));
	}
	else if (opt.bboscheme == dec_opt_options::EXP) {
		bboptr = std::make_shared<bbopt::bb_opt_boolvec_exp>(costfn, utl::random_boolvec(num_keys));
	}
	else {
		neos_abort("unsupported bbo scheme");
	}

	bboptr->time_deadline = time_deadline;

	bboptr->optimize(iteration_limit);

	dec::check_key(bboptr->xt);
}


// TODO: what's this for?
void dec_opt::hill_climb() {

	global::stat_obj_ptr = this;

	interkey = boolvec(enc_cir->nKeys(), 0);
	foreach_gate(*enc_cir, g, gid) {
		if (g.gfunction() == fnct::TMUX) {
			interkey[ewid2ind.at(g.fanins[0])] = 1;
		}
	}
	boolvec candKey = interkey;

	auto start_time = utl::_start_wall_timer();
	double metric = std::numeric_limits<real>::max();

	uint step = 0, settled = 0;
	std::cout << "\n";
	while (true) {

		_update_key(interkey, candKey, metric);

		//std::cout << "key: " << to_str(candKey) << "\n";

		double new_metric = get_metric(candKey);

		std::cout << "key: " << to_str(candKey) << "  " << to_str(interkey) << "  ";

		if (new_metric < metric) {
			//std::cout << "\nstep " << step++ << " key-err: " << metric << "\n";
			metric = new_metric;
			_safe_set_interkey(candKey);
		}

		std::cout << "err : " << std::left << std::setw(4) << metric << "  " << std::setw(4) << new_metric << "\n";

		if (opt.time_deadline != -1 && utl::_stop_wall_timer(start_time) > (double)opt.time_deadline) {
			std::cout << "time out!\n";
			std::cout << "last key: " << utl::to_str(interkey) << "\n";
			std::cout << "error: " << get_metric(interkey, true) << "\n";
			return;
		}

		if (metric < 0.001 && settled++ == 5) {
			std::cout << "reached error goal\n";
			std::cout << "key=" << utl::to_str(interkey) << "\n";
			std::cout << "key-err: " << metric << "\n";
			boolvec checkkey;
			uint i = 0;
			for (auto kid : enc_cir->keys()) {
				checkkey.push_back(interkey[i++]);
			}

			dec_ns::dec::check_key(*sim_cir, *enc_cir, checkkey);
			break;
		}
	}
}


void dec_opt::_measure_sim_prob(id2realmap& simprobMap) {
	sig_prob_sim(*sim_cir, opt.num_sim_patterns, simprobMap);
}



} // namespace st


