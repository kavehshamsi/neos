/*
 * pbsat.cpp
 *
 *  Created on: Mar 28, 2021
 *      Author: kaveh
 */

#include "sat/pbsat.h"
#include "bdd/bdd.h"
#include "base/blocks.h"

namespace csat {

using namespace dd;
using namespace ckt;

void lin_constr_t::normalize() {
	for (uint i = 0; i < coeffs.size(); i++) {
		assert(abs(coeffs[i]) == 1);
		if (coeffs[i] < 0) {
			coeffs[i] = abs(coeffs[i]);
			rhs += coeffs[i];
			sgns[i] = ~sgns[i];
		}
	}
}

BDD constr_build_BDD_geq(const vector<bool>& sgns, int rhs, int pos, int sum, Omap<std::pair<int, int>, BDD>& memo, Cudd& bdd_mgr) {
	int total_len = sgns.size();
	int vars_left = total_len - pos;

	if (sum >= rhs)
		return bdd_mgr.bddOne();
	else if (sum + vars_left < rhs)
		return bdd_mgr.bddZero();

	std::pair<int, int> key = {pos, sum};
	auto it = memo.find(key);
	if (it == memo.end()) {
		int hi_sum = (sgns[pos]) ? sum : sum + 1;
		int lo_sum = (sgns[pos]) ? sum + 1 : sum;
		BDD hi_result = constr_build_BDD_geq(sgns, rhs, pos+1, hi_sum, memo, bdd_mgr);
		BDD lo_result = constr_build_BDD_geq(sgns, rhs, pos+1, lo_sum, memo, bdd_mgr);
		const auto& ret = memo[key] = bdd_mgr.bddVar(pos).Ite(hi_result, lo_result);
		return ret;
	}
	else {
		return it->second;
	}

}

BDD constr_build_BDD_leq(const vector<bool>& sgns, int rhs, int pos, int sum, Omap<std::pair<int, int>, BDD>& memo, Cudd& bdd_mgr) {
	int total_len = sgns.size();
	int vars_left = total_len - pos;

	if (sum > rhs)
		return bdd_mgr.bddZero();
	else if (sum + vars_left <= rhs)
		return bdd_mgr.bddOne();

	std::pair<int, int> key = {pos, sum};
	auto it = memo.find(key);
	if (it == memo.end()) {
		int hi_sum = (sgns[pos]) ? sum : sum + 1;
		int lo_sum = (sgns[pos]) ? sum + 1 : sum;
		BDD hi_result = constr_build_BDD_leq(sgns, rhs, pos+1, hi_sum, memo, bdd_mgr);
		BDD lo_result = constr_build_BDD_leq(sgns, rhs, pos+1, lo_sum, memo, bdd_mgr);
		const auto& ret = memo[key] = bdd_mgr.bddVar(pos).Ite(hi_result, lo_result);
		return ret;
	}
	else {
		return it->second;
	}

}

void lin_constr_cir_bdd(circuit& cir, lin_constr_t& lc) {

	lc.normalize();

	Cudd bdd_mgr;
	bdd_mgr.AutodynEnable();
	BDD tip;
	if (lc.op == lin_constr_t::CONSTR_GEQ) {
		Omap<std::pair<int, int>, BDD> memo;
		tip = constr_build_BDD_geq(lc.sgns, lc.rhs, 0, 0, memo, bdd_mgr);
		circuit_bdd::write_to_dot("./bdd_geq.dot", bdd_mgr, {tip});
	}
	else if (lc.op == lin_constr_t::CONSTR_LEQ) {
		Omap<std::pair<int, int>, BDD> memo;
		tip = constr_build_BDD_leq(lc.sgns, lc.rhs, 0, 0, memo, bdd_mgr);
		circuit_bdd::write_to_dot("./bdd_leq.dot", bdd_mgr, {tip});
	}
	else if (lc.op == lin_constr_t::CONSTR_EQ) {
		Omap<std::pair<int, int>, BDD> memo;
		BDD tip_leq = constr_build_BDD_leq(lc.sgns, lc.rhs, 0, 0, memo, bdd_mgr);
		memo.clear();
		BDD tip_geq = constr_build_BDD_geq(lc.sgns, lc.rhs, 0, 0, memo, bdd_mgr);
		tip = tip_leq & tip_geq;
		circuit_bdd::write_to_dot("./bdd_eq.dot", bdd_mgr, {tip});
	}

	vector<string> vnames;
	for (uint i = 0; i < lc.coeffs.size(); i++) vnames.push_back(fmt("x%1%", i));

	circuit_bdd::to_circuit(cir, bdd_mgr, vnames, {tip});

}


void lin_constr_cir_oe_sort(circuit& cir, lin_constr_t& lc) {

	lc.normalize();

	circuit sortcir = ckt_block::bit_odd_even_sorter(lc.sgns.size());
	idvec outvec = utl::to_vec(sortcir.outputs());

	id tip = -1;

	if (lc.op == lin_constr_t::CONSTR_GEQ) {
		int oneindex = lc.rhs-1;
		sortcir.setwirename(outvec[oneindex], "geq_tip");
		sortcir.setwiretype(outvec[oneindex], wtype::OUT);
		tip = outvec[oneindex];
	}
	else if (lc.op == lin_constr_t::CONSTR_LEQ) {
		int zeroindex = lc.rhs - 1;
		tip = sortcir.add_wire("leq_tip", wtype::OUT);
		sortcir.setwiretype(outvec[zeroindex], wtype::INTER);
		sortcir.add_gate({outvec[zeroindex]}, tip, fnct::NOT);
		tip = outvec[zeroindex];
	}
	else if (lc.op == lin_constr_t::CONSTR_EQ) {
		int zeroindex = lc.rhs - 1;
		int oneindex = lc.rhs - 2;
		assert(oneindex >= 0);
		tip = sortcir.add_wire("eq_tip", wtype::OUT);
		sortcir.add_gate({outvec[zeroindex], outvec[oneindex]}, tip, fnct::XOR);
	}

	sortcir.trim_circuit_to({tip});

}

} // namespace csat


