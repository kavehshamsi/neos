/*
 * stat.h
 *
 *  Created on: Apr 23, 2018
 *      Author: kaveh
 *      Description: a module for statistical properties of circuits
 */

#ifndef STAT_H_
#define STAT_H_

#include "base/circuit.h"
#include <boost/dynamic_bitset.hpp>

#include "utl/cktgraph.h"
#include "dec/dec_sat.h"

namespace stat_ns {

using namespace ckt;

// data structures for fourier analysis
// -1: undef, 0:0, 1:1

typedef long double real;
typedef Hmap<id, real> id2realHmap;
typedef idmap<real> id2realmap;

void propagate_prob(const circuit& ckt, id2realmap& probMap);
real propagate_prob(const gate& gt, id2realmap& probMap);

void sig_controllability(const circuit& cir, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);
void sig_controllability(const gate& gt, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);

void sig_observability(const circuit& cir, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);
void sig_observability(const gate& gt, id2idmap& cc0_map, id2idmap& cc1_map, id2idmap& co_map);

void sig_prob_eqn(const circuit& ckt, id2realmap& probMap);
void sig_prob_sim(const circuit& ckt, uint num_patterns, id2realmap& probMap);
void sig_prob_dupuis(const circuit& cir, uint num_patterns, id2realmap& probmap);
void flip_impact_sim(const circuit& ckt, uint num_patterns, id2realmap& probMap);
double flip_impact_sim(id wid, const circuit& ckt, uint num_patterns);
void sig_prob_seq(const circuit& ckt, uint num_patterns, uint max_depth,
		id2realmap& probMap, const oidset& frozens);
void get_output_prob(const circuit& ckt, id num_patterns = -1);
void sig_prob_graph(const circuit& sim_ckt, uint num_patts, const std::string& filename);

} // namespace st


#endif /* SRC_STAT_STAT_H_ */
