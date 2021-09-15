/*
 * refsm.cpp
 *
 *  Created on: Jan 19, 2019
 *      Author: kaveh
 */

#include "refsm.h"

namespace re {

void refsm::extract_state_logic(const oidset& state_ffs) {

	using namespace utl;

	idset visited = to_hset(state_ffs);
	idque Q;

	for (auto ffid : state_ffs) {
		id ffout = cir->gfanout(ffid);
		sl_ffouts.insert(ffout);
		id ffin = cir->gfanin0(ffid);
		sl_ffins.insert(ffin);
		std::cout << "sl ffin: " << cir->wname(ffin) << "   ";
		std::cout << "sl ffout: " << cir->wname(ffout) << "\n";
		Q.push(ffid);
	}

	while (!Q.empty()) {

		id curgid = utl::utldeque(Q);

		for (auto wfanin : cir->gfanin(curgid)) {

			id ginid = cir->wfanin0(wfanin);
			if (ginid != -1) { // for non-input wires

				if (_is_not_in(ginid, visited)) {
					if (cir->isLatch(ginid)) {
						std::cout << "found non-state dff in state-logic\n";
					}
					else {
						sl_gates.insert(ginid);
						Q.push(ginid);
					}
				}
			}
			else {
				if (!cir->isInput(wfanin)) {
					std::cout << "state-logic input signal " <<
							cir->wname(wfanin) << " is not a primary input\n";
				}
				if (!cir->isConst(wfanin)) {
					std::cout << "state logic input: " << cir->wname(wfanin) << "\n";
					sl_pis.insert(wfanin);
				}
			}

			sl_wires.insert(wfanin);
		}
	}

}

void refsm::enumerate_fsm(const oidset& state_ffs, int max_states) {

	extract_state_logic(state_ffs);

	sat_solver S, Sexp;
	slitvec assumps;
	id2litmap_t wlitmap;


	for (auto sl_wid : sl_wires) {
		create_variable(S, wlitmap, sl_wid);
	}

	for (auto sl_gid : sl_gates) {
		add_gate_clause(S, cir->getcgate(sl_gid), wlitmap);
	}

	add_const_sat_anyway(*cir, S, wlitmap);

	std::list<boolvec> state_Q;
	boolvec cur_state;
	for (auto sl_dffout : sl_ffouts) {
		assumps.push_back(~wlitmap.at(sl_dffout)); // zero initialize
		cur_state.push_back(0);
	}

	/*
	state_t st;
	st.state_val = cur_state;
	fsm.statemap[cur_state] = st;*/

	Sexp = S;

	int step = 0;
	while (step++ < max_states) {

		if (Sexp.solve(assumps)) {

			// extract input
			std::cout << "x=";
			for (auto sl_pi : sl_pis) {
				std::cout << Sexp.get_value(wlitmap.at(sl_pi));
			}

			// print current state
			std::cout << " : ";
			for (auto b : cur_state) {
				std::cout << b;
			}

			std::cout << " -> ";

			// extract next state
			slitvec ban_clause;
			boolvec next_state_vec;
			for (auto sl_ffin : sl_ffins) {
				slit ns_lit = wlitmap.at(sl_ffin);
				bool ns_val = Sexp.get_value(wlitmap.at(sl_ffin));
				std::cout << ns_val;
				next_state_vec.push_back(ns_val);
				ban_clause.push_back(ns_val ? ~ns_lit : ns_lit);
			}
			Sexp.add_clause(ban_clause);

			state_Q.push_front(next_state_vec);
			/*state_t next_state_obj;
			next_state_obj.state_val = next_state_vec;
			fsm.statemap[next_state_vec] = next_state_obj;
			fsm.statemap[cur_state].next_states.push_back(&fsm.statemap.at(next_state_vec));*/

			/*for (auto& st : state_Q) {
				std::cout << " { ";
				for (auto b : st) {
					std::cout << b;
				}
				std::cout << "}, ";
			}*/

			std::cout << "\n";
		}
		else {
			if (state_Q.empty()) {
				std::cout << "\ndone exploring!\n";
				break;
			}

			slitvec ban_clause;
			int i = 0;
			for (auto sl_ffin : sl_ffins) {
				slit ns_lit = wlitmap.at(sl_ffin);
				bool ns_val = cur_state[i++];
				ban_clause.push_back(ns_val ? ~ns_lit : ns_lit);
			}
			S.add_clause(ban_clause);

			Sexp = S; // reset SAT solver

			cur_state = state_Q.back();
			state_Q.pop_back();

			assumps.clear();
			std::cout << "now exploring from : s=";
			i = 0;
			for (auto sl_dffout : sl_ffouts) {
				bool state_val = cur_state[i++];
				std::cout << state_val;
				assumps.push_back(state_val ? wlitmap.at(sl_dffout):~wlitmap.at(sl_dffout));
			}
			std::cout << "\n";
		}
	}
}


} // namespace re


