#include "sat/c_sat.h"

namespace csat {

using namespace utl;

void add_to_sat(sat_solver& S, const circuit& cir, id2litmap_t& vrmps) {
	slit p;
	foreach_wire(cir, w, wid) {
		//add all to maps
		p = S.create_new_var();
		vrmps[wid] = p;
	}

	foreach_gate(cir, g, gid) {
		if (!g.isLatch())
			add_gate_clause(S, g, vrmps);
	}
}

void add_const_sat_anyway(const circuit& ckt, sat_solver& S, id2litmap_t& vmap) {
	if (ckt.has_const0()) {
		if (_is_not_in(ckt.get_cconst0(), vmap)) {
			vmap[ckt.get_cconst0()] = S.create_new_var();
		}
		S.add_clause(~vmap.at(ckt.get_cconst0()));
	}
	if (ckt.has_const1()) {
		if (_is_not_in(ckt.get_cconst1(), vmap)) {
			vmap[ckt.get_cconst1()] = S.create_new_var();
		}
		S.add_clause(~vmap.at(ckt.get_cconst1()));
	}
}

void add_const_sat_necessary(const circuit& ckt, sat_solver& S, id2litmap_t& vmap) {
	if (ckt.has_const0()) {
		if (_is_not_in(ckt.get_cconst0(), vmap)) {
			vmap[ckt.get_cconst0()] = S.false_lit();
		}
	}
	if (ckt.has_const1()) {
		if (_is_not_in(ckt.get_cconst1(), vmap)) {
			vmap[ckt.get_cconst1()] = S.true_lit();
		}
	}
}


void add_ckt_to_sat_necessary(sat_solver& S, const circuit& cir, id2litmap_t& vrmps) {

	add_const_sat_necessary(cir, S, vrmps);

	slit p;
	foreach_wire(cir, w, wid) {
		//add all to maps
		if (_is_not_in(wid, vrmps)) {
			p = S.create_new_var();
			vrmps[wid] = p;
		}
	}

	foreach_gate(cir, g, gid) {
		if (!g.isLatch())
			add_gate_clause(S, g, vrmps);
	}

}

void add_ckt_gate_clauses(sat_solver& S, const circuit& cir, id2litmap_t& vrmps) {

	foreach_gate(cir, g, gid) {
		if (!g.isLatch())
			add_gate_clause(S, g, vrmps);
	}
}

void add_gate_clause(sat_solver& S, const ckt::gate& g, const id2litmap_t& vrmps) {

	std::vector<slit> ins;
	for (auto x : g.fanins) {
		ins.push_back(vrmps.at(x));
	}
	slit y = vrmps.at(g.fo0());
	add_logic_clause(S, ins, y, g.gfun());

}

void add_gate_clause(sat_solver& S, const idvec& fanins, id fanout, fnct fun, const id2litmap_t& vrmps) {
	gate gt(0, nltype::GATE, fanins, fanout, (int8_t)fun);
	add_gate_clause(S, gt, vrmps);
}

/*
void push_all(std::vector<slit>& slitvec, const std::vector<slit>& toadd) {
	for (uint i = 0; i < toadd.size(); i++)
		slitvec.push_back(toadd[i]);
}

void pop_all(std::vector<slit>& slitvec, const std::vector<slit>& toadd) {
	uint i = 0;
	while (i++ < toadd.size())
		slitvec.pop_back();
}
*/

slit create_variable(sat_solver& S, id2litmap_t& varmap, id wid) {
	slit p = S.create_new_var();
	varmap[wid] = p;
	return p;
}

slit create_variable_necessary(sat_solver& S, id2litmap_t& varmap, id wid) {
	try {
		return varmap.at(wid);
	}
	catch (std::out_of_range&) {
		slit p = varmap[wid] = S.create_new_var();
		return p;
	}
}

id build_diff_ckt(const circuit& ckt1, const circuit& ckt2,
		circuit& retckt, int tie_opts) {

	retckt = ckt1;

	id2idmap old2newmap;
	if (tie_opts & OPT_INP) {
		for (auto x2 : ckt2.inputs())
			old2newmap[x2] = ckt1.find_wcheck(ckt2.wname(x2));
	}
	if (tie_opts & OPT_KEY) {
		for (auto x2 : ckt2.keys())
			old2newmap[x2] = ckt1.find_wcheck(ckt2.wname(x2));
	}

	retckt.add_circuit(ckt2, old2newmap, "_$1");

	idvec out(retckt.outputs().begin(), retckt.outputs().end());

	vector<idvec> outs(2);
	devide_ckt_ports(retckt, retckt.outputs(), outs, {"_$1"});

	id diff_tip = build_comparator(outs[0], outs[1], retckt, "$0");

	for (auto& out : outs) {
		for (auto oid : out) {
			retckt.setwiretype(oid, wtype::INTER);
		}
	}

	return diff_tip;
}

id equality_comparator(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix) {
	return build_comparator(xvec1, xvec2, cir, name_postfix);
}

id inequality_comparator(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix) {
	return build_comparator(xvec1, xvec2, cir, name_postfix);
}

// adds a comparator between two vectors
id build_comparator(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix, bool ineq) {

	if (xvec1.size() != xvec2.size()) {
		std::cout << "size mismatch between inequaslity vectors\n";
		abort();
	}

	// add XORs and construct OR fanins at the end
	idvec orgate_fanins;
	for ( uint i = 0; i < xvec1.size(); i++ ){
		string ident = name_postfix + "_" + std::to_string(i);
		idvec xor_fanins = {xvec1[i], xvec2[i]};
		id xor_out = cir.add_wire("cmp_" + ident, wtype::INTER);
		cir.add_gate(xor_fanins, xor_out, (ineq ? fnct::XOR : fnct::XNOR));
		orgate_fanins.push_back(xor_out);
	}

	string ident = name_postfix;
	id gate_fanout = cir.add_wire("cmp_" + ident, wtype::OUT);
	cir.add_gate(orgate_fanins, gate_fanout, (ineq ? fnct::OR : fnct::AND));


	return gate_fanout;
}

// compare the integer value of two vectors
id build_integer_leqcir(const idvec& xvec1, const idvec& xvec2,
		circuit& cir, const std::string& name_postfix, bool ineq) {

	id2idmap fanin_connections;
	id2idmap xorB_cons;

	circuit tmp_cir = cir;

	if (xvec1.size() != xvec2.size()) {
		std::cout << "size mismatch between inequaslity vectors\n";
		exit(EXIT_FAILURE);
	}

	// add XORs and construct OR fanins at the end
	idvec orgate_fanins;
	for ( uint i = 0; i < xvec1.size(); i++ ){
		string ident = name_postfix + "_" + std::to_string(i);
		idvec xor_fanins = {xvec1[i], xvec2[i]};
		id xor_out = tmp_cir.add_wire("cmp_" + ident, wtype::INTER);
		tmp_cir.add_gate(xor_fanins, xor_out, (ineq ? fnct::XOR:fnct::XNOR));
		orgate_fanins.push_back(xor_out);
	}

	string ident = name_postfix;
	id gate_fanout = tmp_cir.add_wire("cmp_" + ident, wtype::OUT);
	tmp_cir.add_gate(orgate_fanins, gate_fanout, (ineq ? fnct::OR:fnct::AND));

	// set duplckt, and clean it up
	cir = tmp_cir;

	return gate_fanout;
}

bool check_equality_sat(const circuit& ckt1, const circuit& ckt2) {

	assert(ckt1.nInputs() == ckt2.nInputs());
	assert(ckt1.nOutputs() == ckt2.nOutputs());

	circuit cmp_ckt;
	id fanout = build_diff_ckt(ckt1, ckt2, cmp_ckt, OPT_INP + OPT_KEY);

	sat_solver S;
	id2litmap_t cmpmap;
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
}



} // namespace c_sat
