/*
 * prove.h
 *
 *  Created on: Jun 25, 2020
 *      Author: kaveh
 */

#ifndef SRC_SAT_PROVE_H_
#define SRC_SAT_PROVE_H_

#include "opt/opt.h"
#include "opt/rwr.h"
#include "sat/c_sat.h"

namespace prove {

using namespace aig_ns;
int prove_fraig0(const aig_t& miter_orig, const oidset& mit_outs, id2boolHmap& cex, int64_t max_prop_limit = -1);
int prove_fraig0(const aig_t& miter_orig, aig_t& simp_miter, const oidset& mit_outs, id2boolHmap& cex,
		int64_t max_prop_limit = -1, csat::sat_solver* Sptr = nullptr, const csat::slitvec& assumps = utl::empty<csat::slitvec>());
int check_equality_fraig(const aig_t& ntk1, const aig_t& ntk2);

} // namespace opt_ns

#endif /* SRC_SAT_PROVE_H_ */
