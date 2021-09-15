/*
 * ckt_sat.h
 *
 *  Created on: Apr 30, 2016
 *      Author: kaveh
 */

#ifndef C_SAT_H_
#define C_SAT_H_

#include "utl/utility.h"
#include "sat/sat.h"
#include "base/circuit.h"
#include "aig/aig.h"
#include "sat/base_sat.h"

namespace csat {

//@DEBUG MACROS
// #define IOVECDEBUG
// #define CSATDEBUG
//#define DIPDEBUG

using namespace sat;
using namespace ckt;
using namespace aig_ns;
using namespace utl;


struct xor_pair {
	id xorB = 0;
	id xorO = 0;
};

// SAT methods
void add_gate_clause(sat_solver& S, const ckt::gate& g, const id2litmap_t& vrmp);
void add_gate_clause(sat_solver& S, const idvec& fanins, id fanout, fnct fun,
		const id2litmap_t& vrmps);

slit create_variable(sat_solver& S, id2litmap_t& varmap, id wireid);
slit create_variable_necessary(sat_solver& S, id2litmap_t& varmap, id wid);

// mitter circuits
id equality_comparator(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix);
id inequality_comparator(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix);
id build_comparator(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix, bool comp = true);

id build_diff_ckt(const circuit& ckt1, const circuit& ckt2,
		circuit& retckt, int tie_opts = OPT_INP);

bool check_equality_sat(const circuit& ckt1, const circuit& ckt2);

// circuit to SAT
void add_to_sat(sat_solver& S, const circuit& cir, id2litmap_t& vrmp); // FIXME: we have to of these!
void add_ckt_to_sat_necessary(sat_solver& S, const circuit& cir, id2litmap_t& vrmp); // FIXME: we have to of these!
void add_ckt_gate_clauses(sat_solver& S, const circuit& cir, id2litmap_t& vrmps);
void add_const_sat_necessary(const circuit& ckt, sat_solver& S, id2litmap_t& vmap);
void add_const_sat_anyway(const circuit& ckt, sat_solver& S, id2litmap_t& vmap);

void _print_varmap(const sat_solver& S, const circuit& cir, id2litmap_t& map);

// helper functions
template<typename M>
void _fill_rand(M& key_map) {
	for (auto&& x : key_map)
		x.second = ((rand() % 2) == 0);
	return;
}

void _seed_sat_solver(sat_solver& S);

template<typename T>
void devide_ckt_ports(const circuit& cmp_ckt, const T& port_set,
		vector<idvec>& outvecs, const std::vector<std::string>& postfixs) {

	for (auto wid0 : port_set) {
		string wn0 = cmp_ckt.wname(wid0);
		bool skip = false;
		for (auto& postfix : postfixs) {
			if ( _is_in(postfix, wn0) ) {
				skip = true;
				break;
			}
		}
		if (!skip) { // name without postfixs
			outvecs[0].push_back(wid0);
			uint i = 1;
			for (auto& postfix : postfixs) {
				string wn1 = wn0 + postfix;
				id widi = cmp_ckt.find_wcheck(wn1);
				outvecs[i++].push_back(widi);
			}
		}
	}
}


} // namespace csat


#endif /* SRC_C_SAT_H_ */

