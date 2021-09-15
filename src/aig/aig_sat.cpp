/*
 * aig_sat.cpp
 *
 *  Created on: Aug 30, 2018
 *      Author: kaveh
 */

#include "sat/c_sat.h"
#include "aig/aig_sat.h"

namespace csat {

using namespace utl;

void add_to_sat(sat_solver& S, aig_t& ntk, id2litmap_t& vrmp) {
	slit p;
	foreach_nodeid(ntk, aid) {
		//add all to maps
		p = S.create_new_var();
		vrmp[aid] = p;
	}

	add_aig_gate_clauses(S, ntk, vrmp);
}

void add_aig_gate_clauses(sat_solver& S, const aig_t& ntk, id2litmap_t& vrmp) {

	foreach_node(ntk, nd, nid) {
		if (nd.isAnd() || nd.isOutput())
			add_aignode_clause(S, nd, vrmp);
	}
}

void add_aignode_clause(sat_solver& S, id nid, alit f0, alit f1, const id2litmap_t& vrmps) {
	std::vector<slit> ins;
	slit a = vrmps.at(f0.lid());
	slit b = vrmps.at(f1.lid());
	slit y = vrmps.at(nid);
	add_logic_clause(S, {(f0.sgn() ? ~a:a), (f1.sgn() ? ~b:b)}, y, fnct::AND);
}

void add_aignode_clause(sat_solver& S, const aigNode& nd, const id2litmap_t& vrmps) {
	std::vector<slit> ins;
	slit a = vrmps.at(nd.fi0());
	slit b = vrmps.at(nd.fi1());
	slit y = vrmps.at(nd.nid);
	add_logic_clause(S, {(nd.cm0() ? ~a:a), (nd.cm1() ? ~b:b)}, y, fnct::AND);
}


id build_diff_ckt(const aig_t& ntk1, const aig_t& ntk2,
		aig_t& retckt, int tie_opts) {

//	retckt = ckt1;
//
//	retckt.add_circuit(tie_opts, ckt2, "$1");
//
//	idvec out(retckt.prim_output_set.begin(), retckt.prim_output_set.end());
//
//	idvec out0(out.begin(), out.begin() + out.size()/2);
//	idvec out1(out.begin() + out.size()/2, out.end());
//
//	id gate_fanout = create_inequality_ckt(out0, out1, retckt, "$0");
//
//	return gate_fanout;
	assert(0); // TODO
	return -1;
}


void _print_varmap(const sat_solver& S, const aig_t& cir, id2litmap_t& vmap){
	/*
	for ( auto& x : cir.prim_input_set ){
		std::cout << cir.wname(x) << " : " << vmap.at(x)
		<< " value: "
			<< S.get_value(vmap.at(x)) << "\n";
	}
	std::cout << "\n";
	for ( auto& x : cir.key_input_set ){
		std::cout << cir.wname(x) << " : " << vmap.at(x)
			<< " value: "
			<< S.get_value(vmap.at(x)) << "\n";
	}
	std::cout << "\n";
	for ( auto& x : cir.prim_output_set ){
		std::cout << cir.wname(x) << " : " << vmap.at(x)
			<< " value: "
			<< S.get_value(vmap.at(x)) << "\n";
	}
	std::cout << "\n";
	*/
}


bool check_equality(const aig_t& ntk1, const aig_t& ntk2) {
/*

	assert(ckt1.nInputs() == ckt2.nInputs());
	assert(ckt1.nOutputs() == ckt2.nOutputs());

	circuit cmp_ckt;
	id fanout = build_diff_ckt(ckt1, ckt2, cmp_ckt, OPT_INP + OPT_KEY);

	sat_solver S;
	id2varmap_t cmpmap;
	add_to_sat(S, cmp_ckt, cmpmap);

	slit out = cmpmap.at(fanout);
	if (!S.solve(out)) {
		std::cout << "equivalent\n";
		return true;
	}
	else {
		std::cout << "different\n";
		return false;
	}
*/
	assert(0); // TODO
	return false;
}


void add_const_sat_anyway(const aig_t& aig, sat_solver& S, id2litmap_t& vmap) {
	if (aig.has_const0()) {
		if (_is_not_in(aig.get_cconst0id(), vmap)) {
			vmap[aig.get_cconst0().lid()] = S.create_new_var();
		}
		S.add_clause(~vmap.at(aig.get_cconst0id()));
	}
}

void add_const_sat_necessary(const aig_t& aig, sat_solver& S, id2litmap_t& vmap) {
	if (aig.has_const0()) {
		if (_is_not_in(aig.get_cconst0id(), vmap)) {
			slit p = S.create_new_var();
			vmap[aig.get_cconst0id()] = p;
			S.add_clause(~p);
		}
	}
}

} // namespace csat



