/*
 * bboptimize.h
 *
 *  Created on: Jan 14, 2021
 *      Author: kaveh
 */

#ifndef SRC_UTL_BBOPTIMIZE_H_
#define SRC_UTL_BBOPTIMIZE_H_

#include <vector>
#include <functional>
#include "utl/omath.h"
#include <map>

namespace bbopt {

using std::vector;
typedef vector<bool> boolvec;

class bb_opt_boolvec {
public:
	typedef std::function<float(const boolvec&)> cost_fun_t;
	cost_fun_t costfn;

	double cur_improve = 0, last_improve = 0, improve_avg = 0, best_cost = 0;

	double cost_scale = 2;

	bool noisy_cost_function = true;

	boolvec xt, xt_cand;

	int time_deadline = -1;

	bool allow_interrupt = true;
	bool interrupted = false;

	utl::math::avgstdev_t costavg;

	struct weight_constraint_t {
		vector<int> positions;
		int cur_weight;
		int min_weight;
		int max_weight;
	};

	vector<weight_constraint_t> wcvec;
	vector<int> pos2wc;

	bb_opt_boolvec(const cost_fun_t& costfn, const boolvec& initial_x);
	~bb_opt_boolvec();
	void add_weight_constraint(const vector<int>& positions, int min_weight, int max_weight);

	void optimize(int32_t max_i_iterations = -1, int32_t max_c_iterations = -1);

	void random_flip_bits(boolvec& cand_x, int num_flips);
	void suggest_candidate(const boolvec& best_x, boolvec& cand_x, float best_cost);
	void update_cur_weights(const boolvec& wrt);
	void force_to_constraint(boolvec& cand_x);

	void setup_signal();
};

class bb_opt_boolvec_sa : public bb_opt_boolvec {
public:
	uint iter = 0;

	bb_opt_boolvec_sa(const cost_fun_t& costfn, const boolvec& initial_x) : bb_opt_boolvec(costfn, initial_x) {}

	void optimize(int32_t max_iter = -1);

	virtual ~bb_opt_boolvec_sa() {}
};

class bb_opt_boolvec_exp : public bb_opt_boolvec {
public:
	uint iter = 0;

	struct node_t {
		boolvec xt;
		double cost;
		vector<size_t> unexplored;
	};

	vector<node_t> trace;

	bb_opt_boolvec_exp(const cost_fun_t& costfn, const boolvec& initial_x) : bb_opt_boolvec(costfn, initial_x) {}

	void optimize(int32_t max_iter = -1);
};

class bb_opt_boolvec_exp_fifo : public bb_opt_boolvec {
public:
	uint iter = 0;

	struct node_t {
		boolvec xt;
		double cost;
		vector<size_t> unexplored;
	};

	std::map<double, node_t> trace;

	bb_opt_boolvec_exp_fifo(const cost_fun_t& costfn, const boolvec& initial_x) : bb_opt_boolvec(costfn, initial_x) {}

	void optimize(int32_t max_iter = -1);
};


class bb_opt_boolvec_bf : public bb_opt_boolvec {
public:
	uint iter = 0;

	bb_opt_boolvec_bf(const cost_fun_t& costfn, const boolvec& initial_x) : bb_opt_boolvec(costfn, initial_x) {}

	void optimize(int32_t max_iter = -1);
};


} // namespace bbopt

#endif /* SRC_UTL_BBOPTIMIZE_H_ */
