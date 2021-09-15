/*
 * sym.cpp
 *
 *  Created on: Jan 11, 2021
 *      Author: kaveh
 */

#include "sat/sat.h"
#include "misc/sym.h"

#include "base/blocks.h"

namespace sym {

sym_var_bool_t& sym_mgr_t::new_bool_variable(const string& nm, bool frozen) {
	id vid = sym_vars.create_new_entry();
	auto b = std::make_shared<sym_var_bool_t>();
	b->mgr = this;
	b->frozen = frozen;
	b->varid = vid;
	b->name = nm == "" ? new_var_name():nm;
	sym_vars[vid] = std::static_pointer_cast<sym_var_t>(b);
	return *b;
}

sym_var_boolvec_t& sym_mgr_t::new_boolvec_variable(uint sz, const string& basenm, bool frozen) {
	id vid = sym_vars.create_new_entry();
	auto b = std::make_shared<sym_var_boolvec_t>();
	b->mgr = this;
	b->frozen = frozen;
	b->varid = vid;
	b->name = basenm == "" ? new_var_name():basenm;
	name2var[basenm] = vid;
	b->num_bits = sz;
	sym_vars[vid] = std::static_pointer_cast<sym_var_t>(b);
	return *b;
}

string sym_mgr_t::new_op_name() const {
	string ret;
	id i = sym_ops.size();
	do {
		ret = "op" + std::to_string(i++);
	} while (_is_in(ret, name2op));
	return ret;
}

string sym_mgr_t::new_var_name() const {
	string ret;
	id i = sym_vars.size();
	do {
		ret = "v" + std::to_string(i++);
	} while (_is_in(ret, name2var));
	return ret;
}


sym_op_t& sym_mgr_t::new_operation(sym_op_t::op_t ope, const idvec& fanins, const idvec& fanouts, const string& opnm) {
	id oid = sym_ops.create_new_entry();
	sym_op_t& b = sym_ops[oid];
	b.oid = oid;
	b.fanins = fanins;
	b.fanouts = fanouts;
	string nm = (opnm == "") ? new_op_name():opnm;
	name2op[nm] = oid;
	b.name = opnm;
	b.op = ope;
	std::cout << "adding op " << sym_op_t::op_enum2string(ope) << "\n";
	for (auto fo : fanouts) {
		getvar(fo).fanins.push_back(oid);
		std::cout << "fo: " << fo << "\n";
	}
	for (auto fin : fanins) {
		getvar(fin).fanouts.push_back(oid);
		std::cout << "fi: " << fin << "\n";
	}
	return b;
}

vector<string> sym_var_boolvec_t::enumerate_names() {
	vector<string> ret;
	for (uint i = 0; i < num_bits; i++) {
		ret.push_back(name + "_" + std::to_string(i));
	}
	return ret;
}

id bool_apply_op(const vector<sym_var_bool_t>& ins,
		sym_var_bool_t& y, sym_op_t::op_t ope) {
	auto& mgr = ins.at(0).getmgr();
	idvec inids;
	for (auto& x : ins) {
		inids.push_back(x.varid);
	}
	auto& op = mgr.new_operation(ope, inids, {y.varid});
	return op.oid;
}

id boolvec_apply_op(const vector<sym_var_boolvec_t>& ins, sym_var_boolvec_t& y, sym_op_t::op_t ope) {
	auto& mgr = ins.at(0).getmgr();
	uint sz = ins.at(0).num_bits;
	idvec fanins;
	for (auto& x : ins) {
		assert(sz == x.num_bits);
		assert(&mgr == &x.getmgr());
		fanins.push_back(x.varid);
	}
	auto& op = mgr.new_operation(ope, fanins, {y.varid});
	return op.oid;
}

id boolvec_apply_op(const vector<sym_var_boolvec_t>& ins, sym_var_bool_t& y, sym_op_t::op_t ope) {
	auto& mgr = ins.at(0).getmgr();
	uint sz = ins.at(0).num_bits;
	idvec fanins;
	for (auto& x : ins) {
		assert(sz == x.num_bits);
		fanins.push_back(x.varid);
	}
	auto& op = mgr.new_operation(ope, fanins, {y.varid});
	return op.oid;
}

sym_var_bool_t operator&(const sym_var_bool_t& a, const sym_var_bool_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	bool_apply_op({a, b}, y, sym_op_t::AND);
	return y;
}

sym_var_bool_t operator|(const sym_var_bool_t& a, const sym_var_bool_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	bool_apply_op({a, b}, y, sym_op_t::OR);
	return y;
}

sym_var_bool_t operator^(const sym_var_bool_t& a, const sym_var_bool_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	bool_apply_op({a, b}, y, sym_op_t::XOR);
	return y;
}

sym_var_bool_t operator+(const sym_var_bool_t& a, const sym_var_bool_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	bool_apply_op({a, b}, y, sym_op_t::AND);
	return y;
}

sym_var_bool_t operator~(const sym_var_bool_t& a) {
	sym_var_bool_t y;
	bool_apply_op({a}, y, sym_op_t::NOT);
	return y;
}

/*
 *  BOOLVEC operations
 */
sym_var_boolvec_t operator+(const sym_var_boolvec_t& a) {
	uint num_outs = (uint)std::log2(a.num_bits) + 1;
	sym_var_boolvec_t y = a.getmgr().new_boolvec_variable(num_outs);
	boolvec_apply_op({a}, y, sym_op_t::AND);
	return y;
}

sym_var_boolvec_t bit_sum(const sym_var_boolvec_t& a) {
	return +a;
}

sym_var_boolvec_t operator&(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_boolvec_t y;
	boolvec_apply_op({a, b}, y, sym_op_t::AND);
	return y;
}

sym_var_boolvec_t operator|(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_boolvec_t y;
	boolvec_apply_op({a, b}, y, sym_op_t::OR);
	return y;
}

sym_var_boolvec_t operator^(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_boolvec_t y = a.getmgr().new_boolvec_variable(a.num_bits);
	boolvec_apply_op({a, b}, y, sym_op_t::XOR);
	return y;
}

sym_var_boolvec_t operator+(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_boolvec_t y = a.getmgr().new_boolvec_variable(a.num_bits);
	boolvec_apply_op({a, b}, y, sym_op_t::ADD);
	return y;
}

sym_var_boolvec_t operator~(const sym_var_boolvec_t& a) {
	sym_var_boolvec_t y = a.getmgr().new_boolvec_variable(a.num_bits);
	boolvec_apply_op({a}, y, sym_op_t::NOT);
	return y;
}

sym_var_bool_t operator==(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	boolvec_apply_op({a, b}, y, sym_op_t::EQ);
	return y;
}

sym_var_bool_t operator!=(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_bool_t y = (a == b);
	return ~y;
}

sym_var_bool_t operator>(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	boolvec_apply_op({a, b}, y, sym_op_t::GR);
	return y;
}

sym_var_bool_t operator>=(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	boolvec_apply_op({a, b}, y, sym_op_t::GEQ);
	return y;
}

sym_var_bool_t operator<(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	boolvec_apply_op({b, a}, y, sym_op_t::GR);
	return y;
}

sym_var_bool_t operator<=(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b) {
	sym_var_bool_t y = a.getmgr().new_bool_variable();
	boolvec_apply_op({b, a}, y, sym_op_t::GEQ);
	return y;
}

sym_var_boolvec_t& apply_cir(const sym_var_boolvec_t& x, const sym_var_boolvec_t& k, const circuit& cir) {
	auto& mgr = x.getmgr();
	assert(&x.getmgr() == &k.getmgr());
	assert(x.num_bits == cir.nInputs());
	assert(k.num_bits == cir.nKeys());
	sym_var_boolvec_t y = mgr.new_boolvec_variable(cir.nOutputs());
	auto& op = mgr.new_operation(sym_op_t::CIR, {x.varid, k.varid}, {y.varid});
	mgr.sym_ops[op.oid].attchmap[sym_op_t::ATTCH_CIR] = cir;
	std::cout << "added op with id " << op.oid << "\n";
	return mgr.getvar<sym_var_boolvec_t>(y.varid);
}


void sym_var_t::to_circuit(circuit& rcir, Hmap<id, idvec> v2wmap) const {

	assert(mgr);

	for (auto p : mgr->sym_vars) {
		auto& vr = *p.second;
		for (auto& nm : vr.enumerate_names()) {
			v2wmap[vr.varid].push_back( rcir.add_wire(nm, wtype::INTER) );
		}
		if (vr.fanins.empty()) {
			for (auto xid : v2wmap.at(vr.varid))
				rcir.setwiretype(xid, wtype::IN);
		}
	}

	for (auto wid : v2wmap.at(varid)) {
		rcir.setwiretype(wid, wtype::OUT);
	}

	for (auto p : mgr->sym_ops) {
		auto& op = p.second;
		op_to_circuit(op, rcir, v2wmap);
	}
}

vector<sym_var_bool_t> split_boolvec(const sym_var_boolvec_t& bvar) {
	auto& mgr = bvar.getmgr();
	vector<sym_var_bool_t> retvec;
	idvec retids;
	for (uint i = 0; i < bvar.num_bits; i++) {
		auto v = mgr.new_bool_variable(bvar.name + "_" + std::to_string(i));
		retvec.push_back( v );
		retids.push_back( v.varid );
	}
	auto& op = mgr.new_operation(sym_op_t::SPLIT, {bvar.varid}, retids);
	return retvec;
}

id sym_mgr_t::find_var(const string& nm) {
	auto it = name2var.find(nm);
	if (it == name2var.end())
		return -1;
	return it->second;
}

id sym_mgr_t::find_op(const string& nm) {
	auto it = name2op.find(nm);
	if (it == name2op.end())
		return -1;
	return it->second;
}

void sym_mgr_t::set_varname(id varid, const string& nm) {
	auto& var = getvar(varid);
	name2var.erase(var.name);
	name2var[nm] = varid;
	var.name = nm;
}

void sym_mgr_t::set_opname(id opid, const string& nm) {
	auto& op = getop(opid);
	name2var.erase(op.name);
	name2var[nm] = opid;
	op.name = nm;
}

void sym_mgr_t::remove_var(id varid) {
	auto& var = getop(varid);
	name2var.erase(var.name);
	for (auto fo : var.fanouts) {
		utl::erase_byval(getvar(fo).fanins, varid);
	}
	for (auto fi : var.fanins) {
		utl::erase_byval(getvar(fi).fanouts, varid);
	}
	sym_vars.erase(varid);
}

void sym_mgr_t::remove_op(id opid) {
	auto& op = getop(opid);
	name2op.erase(op.name);
	for (auto fo : op.fanouts) {
		utl::erase_byval(getvar(fo).fanins, opid);
	}
	for (auto fi : op.fanins) {
		utl::erase_byval(getvar(fi).fanouts, opid);
	}
	sym_ops.erase(opid);
}

// remove one hierarchy level: replace cir/aig with their boolean structures
void sym_mgr_t::flatten_cir_op(sym_op_t& op) {
	for (auto p : sym_ops) {
		auto& op = p.second;
		if (op.op == sym_op_t::CIR) {
			assert(op.fanins.size() == 2);
			sym_var_boolvec_t x = getvar<sym_var_boolvec_t>(op.fanins[0]);
			sym_var_boolvec_t k = getvar<sym_var_boolvec_t>(op.fanins[1]);
			sym_var_boolvec_t y = getvar<sym_var_boolvec_t>(op.fanouts[0]);
			vector<sym_var_bool_t> xvec = split_boolvec(x);
			vector<sym_var_bool_t> kvec = split_boolvec(k);

			const circuit& cir = std::any_cast<circuit&>(op.attchmap.at(sym_op_t::ATTCH_CIR));

			for (auto xid : cir.inputs()) {

			}
		}
	}
}


void sym_var_t::op_to_circuit(const sym_op_t& op, circuit& rcir,
		const Hmap<id, idvec>& v2wmap) const {

	auto& fanins = op.fanins;
	auto& fanouts = op.fanouts;

	switch (op.op) {
	case sym_op_t::ADD : {
		// N-bit binary additions
		if (fanins.size() == 2) {
			id2idmap added2new;
			uint sz = v2wmap.at(fanins.at(0)).size();
			idvec avec, bvec, svec;
			circuit vec_adder = ckt_block::vector_adder(sz, avec, bvec, svec);
			for (uint i = 0; i < sz; i++) {
				added2new[avec[i]] = v2wmap.at(fanins[0])[i];
				added2new[bvec[i]] = v2wmap.at(fanins[1])[i];
				added2new[svec[i]] = v2wmap.at(fanouts[0])[i];
			}
			rcir.add_circuit(vec_adder, added2new);
			sym_mgr_t::buf_to_other_fanouts(rcir, op, v2wmap);
		}
		// bit counting
		else if (fanins.size() == 1) {
			id2idmap added2new;
			uint sz = v2wmap.at(fanins[0]).size();
			circuit bit_adder = ckt_block::bit_counter(sz);
			idvec adderins = utl::to_vec(bit_adder.inputs());
			for (uint i = 0; i < sz; i++) {
				added2new[adderins[i]] = v2wmap.at(fanins[0])[i];
			}
			rcir.add_circuit(bit_adder, added2new);
			sym_mgr_t::buf_to_other_fanouts(rcir, op, v2wmap);
		}
		break;
	}
	case sym_op_t::NOT : {
		assert(op.fanins.size() == 1);
		for (uint i = 0; i < v2wmap.at(fanins[0]).size(); i++) {
			rcir.add_gate({v2wmap.at(fanins[0])[i]}, v2wmap.at(fanouts[0])[i], fnct::NOT);
		}

		for (uint i = 1; i < fanouts.size(); i++) {
			rcir.add_gate({v2wmap.at(fanouts[0])[i]}, v2wmap.at(fanouts[i])[i], fnct::BUF);
		}
		break;
	}
	case sym_op_t::XOR :
	case sym_op_t::OR :
	case sym_op_t::AND : {
		if (fanins.size() == 1) {

		}
		else {
			vector<idvec> gateins;
			sym_mgr_t::get_gate_inputs(op, v2wmap, gateins);
			fnct fn;
			if (op.op == sym_op_t::AND) fn = fnct::AND;
			else if (op.op == sym_op_t::OR) fn = fnct::OR;
			else if (op.op == sym_op_t::OR) fn = fnct::XOR;

			for (uint i = 0; i < gateins.size(); i++) {
				rcir.add_gate(gateins[i], v2wmap.at(fanouts[0])[i], fn);
			}
			sym_mgr_t::buf_to_other_fanouts(rcir, op, v2wmap);
		}
		break;
	}
	case sym_op_t::EQ: {
		vector<idvec> gateins;
		sym_mgr_t::get_gate_inputs(op, v2wmap, gateins);
		idvec xor_outs;
		for (uint i = 0; i < gateins.size(); i++) {
			id xor_out = rcir.add_wire(wtype::INTER);
			rcir.add_gate(gateins[i], xor_out, fnct::XNOR);
			xor_outs.push_back(xor_out);
		}
		rcir.add_gate(xor_outs, v2wmap.at(fanouts[0])[0], fnct::AND);
		break;
	}
	case sym_op_t::GR: {
		if (fanins.size() == 2) {
			id2idmap added2new;
			uint sz = v2wmap.at(fanins.at(0)).size();
			idvec avec, bvec;
			id agb;
			circuit vec_adder = ckt_block::vector_half_comparator(sz, avec, bvec, agb);
			for (uint i = 0; i < sz; i++) {
				added2new[avec[i]] = v2wmap.at(fanins[0])[i];
				added2new[bvec[i]] = v2wmap.at(fanins[1])[i];
			}
			added2new[agb] = v2wmap.at(fanouts[0])[0];
			rcir.add_circuit(vec_adder, added2new);
			sym_mgr_t::buf_to_other_fanouts(rcir, op, v2wmap);
		}
		break;
	}
	default:
		neos_error("unsupported covertion to circuit operation");
		break;
	}

}

void sym_mgr_t::buf_to_other_fanouts(circuit& rcir, const sym_op_t& op, const Hmap<id, idvec>& v2wmap) {
	for (uint i = 1; i < op.fanouts.size(); i++) {
		rcir.add_gate({v2wmap.at(op.fanouts[0])[i]}, v2wmap.at(op.fanouts[i])[i], fnct::BUF);
	}
}

void sym_mgr_t::get_gate_inputs(const sym_op_t& op, const Hmap<id, idvec>& v2wmap, vector<idvec>& gateins) {
	uint sz = v2wmap.at(op.fanins[0]).size();
	gateins.resize(sz);
	for (uint i = 0; i < sz; i++) {
		for (uint j = 0; j < op.fanins.size(); j++) {
			gateins[i].push_back(v2wmap.at(op.fanins[j])[i]);
		}
	}
}

void sym_var_t::write_graph() const {
	idque Q;
	Q.push(varid);
	idset visited = {varid};

	string in_str, gate_str;

	in_str = "OUTPUT(" + name + ")\n";

	assert(mgr);

	while ( !Q.empty() ) {
		id curid = Q.front(); Q.pop();
		auto& cv = mgr->getvar(curid);
		if (cv.fanins.empty()) {
			in_str += "INPUT(" + cv.name + ")\n";
		}
		else {
			id opid = cv.fanins.at(0);
			auto& op = mgr->getop(opid);

			vector<string> gate_ins;
			for (auto opin : op.fanins) {
				if (_is_not_in(opin, visited)) {
					gate_ins.push_back(mgr->getvar(opin).name);
					visited.insert(opin);
					Q.push(opin);
				}
			}

			gate_str += cv.name + " = " + sym_op_t::op_enum2string(op.op) + "("
						+ utl::to_delstr(gate_ins, ", ") + ")\n";
		}
	}

	std::cout << in_str;
	std::cout << gate_str;
}

} // namespace sym
