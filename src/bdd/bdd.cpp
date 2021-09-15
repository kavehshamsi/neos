/*
 * bdd.cpp
 *
 *  Created on: Jul 8, 2018
 *      Author: kaveh
 */

#include "bdd/bdd.h"
#include <string.h>

namespace dd {

circuit_bdd::circuit_bdd() {}

circuit_bdd::circuit_bdd(const circuit& cir, uint szl) {

	size_limit = szl;
	owner_ckt = &cir;

	auto start = utl::_start_wall_timer();

	init_bddvars_from_cktins(cir);
	_mgr.AutodynEnable();

	if ( add_gate_bdds(cir, _mgr, _id2bdd_map, size_limit) == -1 ) {
		is_ok = false;
		std::cout << "BDD reached size limit of " << size_limit << ". aborting\n";
		return;
	}
	is_ok = true;

	foreach_outid(cir, oid)
		outs.push_back(_id2bdd_map.at(oid));

	std::cout << "done conversion. time: " << utl::_stop_wall_timer(start) << "\n";

}

void circuit_bdd::init_bddvars_from_cktins(const circuit& cir) {

	foreach_inid(cir, inid) {
		create_var(cir.wname(inid), inid);
	}

	foreach_keyid(cir, kid) {
		create_var(cir.wname(kid), kid);
	}

	foreach_latch(cir, dffid, dffout, dffin) {
		create_var(cir.wname(dffout), dffout);
	}

	add_constants(cir);
}

void circuit_bdd::create_var(const std::string& vname, id wid) {
	assert( _is_not_in(vname, _vname2index) );
	_vname2index[vname] = num_vars();
	_vnames.push_back(vname);
	_id2bdd_map[wid] = _mgr.bddVar();
}

void circuit_bdd::add_constants(const circuit& ckt, id2bddmap& bddmap) {
	if (ckt.has_const0())
		bddmap[ckt.get_cconst0()] = _mgr.bddZero();
	if (ckt.has_const1())
		bddmap[ckt.get_cconst1()] = _mgr.bddOne();
}

void circuit_bdd::add_constants(const circuit& cir) {
	add_constants(cir, _id2bdd_map);
}

std::string circuit_bdd::get_name(int index) {
	return _vnames.at(index);
}

int circuit_bdd::get_index(const std::string& str) {
	try {
		return _vname2index.at(str);
	} catch (std::out_of_range&) {
		return -1;
	}
}

// add circuit gates to BDD. If size_limit is reached return 1 else return 0
int add_gate_bdds(const circuit& cir, Cudd& mgr, id2bddmap& bddmap, uint size_limit) {

	const idvec& gate_order = cir.get_topsort_gates();

	id gcount = 0;

	for (auto gid : gate_order) {

		id gout = cir.gfanout(gid);
		fnct fun = cir.gfunction(gid);
		//std::cout << "at " << gcount++ << "/" << gate_order.size() << "\n\r";
 		std::vector<BDD> inbdds;

		for (auto fanin : cir.gfanin(gid)) {
			try {
				inbdds.push_back(bddmap.at(fanin));
			} catch (std::out_of_range&) {
				std::cout << "BDD for " << cir.wname(fanin) << " not found\n";
				assert(0);
			}
		}

		BDD y = apply_op(mgr, inbdds, fun, size_limit);
		// wish there was a faster way to get the nodeCount
		if (y.getNode() == NULL) {
			return -1;
		}
		bddmap[gout] = y;

	}

	return 0;

}


/*
 * add single-output circuit to single-ended bdd and apply operator to outputs.
 * inputs are matched by name
 */
id circuit_bdd::add_ckt_to_bdd(const circuit& cir, fnct op) {

	id2bddmap bddmap;
	return add_ckt_to_bdd(cir, op, bddmap);

}

id circuit_bdd::add_ckt_to_bdd(const circuit& cir, fnct op, id2bddmap& bddmap) {

	assert(outs.size() <= 1);
	assert(cir.nOutputs() == 1);

	if (owner_ckt == NULL)  {// fresh BDD
		owner_ckt = &cir;
		_mgr.AutodynEnable();
	}

	for (auto inid : cir.allins()) {
		int index = get_index(cir.wname(inid));
		if (index == -1) {
			//std::cout << "variable " << ckt.wname(inid) << " not found in bdd. adding it\n";
			create_var(cir.wname(inid), inid);
			bddmap[inid] = _id2bdd_map.at(inid);
		}
		else {
			bddmap[inid] = _mgr.bddVar(index);
		}
	}

	add_constants(cir, bddmap);
	if (add_gate_bdds(cir, _mgr, bddmap, size_limit) == -1) {
		std::cout << "add_opt failed due to size blow-up (size_limit = " << size_limit << ")\n";
		return -1;
	}

	id cir_out = *(cir.outputs().begin());
	BDD y = bddmap.at(cir_out);

	id ret = cir_out;
	if (!outs.empty() && op != fnct::UNDEF) {
		outs[0] = apply_op(_mgr, {outs[0], y}, op, size_limit);
		if (outs[0].getNode() == 0) {
			std::cout << "add_opt failed due to size blow-up (size_limit = " << size_limit << ")\n";
			ret = -1;
		}
	}
	else {
		if (op != fnct::UNDEF || outs.empty()) {
			outs.push_back(bddmap.at(cir_out));
		}
	}

	_id2bdd_map.clear();

	return ret;
}

BDD gate_bdd_op(const gate& g, Cudd& mgr, const id2bddmap& bddmap, uint limit) {

	fnct fun = g.gfunction();

	std::vector<BDD> inbdds;

	for (auto fanin : g.gfanin()) {
		try {
			inbdds.push_back(bddmap.at(fanin));
		} catch (std::out_of_range&) {
			std::cout << "BDD for fanin: " << fanin << " not found\n";
			assert(0);
		}
	}

	return apply_op(mgr, inbdds, fun, limit);

}

BDD apply_op(Cudd& mgr, const bddvec& inputs, fnct op, uint limit) {

	BDD ret;
	switch (op) {
	case fnct::NAND:
	case fnct::AND: {
		BDD o = mgr.bddOne();
		for (auto& in: inputs) {
			o = o.And(in, limit);
			if (o.getNode() == NULL)
				return o;
		}
		ret = (op == fnct::AND) ? o : ~o;
		break;
	}
	case fnct::OR:
	case fnct::NOR: {
		BDD o = mgr.bddZero();
		for (auto& in: inputs) {
			o = o.Or(in, limit);
			if (o.getNode() == NULL)
				return o;
		}
		ret = (op == fnct::OR) ? o : ~o;
		break;
	}
	case fnct::XNOR: {
		ret = inputs[0].Xnor(inputs[1], limit);
		break;
	}
	case fnct::XOR: {
		ret = inputs[0].Xnor(inputs[1], limit);
		if (ret.getNode() != NULL)
			return ~ret;
		break;
	}
	case fnct::NOT: {
		ret = ~inputs[0];
		break;
	}
	case fnct::BUF: {
		ret = inputs[0];
		break;
	}
	default:
		neos_abort("unsupported BDD operation");
	}
	return ret;
}

void circuit_bdd::write_to_dot(const std::string& filename) {
	circuit_bdd::write_to_dot(filename, _mgr, outs, _vnames);
}

void circuit_bdd::write_to_dot(const std::string& filename, Cudd& mgr, const bddvec& nodes, const vector<string>& inames) {

	vector<ADD> addnodes;
	for (auto& nd : nodes) {
		addnodes.push_back(nd.Add());
	}

	FILE* fp = fopen(filename.c_str(), "w");
	char** cinames = new char* [inames.size()];
	for (int i = 0; i < inames.size(); i++) {
		std::cout << inames[i] << "\n";
		cinames[i] = new char[inames[i].size() + 1];
		for (int j = 0; j < inames[i].size(); j++)
			cinames[i][j] = inames[i][j];
		cinames[i][inames[i].size()] = 0;
		std::cout << cinames[i] << "\n";
	}

	if (inames.size() != 0) {
		mgr.DumpDot(addnodes, cinames, NULL, fp);
	}
	else {
		mgr.DumpDot(addnodes, NULL, NULL, fp);
	}


	for (int i = 0; i < inames.size(); i++) {
		delete [] cinames[i];
	}
	delete []cinames;



	fclose(fp);
}

int circuit_bdd::num_nodes(id oid) {
	if (oid == -1)
		return _mgr.nodeCount(outs);
	else
		return _mgr.nodeCount({_id2bdd_map.at(oid)});
}

std::string circuit_bdd::get_stats() {
	std::string ret;
	ret = "num nodes: " + std::to_string(num_nodes());
	return ret;
}

void circuit_bdd::to_circuit(circuit& cir) {

	if (outs.empty())
		return;

	circuit_bdd:to_circuit(cir, _mgr, _vnames, outs);
}

void circuit_bdd::to_circuit(circuit& cir, Cudd& mgr, const vector<string>& vnames, const bddvec& outs) {

	std::vector<ADD> addouts(outs.size()); // convert to add for circuit

	for (uint i = 0; i < outs.size(); i++) {
		addouts[i] = outs[i].Add();
	}

	DdManager *dd = addouts.at(0).manager();
	// std::cout << "circuit conversion\n num vars: " << dd->size << " num nodes: " << mgr.nodeCount(outs) << "\n";

	std::map<DdNode*, id> node2idmap;
	std::map<uint, id> index2id;

	for (uint i = 0 ; i < vnames.size(); i++) {
		if (vnames[i].rfind("keyinput", 0) == 0)
			index2id[i] = cir.add_wire(vnames[i], wtype::KEY);
		else
			index2id[i] = cir.add_wire(vnames[i], wtype::IN);
	}

	for (auto out : addouts) {
		DdGen *gen;
		DdNode *top = out.getNode();
		DdNode *node;
		// node2idmap[top] = cir.add_wire("out" + std::to_string(i++), wtype::OUT);
		Cudd_ForeachNode(dd, top, gen, node) {
			if (node == DD_ONE(dd)) {
				node2idmap[node] = cir.get_const1();
			}
			else if (node == DD_ZERO(dd)) {
				node2idmap[node] = cir.get_const0();
			}
			else {
				DdNode *e = Cudd_E(node), *t = Cudd_T(node);
				if (_is_not_in(e, node2idmap)) {
					node2idmap[e] = cir.add_wire(wtype::INTER);
				}
				if (_is_not_in(t, node2idmap)) {
					node2idmap[t] = cir.add_wire(wtype::INTER);
				}
				id muxout = cir.add_wire(wtype::INTER);
				node2idmap[node] = muxout;
				id sel = index2id[node->index];
				// std::cout << "index : " << node->index << " level: " << Cudd_Regular(node)->index << " sel: " << cir.wname(sel) << "\n";
				cir.add_gate({sel, node2idmap.at(e), node2idmap.at(t)}, muxout, fnct::MUX);
			}
		}

		cir.setwiretype(node2idmap.at(top), wtype::OUT);
	}

}


void test_bdd_ckt(const circuit& ckt) {
	circuit_bdd cktbdd(ckt);
	std::cout << "bdd statistics: \n";
	for (auto oid : ckt.outputs()) {
		std::cout << "for out " << ckt.wname(oid) << " num nodes: ";
		std::cout << cktbdd.num_nodes(oid) << "\n";
	}
	circuit bddcir;
	// cktbdd.write_to_dot("./bench/testing/bdd.dot");
	cktbdd.to_circuit(bddcir);
	bddcir.print_stats();
	// bddcir.write_bench();
}

} // namespace bdd



