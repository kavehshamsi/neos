/*
 * aig_sat.h
 *
 *  Created on: Aug 30, 2018
 *      Author: kaveh
 *      Description: AIG SAT interface
 */

#ifndef SRC_AIG_AIG_SAT_H_
#define SRC_AIG_AIG_SAT_H_

#include "aig/aig.h"
#include "sat/base_sat.h"

namespace csat {

using namespace aig_ns;

// SAT methods
void add_aignode_clause(sat_solver& S, const aigNode& nd, const id2litmap_t& vrmp);
void add_aignode_clause(sat_solver& S, id nid, alit f0, alit f1, const id2litmap_t& vrmps);

// mitter aig_ckts
id build_diff_ckt(const aig_t& ntk1, const aig_t& ntk2,
		aig_t& retckt, int tie_opts = OPT_INP);
id create_inequality_ckt(idvec xvec1, idvec xvec2,
		aig_t& cir, const std::string& name_postfix);

// aig_ckt to SAT
void add_to_sat(sat_solver& S, aig_t& cir, id2litmap_t& vrmp);
void add_aig_gate_clauses(sat_solver& S, const aig_t& cir, id2litmap_t& vrmps);
void add_const_sat_anyway(const aig_t& ckt, sat_solver& S, id2litmap_t& vmap);
void add_const_sat_necessary(const aig_t& ckt, sat_solver& S, id2litmap_t& vmap);

// SAT based equivalence checker
bool check_equality(const aig_t& ckt1, const aig_t& ckt2);

void _print_varmap(const sat_solver& S, const aig_t& cir, id2litmap_t& map);




} // namespace csat


#endif /* SRC_AIG_AIG_SAT_H_ */
