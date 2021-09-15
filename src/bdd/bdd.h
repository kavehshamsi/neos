/*
 * bdd.h
 *
 *  Created on: Jul 8, 2018
 *      Author: kaveh
 */

#ifndef SRC_BASE_BDD_H_
#define SRC_BASE_BDD_H_


#include "base/neos_config.h"


#include <stack>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>
#include <iostream>

#include "base/circuit.h"
#include "cudd/cplusplus/cuddObj.hh"
#include "cudd/cudd/cuddInt.h"

namespace dd {

using namespace ckt;

typedef std::vector<BDD> bddvec;
typedef Hmap<id, BDD> id2bddmap;

BDD apply_op(Cudd& mgr, const bddvec& inputs, fnct op, uint limit = 0);
BDD gate_bdd_op(const gate& g, Cudd& mgr, const id2bddmap& bddmap, uint limit = 0);
int add_gate_bdds(const circuit& cir, Cudd& mgr, id2bddmap& bddmap, uint size_limit = 0);

class circuit_bdd {
private:
	Cudd _mgr;
	std::vector<std::string> _vnames;
	std::unordered_map<std::string, uint> _vname2index;
	id2bddmap _id2bdd_map;
	bddvec outs;
	const circuit* owner_ckt = nullptr;

	bool is_ok = false;

public:
	uint size_limit = 0;

public:
	circuit_bdd();
	circuit_bdd(const circuit& ckt, uint size_limit = 0);

	void init_bddvars_from_cktins(const circuit& ckt);
	void add_constants(const circuit& ckt);
	void add_constants(const circuit& ckt, id2bddmap& bddmap);
	Cudd& get_mgr() { return _mgr; }

	void create_var(const std::string& varname, id wid);
	uint num_vars() { return _vnames.size(); }
	int get_index(const std::string& str);
	BDD get_out(int out_ind) { return outs.at(out_ind); }
	std::string get_name(int index);
	bool isGood() { return is_ok; }

	int num_nodes(id oid = -1);
	id add_ckt_to_bdd(const circuit& ckt, fnct op);
	id add_ckt_to_bdd(const circuit& cir, fnct op, id2bddmap& bddmap);
	const BDD& cirwid2node(id wid) {
		return _id2bdd_map.at(wid);
	}

	void write_to_dot(const std::string& filename);
	std::string get_stats();

	// convert to MUX circuit
	void to_circuit(circuit& cir);

	static void write_to_dot(const std::string& filename, Cudd& mgr, const bddvec& nodes,
			const vector<string>& inames = utl::empty<vector<string>>());
	static void to_circuit(circuit& cir, Cudd& mgr, const vector<string>& vnames, const bddvec& outs);

};

void test_bdd_ckt(const circuit& ckt);

}


#endif /* SRC_BASE_BDD_H_ */
