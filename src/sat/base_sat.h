/*
 * base_sat.h
 *
 *  Created on: Dec 8, 2017
 *      Author: kaveh
 */

#ifndef SRC_DOBF_BASE_SAT_H_
#define SRC_DOBF_BASE_SAT_H_

#include "utl/idmap.h"
#include "sat/sat.h"

#include "base/base.h"

namespace csat {

using namespace sat;
using namespace ckt;

typedef std::pair<id2boolmap, id2boolmap> iopair;
//typedef std::map<id, slit> id2litmap_t;
typedef utl::idmap<slit> id2litmap_t;

void add_logic_clause(sat_solver& S, const std::vector<slit>& ins, slit y, fnct fn);
slit add_logic_clause(sat_solver& S, const std::vector<slit>& ins, fnct fn);
void get_logic_clause(vector<vector<slit>>& Cls, const std::vector<slit>& ins, slit y, fnct fn);

bool getValue(const sat_solver& S, slit p);
} // namespace c_sat

#endif /* SRC_DOBF_BASE_SAT_H_ */
