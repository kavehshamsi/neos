/*
 * dec_stat.h
 *
 *  Created on: Jul 9, 2020
 *      Author: kaveh
 */

#ifndef SRC_DEC_DEC_OPT_H_
#define SRC_DEC_DEC_OPT_H_

#include "dec/dec.h"
#include "stat/stat.h"

namespace dec_ns {

using namespace stat_ns;

struct dec_opt_options {
	enum bbo_scheme_t {HILL, SA, EXP} bboscheme = HILL;
	enum prob_method_t {PATTERNS, PROPRULES} method = PATTERNS;
	uint num_sim_patterns = 1000;
	uint max_sim_depth = 1000;
	uint num_enc_in_patts = 1000;
	uint num_enc_key_patts = 1000;
	uint batch_size = 10;

	stat_ns::real bpalpha = 0.1;
	stat_ns::real learn_rate = 0.1;

	int time_deadline = -1;
};


class dec_opt : public dec {

protected:
	idvec worder;

public:
	dec_opt_options opt;

public:
	// constructor performs important initializiation
	dec_opt() {}
	dec_opt(const circuit& sim_cir, const circuit& enc_cir, const boolvec& corrkey);

	static void get_opt_parser(dec_opt& dc, boost::program_options::options_description& op);
	static void append_opt_parser(dec_opt& dc, boost::program_options::options_description& op);
	int parse_args(int argc, char** argv);
	void print_usage_message();

	void solve();

	void hill_climb();
	void dec_bbo();
	void dec_backprob(uint num_patts, uint epochs, uint batch_size, double alpha);
	void opt_dec();

	double get_metric(const boolvec& key, bool print_output = false);

	// find key value that increases activity
	static void get_active_key(const circuit& cir, boolvec& key);


protected:
	// internal helper methods
	void _measure_sim_prob(id2realmap& simprobMap);
	static void _update_key(const boolvec& key, boolvec& newKey, double mt);
	void _get_prob(const circuit& ckt, const id2boolmap& fixedInputs, id2realmap& encprobMap);
	static double _get_activity(const circuit& cir, const boolvec& key);


public:
	static void hill_climb(const circuit& sim_ckt, const circuit& enc_ckt, uint num_enc_ins, int time_deadline = -1);
	static void opt_attack(const circuit& sim_ckt, const circuit& enc_ckt,
			const vector<string>& args, int timeout_minutes);


};

namespace global {
	extern dec_opt* stat_obj_ptr;
}

} // namespace dec_ns

#endif /* SRC_DEC_DEC_OPT_H_ */
