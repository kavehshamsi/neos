/*
 * bboptimize.cpp
 *
 *  Created on: Jan 14, 2021
 *      Author: kaveh
 */

#include "misc/bboptimize.h"
#include <math.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <signal.h>
#include <utl/utility.h>

namespace bbopt {

void* global_bbo_obj_pointer = nullptr;

void bbo_signal_handler(int s) {
	assert(global_bbo_obj_pointer);
	std::cout << "optimization interrupted. Trying to terminate loop...\n";
	((bb_opt_boolvec*)global_bbo_obj_pointer)->interrupted = true;
}

void bbo_deadline_handler(int s) {
	assert(global_bbo_obj_pointer);
	std::cout << "time out. Trying to terminate loop...\n";
	((bb_opt_boolvec*)global_bbo_obj_pointer)->interrupted = true;
}


void bb_opt_boolvec::setup_signal() {

	global_bbo_obj_pointer = this;

	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = bbo_signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);

	// setup deadline handler.
	if (time_deadline != -1) {
		signal(SIGALRM, bbo_deadline_handler);
		alarm(time_deadline * 60);
	}

}

bb_opt_boolvec::bb_opt_boolvec(const cost_fun_t& costfn, const boolvec& initial_x) : costfn(costfn), xt(initial_x) {
	pos2wc = vector<int>(xt.size(), -1);
}

bb_opt_boolvec::~bb_opt_boolvec() {
	if (allow_interrupt) {
		global_bbo_obj_pointer = nullptr;
		signal(SIGINT, SIG_DFL);
	}
}

void bb_opt_boolvec::add_weight_constraint(const std::vector<int>& positions, int min_weight, int max_weight) {
	weight_constraint_t wc;
	assert(min_weight < positions.size());
	wc.positions = positions;
	wc.min_weight = min_weight;
	wc.max_weight = max_weight;
	wc.cur_weight = 0;
	wcvec.push_back(wc);
	for (auto pos : positions) {
		pos2wc.at(pos) = wcvec.size() - 1;
	}
}

void bb_opt_boolvec::optimize(int32_t num_improvements, int32_t max_steps) {

	if (allow_interrupt)
		setup_signal();

	uint32_t avg_n = 1000;
	double cand_cost = 0;
	xt_cand = xt;
	update_cur_weights(xt);
	force_to_constraint(xt);

	best_cost = costfn(xt_cand);
	uint32_t iter = 0;
	uint32_t improve = 0;
	std::cout << "init: " << utl::to_delstr(xt_cand) << " -> " << best_cost << "\n";
	while (iter++ < max_steps) {

		if (interrupted)
			break;

		//std::cout << "iter: " << iter << "\n";
		suggest_candidate(xt, xt_cand, best_cost);
		cand_cost = costfn(xt_cand);
		costavg.add_point(cand_cost);

		// get worst case best_cost since costfn is noisy
		if (noisy_cost_function)
			best_cost = MAX(best_cost, costfn(xt));
		//std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
		//std::cout << utl::to_delstr(xt_cand) << "  -> " << cand_cost << "\n";

		//bool bad_move = (rand() % 64 == 1);
		//if (bad_move || cand_cost < best_cost) {
		if (cand_cost < best_cost) {
			/*if (bad_move)
				std::cout << "making bad move\n";*/
			std::cout << utl::to_delstr(xt_cand) << "  -> " << cand_cost << "\n";
			last_improve = cur_improve;
			cur_improve = std::abs(cand_cost - best_cost);
			if (iter != 0) {
				improve_avg -= improve_avg/avg_n;
				improve_avg += cur_improve/avg_n;
			}
			// std::cout << utl::to_delstr(xt) << "  ->  ";
			// std::cout << "cost: " << best_cost << "    ";
			// std::cout << "improve avg: " << improve_avg << "\n";
			best_cost = cand_cost;
			xt = xt_cand;

			if (++improve == num_improvements)
				break;
		}
	}

}

void bb_opt_boolvec::suggest_candidate(const boolvec& best_x, boolvec& cand_x, float best_cost) {

	cand_x = best_x;

	uint num_flips = cand_x.size() / 2;


	/*	std::cout << "cost mean: " << costavg.get_mean() << " cost s2: " << costavg.get_sigma2() << "\n";
	float stdcost = (best_cost - costavg.get_mean()) / std::sqrt(costavg.get_sigma2());
	std::cout << "standardized cost: " << stdcost << "\n";

	std::cout << "num flips orig: " << num_flips << "\n";*/

	//std::cout << "cost: " << best_cost/cost_scale << "\n";
	num_flips = (uint)((double)num_flips * best_cost/cost_scale);
	//std::cout << "num flips after scale: " << num_flips << "\n";
	//std::cout << "num flips: " << num_flips << "\n";

    std::random_device rd;
    std::mt19937 gen(rd());

    std::normal_distribution<double> dg((double)num_flips, cand_x.size() / 16);
    double gas = dg(gen);
    //std::cout << "gas: " << gas << "\n";
    num_flips = std::round(std::abs(gas));
    num_flips = (rand() % 4) + 1;// std::clamp<uint>(num_flips, 1, 2);
    //std::cout << "num flips gauss: " << num_flips << "\n";

    random_flip_bits(cand_x, num_flips);

}

void bb_opt_boolvec::random_flip_bits(boolvec& cand_x, int num_flips) {

	assert(num_flips != 0);
	vector<size_t> ivec;

	if (num_flips == 1)
		ivec = {rand() % cand_x.size()};
	else
		ivec = utl::rand_indices(cand_x.size());

	for (uint i = 0; i < num_flips; i++) {
		int flip_pos = ivec[i];
		int wcind = pos2wc[flip_pos];
		if (wcind != -1) {
			auto& wc = wcvec[wcind];
			bool low_bound = (wc.cur_weight == wc.min_weight && cand_x[flip_pos]);
			bool high_bound = (wc.cur_weight == wc.max_weight && !cand_x[flip_pos]);

			if (low_bound || high_bound) {
				auto wcrandpos = utl::rand_indices(wc.positions.size());
				for (auto pos : wcrandpos) {
					if (pos != flip_pos && (cand_x[pos] ^ low_bound) ) {
						cand_x[pos] = !cand_x[pos];
						break;
					}
				}
				cand_x[flip_pos] = !cand_x[flip_pos];
			}
			else {
				wc.cur_weight += cand_x[flip_pos] ? -1 : +1;
				cand_x[flip_pos] = !cand_x[flip_pos];
			}
		}
		else {
			cand_x[flip_pos] = !cand_x[flip_pos];
		}
	}
}

void bb_opt_boolvec::force_to_constraint(boolvec& cand_x) {
	//std::cout << "before " << utl::to_delstr(cand_x);
	for (auto& wc : wcvec) {
		if (wc.cur_weight >= wc.min_weight && wc.cur_weight <= wc.max_weight)
			continue;
		auto rinds = utl::rand_indices(wc.min_weight, wc.positions.size());
		for (auto pos : wc.positions)
			cand_x[pos] = 0;
		for (auto ind : rinds)
			cand_x[wc.positions.at(ind)] = 1;
	}
	//std::cout << "after " << utl::to_delstr(cand_x);
}

void bb_opt_boolvec::update_cur_weights(const boolvec& wrt) {
	for (auto& wc : wcvec) {
		for (auto pos : wc.positions) {
			wc.cur_weight += wrt[pos];
		}
	}
}


void bb_opt_boolvec_sa::optimize(int32_t max_i_iterations) {

	if (allow_interrupt)
		setup_signal();

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> ug(0, 1);

    best_cost = costfn(xt);

    uint num_bad = 0;
	for (iter = 0; (max_i_iterations == -1) || iter < max_i_iterations; iter++) {

		if (interrupted)
			break;

		boolvec z = xt;

		bb_opt_boolvec::random_flip_bits(z, 1);

		double fz = costfn(z);

		if (noisy_cost_function)
			best_cost = MAX(best_cost, costfn(xt));

		//std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
		//std::cout << utl::to_delstr(z) << "  -> " << fz << "\n\n";

		if (fz <= best_cost) {
			num_bad = 0;
			xt = z;
			std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
			update_cur_weights(xt);
			best_cost = fz;
		}
		else {
			num_bad++;
			double uni = ug(gen);
			double cutof = std::exp(- (fz - best_cost) * (2 * xt.size() - num_bad) );
			//std::cout << num_bad << "\n";
			if (uni <= cutof) {
				std::cout << uni << " ? " << cutof << "\n";
				xt = z;
				update_cur_weights(xt);
				best_cost = fz;
				std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
			}
		}
	}
}


void bb_opt_boolvec_exp::optimize(int32_t max_i_iterations) {

	if (allow_interrupt)
		setup_signal();

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> ug(0, 1);

    best_cost = costfn(xt);
    trace.push_back(node_t());
    trace.back().xt = xt;
    trace.back().cost = best_cost;
    trace.back().unexplored = utl::rand_indices(xt.size());

    bool improved;
	for (iter = 0; (max_i_iterations == -1) || iter < max_i_iterations; iter++) {
		improved = false;
		assert(!trace.empty());
		while (true) {

			if (interrupted)
				return;

			if (trace.back().unexplored.empty())
				break;

			boolvec z = trace.back().xt;

			int zi = trace.back().unexplored.back();
			trace.back().unexplored.pop_back();
			z[zi] = !z[zi];

/*
			if (rand() % 10 < 2) {
				zi = trace.back().unexplored.back();
				trace.back().unexplored.pop_back();
				z[zi] = !z[zi];
			}
*/

			double fz = costfn(z);
			double ccost = trace.back().cost;

			std::cout << "unexplored: {" << utl::to_delstr(trace.back().unexplored, " ") << "}\n";
			std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
			std::cout << utl::to_delstr(z) << "  -> " << fz << "\n\n";

			//if (fz <= best_cost || (rand() % 100 < 5)) {
/*
			if (rand() % 100 < 1) { // restart
				trace.resize(1);
				improved = false;
				break;
			}
*/
			if (noisy_cost_function)
				best_cost = MAX(best_cost, costfn(xt));

			if (fz <= ccost || rand() % 100 < 5) {
				if (fz <= best_cost) {
					xt = z;
					best_cost = fz;
				}
				trace.push_back(node_t());
				trace.back().xt = z;
				trace.back().cost = fz;
				trace.back().unexplored = utl::rand_indices(xt.size());
				improved = true;
				break;
			}
		}

		if (!improved) {
			trace.pop_back();
			if (trace.empty()) {
				std::cout << "restarting\n";
				trace.push_back(node_t());
				trace.back().xt = utl::random_boolvec(xt.size());
				trace.back().cost = costfn(trace.back().xt);
				trace.back().unexplored = utl::rand_indices(xt.size());
			}
		}
	}
}


void bb_opt_boolvec_exp_fifo::optimize(int32_t max_i_iterations) {

	if (allow_interrupt)
		setup_signal();

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<double> ug(0, 1);

    best_cost = costfn(xt);
    trace[best_cost] = node_t();
    trace[best_cost].xt = xt;
    trace[best_cost].cost = best_cost;
    trace[best_cost].unexplored = utl::rand_indices(xt.size());

    bool improved;
	for (iter = 0; (max_i_iterations == -1) || iter < max_i_iterations; iter++) {
		improved = false;
		assert(!trace.empty());
		while (true) {

			if (interrupted)
				return;

			if (trace.begin()->second.unexplored.empty())
				break;

			boolvec z = trace.begin()->second.xt;

			int zi = trace.begin()->second.unexplored.back();
			trace.begin()->second.unexplored.pop_back();
			z[zi] = !z[zi];

/*
			if (rand() % 10 < 2) {
				zi = trace.back().unexplored.back();
				trace.back().unexplored.pop_back();
				z[zi] = !z[zi];
			}
*/

			double fz = costfn(z);
			double ccost = trace.begin()->second.cost;

			std::cout << "unexplored: {" << utl::to_delstr(trace.begin()->second.unexplored, " ") << "}\n";
			std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
			std::cout << utl::to_delstr(z) << "  -> " << fz << "\n\n";

			//if (fz <= best_cost || (rand() % 100 < 5)) {
/*
			if (rand() % 100 < 1) { // restart
				trace.resize(1);
				improved = false;
				break;
			}
*/
			if (noisy_cost_function) {
				auto new_best_cost = MAX(best_cost, costfn(xt));
				if (new_best_cost != best_cost && trace.find(best_cost) != trace.end()) {
					trace[new_best_cost] = trace.at(best_cost);
					trace.erase(best_cost);
					best_cost = new_best_cost;
				}
			}

			if (fz <= ccost || rand() % 100 < 5) {
				if (fz <= best_cost) {
					xt = z;
					best_cost = fz;
				}
				trace[best_cost] = node_t();
				trace[best_cost].xt = z;
				trace[best_cost].cost = fz;
				trace[best_cost].unexplored = utl::rand_indices(xt.size());
				improved = true;
				break;
			}
		}

		if (!improved) {
			trace.erase(trace.begin()->first);
			if (trace.empty()) {
				std::cout << "restarting\n";
				boolvec new_xt = utl::random_boolvec(xt.size());
				auto cost = costfn(new_xt);
				trace[cost] = node_t();
				trace[cost].xt = new_xt;
				trace[cost].cost = cost;
				trace[cost].unexplored = utl::rand_indices(xt.size());
			}
		}
	}
}


void bb_opt_boolvec_bf::optimize(int32_t max_i_iterations) {


	if (allow_interrupt)
		setup_signal();

    best_cost = costfn(xt);
    xt_cand = xt;

	for (iter = 0; (max_i_iterations == -1) || iter < max_i_iterations; iter++) {

		if (interrupted)
			return;

		if (noisy_cost_function)
			best_cost = MAX(best_cost, costfn(xt));

		double flip_prob = std::pow(2, (-4 * (1 - best_cost)));


		for( uint i = 0; i < xt.size(); i++ ) {

			double rdoub = ((double)rand() / RAND_MAX);
			bool switchp = rdoub < flip_prob;

			xt_cand[i] = (switchp) ? !xt[i] : xt[i];
		}

		double cand_cost = costfn(xt_cand);

		//std::cout << utl::to_delstr(xt_cand) << "  -> " << cand_cost << " " << flip_prob << "\n";

		if (cand_cost < best_cost) {
			xt = xt_cand;
			best_cost = cand_cost;
			std::cout << utl::to_delstr(xt) << "  -> " << best_cost << " \n";
		}

	}

}



} // namespace bbopt



