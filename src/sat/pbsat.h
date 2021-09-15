/*
 * pbsat.h
 *
 *  Created on: Mar 28, 2021
 *      Author: kaveh
 */

#ifndef SRC_SAT_PBSAT_H_
#define SRC_SAT_PBSAT_H_

#include "sat/c_sat.h"
#include "base/circuit.h"

namespace csat {

struct lin_constr_t {
	enum constr_op_t {
		CONSTR_EQ,
		CONSTR_GEQ,
		CONSTR_LEQ
	} op;
	vector<int> coeffs;
	vector<bool> sgns;
	int rhs;

	void normalize();
	int nterms() { return coeffs.size(); }
};


void lin_constr_cir_bdd(circuit& circuit, lin_constr_t& lc);
void lin_constr_cir_oe_sort(circuit& cir, lin_constr_t& lc);

} // namespace csat

#endif /* SRC_SAT_PBSAT_H_ */
