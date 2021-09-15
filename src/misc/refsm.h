/*
 * refsm.h
 *
 *  Created on: Jan 19, 2019
 *      Author: kaveh
 */

#ifndef SRC_MISC_REFSM_H_
#define SRC_MISC_REFSM_H_

#include "base/circuit.h"
#include "sat/c_sat.h"

namespace re {

using namespace ckt;
using namespace utl;
using namespace csat;

class state_t;

class state_t {
	 boolvec state_val;
	 std::vector<state_t*> next_states;
};

class fsm_t {
	std::unordered_map<boolvec, state_t> statemap;
};

class refsm {

	const circuit* cir;
	idset sl_gates;
	idset sl_wires; // all state-logic wires
	idset sl_ffouts; // state-logic state-inputs
	idset sl_ffins; // state-logic state-inputs
	idset sl_pis; // state-logic primary inputs

	sat_solver S;

	fsm_t fsm;

public:
	refsm(const circuit& crt) : cir(&crt) {}
	void extract_state_logic(const oidset& state_ffs);
	void enumerate_fsm(const oidset& state_ffs, int max_states);
};


} // namespace re


#endif /* SRC_MISC_REFSM_H_ */
