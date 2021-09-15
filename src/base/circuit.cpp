
#include "base/circuit.h"
#include "boost/tokenizer.hpp"

//////////////// The circuit class ////////////////////

namespace ckt {

using namespace utl;

std::ostream& operator<<(std::ostream& out, tern_t t) {
	switch(t.val) {
	case tern_t::X:
		out << "x";
		break;
	case tern_t::ONE:
		out << "1";
		break;
	case tern_t::ZERO:
		out << "0";
		break;
	default:
		neos_abort("bad ternary value " << t.val);
		break;
	}
	return out;
}

// reconstruct circuit with placement new ... hmmm
void circuit::clear(void) {
	this->~circuit();
	new (this) circuit();
}

void circuit::print_ports() const {
	for (auto xid : inputs()) {
		std::cout << "INPUT( " << wname(xid) << " )\n";
	}
	for (auto kid : keys()) {
		std::cout << "KEY( " << wname(kid) << " )\n";
	}
	for (auto oid : outputs()) {
		std::cout << "OUTPUT( " << wname(oid) << " )\n";
	}
}

void circuit::print_stats(void) const {
	std::cout << "   #inputs: " << nInputs();
	std::cout << "   #output: " << nOutputs();
	std::cout << "   #dffs: " << nLatches();
	std::cout << "   #keys: " << nKeys();
	std::cout << "   #gates: " << nGates() << "\n";
}

void circuit::print_gate(id gid) const {
	const auto& g = getcgate(gid);
	std::cout << gname(gid) << " : " << wname(g.gfanout())
			<< " = " << funct::enum_to_string(g.gfunction()) << " (";
	for (auto fanin : g.gfanin())
		std::cout << " " << wname(fanin);
	std::cout << ")\n";
}



/*
 * this open wire function returns a pair (left_wire, right_wire)
 * depending on the wire type of wr.
 */
idpair circuit::open_wire(id wr, const string& str) {

	id right_wire, left_wire;
	string nnm = (str != "") ? str : get_new_wname();

	if (!isOutput(wr)) {

		left_wire = wr;
		right_wire = add_wire(nnm, wtype::INTER);

		idvec leftwfanouts= getwire(left_wire).fanouts;
		for (auto gid : leftwfanouts) {
			getwire(right_wire).addfanout(gid);
			for (auto& y : getgate(gid).fanins) {
				if (y == left_wire) {
					y = right_wire;
				}
			}
		}
		getwire(left_wire).fanouts.clear();
	}
	else {
		right_wire = wr;
		left_wire = add_wire(nnm, wtype::INTER);

		for (auto& g : getwire(right_wire).wfanin()) {
			getgate(g).setfanout(left_wire);
		}
		getwire(left_wire).fanins = getwire(right_wire).fanins;
		getwire(right_wire).fanins.clear();
	}

	return {left_wire, right_wire};
}

/*
 * opens wire at the branch that leads to gate gid
 */
idpair circuit::open_wire_at_branch(id wr, id gid, const string& str){

	id right_wire, left_wire;
	idpair retpair;

	if (wrtype(wr) != wtype::OUT){

		left_wire = wr;
		string nnm = (str != "") ? str : get_new_wname();
		right_wire = add_wire(nnm, wtype::INTER);

		update_gateFanin(gid, left_wire, right_wire);
		getwire(right_wire).addfanout(gid);
		getwire(left_wire).removefanout(gid);

		for (auto& x : getgate(gid).fanins) {
			if (x == left_wire) {
				x = right_wire;
			}
		}
	}
	else {
		// there is no branch node for output wires
		return open_wire(wr, str);
	}

	retpair.first = left_wire;
	retpair.second = right_wire;

	return retpair;

}

void circuit::add_constants(idset& inset, const circuit& cir) {
	if (cir.has_const0())
		inset.insert(cir.get_cconst0());
	if (cir.has_const1())
		inset.insert(cir.get_cconst1());
}

void circuit::addWire_to_gateFanin(id gid, id wr) {
	if (_is_in(wr, getcgate(gid).fanins)) {
		std::cout << "warning: wire already exists in fanin\n";
		return;
	}
	getgate(gid).addfanin(wr);
	getwire(wr).addfanout(gid);
}

void circuit::remove_gatefanin(id gid, id wid) {
	erase_byval(getgate(gid).fanins, wid);
	getwire(wid).removefanout(gid);
}

void circuit::update_gateFanin(id gid, id wr_old, id wr_new) {
	auto& gate_fanin = getgate(gid).fanins;
	for (uint i = 0 ; i < gate_fanin.size(); i++) {
		if (gate_fanin[i] == wr_old) {
			gate_fanin[i] = wr_new;
		}
	}
}

const idvec& circuit::gfanoutGates(id gid) const {
	id wid = gfanout(gid);
	return wfanout(wid);
}

idset circuit::gfaninGates(id gid) const {
	idset retset;
	for (auto wid : gfanin(gid))
		for (auto gid2 : wfanins(wid))
			retset.insert(gid2);
	return retset;
}

idset circuit::wfaninWires(id wr) const {
	idset retset;
	for (auto gid : wfanins(wr)) {
		add_all(retset, gfanin(gid));
	}
	return retset;
}

idset circuit::wfanoutWires(id wr) const {
	idset retset;
	for (auto gid : wfanout(wr)) {
		retset.insert(gfanout(gid));
	}
	return retset;
}

void circuit::unregister_wiretype(id wid, wtype wt) {
	switch (wt) {
	case wtype::OUT :
		prim_output_set.erase(wid);
		break;
	case wtype::IN :
		prim_input_set.erase(wid);
		break;
	case wtype::KEY :
		key_input_set.erase(wid);
		break;
	default :
		break;
	}
}

void circuit::register_wiretype(id wid, wtype wt) {
	switch (wt) {
	case wtype::IN :
		 prim_input_set.insert(wid);
		 break;
	case wtype::INTER :
		 break;
	case wtype::OUT :
		 prim_output_set.insert(wid);
		 break;
	case wtype::KEY :
		 key_input_set.insert(wid);
		 break;
	default: break;
	}
}

// add new wire
id circuit::add_wire(const string& str, wtype wt, id wid, bool const_check) {

	if (str == "")
		return add_wire(wt, wid);

	if (const_check) {
		if (str == "gnd")
			return get_const0();
		else if (str == "vdd")
			return get_const1();
	}

	// remove this line for performance
	if (node_names.is_in_seconds(str))
		neos_abort("wire name " << str << " already exists");

	wid = add_wire(wt, wid);

	node_names.add_pair(wid, str);

	return wid;
}

id circuit::add_wire(wtype wt, id wid) {

	if (wid == -1)
		wid = nodes.create_new_entry(nlnode(nltype::WIRE), (int8_t)nltype::WIRE);
	else {
		assert(!wire_exists(wid));
		nodes.insert_entry(wid, nlnode(nltype::WIRE), (int8_t)nltype::WIRE);
	}

	//assert(!node_names.is_in_firsts(wid));
	auto& w = nodes[wid];
	w.nid = wid;
	w.ncat0 = nltype::WIRE;
	w.ncat1 = (int8_t)wt;

	register_wiretype(wid, wt);
	clear_topsort();

	return wid;
}

// add wire only if the name does not exist
id circuit::add_wire_necessary(const string& str, wtype wt) {
	id ret = find_wire(str);
	if (ret != -1)
		return ret;
	return add_wire(str, wt);
}

// add wire with name and object
id circuit::add_wire(const string& str, const wire& wr) {

	assert(!wire_exists(wr.nid));
	assert(wr.isWire());

	nodes.insert_entry(wr.nid, wr, (int8_t)nltype::WIRE);
	node_names.add_pair(wr.nid, str);

	register_wiretype(wr.nid, wr.wrtype());

	clear_topsort();

	return wr.nid;
}

void circuit::add_word(const string& base, int left, int right, wtype wt) {
	wordmap[base].left = left;
	wordmap[base].right = right;

	bool dir = (left > right);

	int i = left;
	while (true) {
		wordmap[base].wids.push_back(add_wire(fmt("\\%s[%d]", base % i), wt));
		if (i == right)
			break;
		i += (dir) ? -1:1;
	}
}

void circuit::add_word_necessary(const string& base, int left, int right, wtype wt) {
	wordmap[base].left = left;
	wordmap[base].right = right;

	bool dir = (left > right);

	int i = left;
	while (true) {
		wordmap[base].wids.push_back(add_wire_necessary(fmt("\\%s[%d]", base % i), wt));
		if (i == right)
			break;
		i += (dir) ? -1:1;
	}
}


bool circuit::get_word(const string& base, word_t& wrd) const {
	if ( _is_in(base, wordmap) ) {
		wrd = wordmap.at(base);
		return true;
	}
	else {
		return false;
	}
}

id circuit::add_gate(const string& str, const gate& g) {
	return add_gate(str, g.fanins, g.fo0(), g.gfun());
}

id circuit::add_gate(const idvec& fanins, id fanout, fnct fun) {

	id gid = nodes.create_new_entry(nlnode(nltype::GATE), (int8_t)nltype::GATE);

	nodes[gid]= gate(gid, nltype::GATE, fanins, {fanout}, (int8_t)fun);

	for (auto x : fanins) {
		getwire(x).addfanout(gid);
	}
	getwire(fanout).addfanin(gid);

	if (fun == fnct::DFF)
		dffs.insert(gid);

	clear_topsort();

	return gid;
}


id circuit::add_gate(const string& str, const idvec& fanins,
		id fanout, fnct fun) {

	if (str == "")
		return add_gate(fanins, fanout, fun);

	if (node_names.is_in_seconds(str)) {
		neos_error("gate named " << str << " already exists.");
	}

	id gid = add_gate(fanins, fanout, fun);

	node_names.add_pair(gid, str);

	return gid;
}

idvec circuit::add_gate_word(const vector<idvec>& fanin_words, fnct fun) {
	uint wordlen = fanin_words.at(0).size();
	idvec fanout_word;
	for (uint i = 0; i < wordlen; i++) {
		fanout_word.push_back(add_wire(wtype::INTER));
	}
	add_gate_word(fanin_words, fanout_word, fun);
	return fanout_word;
}

idvec circuit::add_gate_word(const vector<idvec>& fanin_words,
		const idvec& fanout_word, fnct fun) {
	uint wordlen = fanin_words.at(0).size();
	for (auto& x : fanin_words) {
		assert(x.size() == wordlen);
	}
	assert(fanout_word.size() == wordlen);

	idvec gatevec;

	for (uint wi = 0; wi < wordlen; wi++) {
		idvec fanins;
		for (uint fin = 0; fin < fanin_words.size(); fin++) {
			fanins.push_back(fanin_words[fin][wi]);
		}
		id fanout = fanout_word[wi];
		gatevec.push_back(add_gate(fanins, fanout, fun));
	}

	return gatevec;
}

void circuit::_resolve_wire_array(string instr, wtype wt) {

	boost::char_separator<char> sep("[:]");
	boost::tokenizer<boost::char_separator<char> > tokens(instr, sep);

	auto it = tokens.begin();

	string wirename = *it;
	uint bound1 = std::stoi(*(++it));
	uint bound2 = std::stoi(*(++it));

	uint low = MIN(bound1, bound2);
	uint high = MAX(bound1, bound2);

	for (uint i = low; i <= high; i++) {
		string wname = wirename + '[' + std::to_string(i) + ']';
		add_wire(wname, wt);
	}
}

bool circuit::reparse(void){

	std::stringstream sstr;
	write_bench((std::ostream&)sstr);
	clear();
	read_bench((std::istream&)sstr);

	return true;
}


void circuit::error_check_assert() const {
	if (!error_check()) {
		write_bench("error_cir.bench");
		assert(false);
	}
}

/*
 * time consuming error check. constant * circuit_size
 * should not be used in performance critical loops
 */
#define error_blocking(message) \
			{ std::cout << message << "\n"; \
			error = true; \
			return !error; }

#define error_nonblocking(message) \
		{ std::cout << message <<  "\n"; \
		error = true;}

bool circuit::error_check(void) const {

	idset g_fanouts;
	bool error = false;

	for (auto xid : inputs()) {
		if (!isInput(xid)) error_blocking("wire " << wname(xid)
				<< " is in input set but has noninput type");
	}
	for (auto xid : outputs()) {
		if (!isOutput(xid)) error_blocking("wire " << wname(xid)
				<< " is in output set but has non-output type");
	}
	for (auto xid : keys()) {
		if (!isKey(xid)) error_blocking("wire " << wname(xid)
				<< " is in key set but has nonkey type");
	}


	foreach_wire(*this, w, wid) {
		if (w.isInput() && _is_not_in(wid, inputs())) {
			error_blocking("netlist error: wire type for " << wname(wid) << " input but wid not in input set");
		}
		else if (w.isOutput() && _is_not_in(wid, outputs())) {
			error_blocking("netlist error: wire type for " << wname(wid) << " output but wid not in output set");
		}
		else if (w.isKey() && _is_not_in(wid, keys())) {
			error_blocking("netlist error: wire type for " << wname(wid) << " key but wid not in key set");
		}
		else {
			if (w.isInter() && _is_in(wid, inputs())) {
				error_blocking("netlist error: wire type is inter for " << wname(wid) << " but wid in input set ");
			}
			else if (w.isInter() && _is_in(wid, outputs())) {
				error_blocking("netlist error: wire type is inter for " << wname(wid) << " but wid in output set ");
			}
			else if (w.isInter() && _is_in(wid, outputs())) {
				error_blocking("netlist error: wire type is inter for " << wname(wid) << " but wid in key set ");
			}
		}
	}

	// ensure no floating internal or output wires
	foreach_gate(*this, g, gid) {

		if (gid != g.nid) {
			error_blocking("netlist error: gate \"" << gname(gid)
					<< "\" has id mismatch : " << gid << " != " << g.nid);
		}

		g_fanouts.insert(gfanout(gid));
		for (auto fo : g.fanouts) {
			if (!wire_exists(fo)) {
				error_blocking("netlist error: wire " << fo << " referenced in \"" << gname(gid)
										<< "\" does not exist \n");
			}
		}
		if (gfanout(gid) == -1) {
			error_blocking("netlist error: gate \"" << gname(gid)
					<< "\" has fanout -1\n");
		}
		for (auto gin : g.fanins) {
			if (!wire_exists(gin)) {
				error_blocking("netlist error: wire " << gin << " referenced in \"" << gname(gid)
										<< "\" does not exist \n");
			}
			if (gin == -1) {
				error_blocking("netlist error: gate \"" << gname(gid)
						<< "\" has -1 in fanin\n");
			}
			if (_is_not_in(gid, wfanout(gin))) {
				error_blocking("netlist error: gate \"" << gname(gid)
									<< "\" fanin wire " << wname(gin)
									<< " does not include its connection to the gate\n");
			}
		}
	}


	// ensure no floating internal or output wires
	foreach_instance(*this, g, gid) {

		for (auto fo : g.fanouts) {
			if (!wire_exists(fo)) {
				error_blocking("netlist error: wire " << fo << " referenced in \"" << ndname(gid)
										<< "\" does not exist \n");
			}
		}

		for (auto gin : g.fanins) {
			if (!wire_exists(gin)) {
				error_blocking("netlist error: wire " << gin << " referenced in \"" << ndname(gid)
										<< "\" does not exist \n");
			}
			if (_is_not_in(gid, getcwire(gin).fanouts)) {
				error_blocking("netlist error: instnace \"" << ndname(gid)
									<< "\" fanin wire " << ndname(gin)
									<< " does not include its connection to the gate\n");
			}
		}
	}


	if (nInst() != 0)
		goto l1;

	foreach_wire(*this, w, wid) {
		if (wid != w.nid) {
			error_blocking("netlist error: wire has id mismatch : "
					<< w.nid << " != " << wid << " \n");
		}

		if ( !isInput(wid) && !isKey(wid) && !isConst(wid) ) {
			if (_is_not_in(wid, g_fanouts) ) {
				error_blocking("netlist error: wire \"" << wname(wid)
						   << "\" never assigned\n");
			}
			if (_is_in(-1, w.fanins)) {
				error_blocking("error: wire \"" << wname(wid)
						<< "\" fanin struct: {";
				std::cout << to_delstr(w.fanins, " ") << "}\n");
			}
		}
	}

	l1:
	foreach_wire((*this), w, wid) {
		if (wfanins(wid).size() > 1) {
			for (auto faning : wfanins(wid)) {
				if (gfunction(faning) != fnct::TBUF) {
					std::cout << "netlist error: net " << wname(wid) << " driven by multiple non-tri-state drivers\n";
					error = true;
					break;
				}
			}
		}
	}

	// ensure wire fanout idset is correct
	foreach_gate(*this, g, gid) {
		for (auto& y : gfanin(gid)) {
			if ( _is_not_in(gid, wfanout(y)) ) {
				std::cout << "netlist error: fanout of wire " << wname(y)
						<< " does not include its connection to gate "
						<< gname(gid) << " -> " << wname(g.fo0()) << "!\n";
				error = true;
			}
		}
	}

	// ensure wire fanins are correct
	foreach_wire(*this, w, wid) {
		for (auto gid : w.fanouts){
			bool found = false;
			for (auto z : gfanin(gid)){
				if (z == wid) {
					found = true;
				}
			}
			if (!found) {
				std::cout << "netlist error: wire " << wname(wid)
						<<" fanout structure points to gate that does not connect to it :\n";
				print_gate(gid);

				error = true;
			}
		}
		for (auto y : wfanins(wid)) {
			if (gfanout(y) != wid) {
				std::cout << "netlist error: wire " << wname(wid)
						<< " fanin structure does not match the driving gates' fanout\n";
			}
		}
	}

	return !error;
}

id circuit::add_key(void){
	string str = "keyinput" + std::to_string(nKeys());
	return add_wire(str, wtype::KEY);
}

void circuit::internal_merge_wires(id wr1, id wr2) {
	id to_remove = wr1, to_remain = wr2;
	// internal wire is connected to port
	if (wrtype(wr1) == wtype::INTER) {
		to_remove = wr1;
		to_remain = wr2;
	}
	else if (wrtype(wr2) == wtype::INTER) {
		to_remove = wr2;
		to_remain = wr1;
	}
	else {
		neos_error("port to port wire-merge "
				<< "will result in port loss...\n");
	}

	merge_wires(to_remain, to_remove);
}

/*
 * merge routine for merging equivalent wires
 * used in sweeping
 */
void circuit::merge_equiv_nodes(id to_remain, id to_remove) {

	assert(to_remain != to_remove);

	auto& tr = getwire(to_remove);
	auto& tk = getwire(to_remain);

	if (tr.isOutput()) {
		if (tk.isOutput()) {
			idpair p = open_wire(to_remove);
			add_gate({to_remain}, p.second, fnct::BUF);
			return;
		}
		else {
			setwiretype(to_remain, wtype::OUT);
			setwirename(to_remain, wname(to_remove));
		}
	}

	for (auto& gid : tr.fanouts) {
		update_gateFanin(gid, to_remove, to_remain);
		tk.addfanout(gid);
	}

	remove_wire(to_remove);
}

void circuit::move_fanouts_to(id from, id to) {
	auto& wr_from = getwire(from);
	auto& wr_to = getwire(to);

	// update neighbor gates fanins/fanouts
	for (auto gid : wr_from.fanouts) {
		update_gateFanin(gid, from, to);
	}

	// update keep fanouts
	idset wr_to_fouts = utl::to_hset(wr_to.fanouts);
	utl::add_all(wr_to_fouts, wr_from.fanouts);
	wr_to.fanouts = utl::to_vec(wr_to_fouts);


	wr_from.fanouts.clear();
}

void circuit::merge_wires(id to_remain, id to_remove) {

	auto& wr_del = getwire(to_remove);
	auto& wr_keep = getwire(to_remain);

	// update neighbor gates fanins/fanouts
	for (auto b : {0, 1}) {
		for (auto gid : (b ? wr_del.fanins : wr_del.fanouts)) {
			auto& g = getgate(gid);
			for (auto& x : (b ? g.fanouts : g.fanins)) {
				if (x == to_remove)
					x = to_remain;
			}
		}
	}

	// update keep fanouts
	idset wr_keep_fouts = utl::to_hset(wr_keep.fanouts);
	utl::add_all(wr_keep_fouts, wr_del.fanouts);
	wr_keep.fanouts = utl::to_vec(wr_keep_fouts);


	if (!wr_del.fanins.empty()) {
		// merging of two wires with non-empty fanins is wrong
		// must use move_fanouts instead
		assert(wr_keep.fanins.empty());
		wr_keep.fanins = wr_del.fanins;
	}

	wr_del.fanins.clear();
	wr_del.fanouts.clear();

	remove_wire(to_remove);
}


void circuit::add_circuit_byname(char option,
		const circuit& cir, const string& postfix) {

	id2idmap added2new;

	if ((option & OPT_INP) != 0) {
		assert (nInputs() == cir.nInputs());
		for (auto& x : cir.inputs())
			added2new[x] = find_wcheck(cir.wname(x));
	}

	if ((option & OPT_KEY) != 0) {
		assert (nKeys() == cir.nKeys());
		for (auto& x : cir.keys())
			added2new[x] = find_wcheck(cir.wname(x));
	}

	if ((option & OPT_OUT) != 0) {
		assert (nOutputs() == cir.nOutputs());
		for (auto& x : cir.outputs())
			added2new[x] = find_wcheck(cir.wname(x));
	}

	add_circuit(cir, added2new, postfix);

}

void circuit::add_circuit(const circuit& cir, id2idmap& added2new, const string& postfix) {

	const circuit *cpt = &cir;
	if (this == &cir) {
		cpt = new circuit(cir);
	}

	if (cpt->has_const0())
		added2new[cpt->get_cconst0()] = get_const0();
	if (cpt->has_const1())
		added2new[cpt->get_cconst1()] = get_const1();

	foreach_wire(*cpt, w, wid) {
		if (_is_not_in(wid, added2new)) {
			string nm = (postfix != "") ?  (cir.wname(wid) + postfix) : "";
			added2new[wid] = add_wire(nm, w.wrtype());
		}
	}

	foreach_gate(*cpt, g, gid) {
		add_gate(g, added2new);
	}

	if (this == &cir) {
		delete cpt;
	}

}

void circuit::add_circuit(char option,
		const circuit& cir, const string& postfix) {
	id2idmap ckt2newmap;
	add_circuit(option, cir, ckt2newmap, postfix);
}

void circuit::add_circuit(char option,
		const circuit& cir, id2idmap& added2new, const string& postfix) {

	if ((option & OPT_INP) != 0) {
		assert (nInputs() == cir.nInputs());
		oidset ordered_ins = inputs();
		auto it = cir.inputs().begin();
		for (auto x : inputs())
			added2new[x] = *it++;
	}

	if ((option & OPT_KEY) != 0) {
		assert (nKeys() == cir.nKeys());
		auto it = cir.keys().begin();
		for (auto x : keys())
			added2new[x] = *it++;
	}

	if ((option & OPT_OUT) != 0) {
		assert (nOutputs() == cir.nOutputs());
		auto it = cir.outputs().begin();
		for (auto x : outputs())
			added2new[x] = *it++;
	}

	add_circuit(cir, added2new, postfix);

}

void circuit::add_circuit_byname(const circuit& cir2, const string& namePostfix)  {
	id2idmap cir2toNew_wmap;
	add_circuit_byname(cir2, namePostfix, cir2toNew_wmap);
}

void circuit::add_circuit_byname(const circuit& cir2, const string& namePostfix, id2idmap& cir2toNew_wmap) {

	// take care of constant nodes
	if (cir2.has_const0())
		cir2toNew_wmap[cir2.get_cconst0()] = get_const0();
	if (cir2.has_const1())
		cir2toNew_wmap[cir2.get_cconst1()] = get_const1();

	// add wires and store them in map
	foreach_wire(cir2, w, wid) {
		if (!cir2.isConst(wid)) {
			// for ports
			if (w.isInput() || w.isKey()) {
				id wfound = find_wire(cir2.wname(wid));
				if (  wfound != -1 ) {
					cir2toNew_wmap[wid] = wfound;
				}
				else {
					cir2toNew_wmap[wid] = add_wire(cir2.wname(wid), w.wrtype());
					// std::cout << "adding inkey" << cir2.wname(wid) << "\n";
				}
			}
			else {

				// for internals and outputs
				string newname = cir2.wname(wid) + namePostfix;
				if (find_wire(newname) != -1)
					newname = "";

				cir2toNew_wmap[wid] = add_wire(newname, cir2.wrtype(wid));
				// std::cout << "adding " << newname << " for " << cir2.wname(wid) << " " << (int)(cir2.wrtype(wid)) << "\n";
			}
		}
	}

	// add gates while mapping to the new wires
	foreach_gate(cir2, g, gid) {
		assert(g.isGood());
		id newFanout = -1;
		try {
		newFanout = cir2toNew_wmap.at(cir2.gfanout(gid));
		}
		catch(std::out_of_range&) {neos_error("cant find " << cir2.gfanout(gid)); }

		idvec newFanins;
		for (auto fanin : g.fanins) {
			try {
			newFanins.push_back(cir2toNew_wmap.at(fanin));
			}
			catch(std::out_of_range&) { neos_error("cant find " << cir2.wname(fanin)); }
		}

		add_gate(newFanins, newFanout, cir2.gfunction(gid));
	}

}


void circuit::remove_circuit(idvec rem_gates){
	bool wire_live = false;
	idset cut_wires;
	for ( auto&& x : rem_gates ){
		getgate(x).setgfun(fnct::UNDEF);
		for ( auto& y : getgate(x).fanins ){
			cut_wires.insert(y);
		}
		getgate(x).fanins.clear();
		cut_wires.insert(getgate(x).fo0());
		getgate(x).fo0() = -1;
	}

	for ( auto& x : cut_wires ){
		wire_live = false;
		if ( getwire(x).isOutput() || getwire(x).isInput() ){
			wire_live = true;
		}
		if ( getwire(x).isInter() ){
			for (auto& gid : getwire(x).fanins)
				if (_is_not_in(gid, rem_gates) ) {
					wire_live = true;
					break;
				}

			for (  auto& gid : getwire(x).fanouts )
				if ( _is_not_in(gid, rem_gates) ) {
					wire_live = true;
					break;
				}
		}
		if ( !wire_live )
			getwire(x).setwtype(wtype::REMOVED);
	}
}


void circuit::remove_kcut(const idset& kcut, const idset& fanins) {
	for (auto gid : kcut) {
		if (gate_exists(gid)) {
			for (auto wid : getgate(gid).fanins){
				if (_is_not_in(wid, fanins) && wire_exists(wid)){
					remove_wire(wid);
				}
			}
			remove_gate(gid);
		}
	}
}

void circuit::remove_gate(id gid, bool update_wires) {

	assert( gate_exists(gid) );

	const auto& g = getcgate(gid);

	if (update_wires) {
		for (auto fo : g.fo()) {
			if ( wire_exists(fo) ) {
				getwire(fo).removefanin(gid);
			}
		}
		for (auto fi : g.fi()) {
			if ( wire_exists(fi) )
				getwire(fi).removefanout(gid);
		}
	}

	if (g.gfun() == fnct::DFF)
		dffs.erase(gid);

	nodes.erase(gid, (int8_t)nltype::GATE);
	node_names.remove_byfirst(gid);

	clear_topsort();
}


void circuit::remove_wire(id wid, bool update_gates) {

	if (update_gates) {
		const auto& w = getcwire(wid);
		for (auto gid : w.fo()) {
			getgate(gid).removefanin(wid);
		}
		for (auto wfanin : w.fi()) {
			getgate(wfanin).removefanout(wid);
		}
	}

	setwiretype(wid, wtype::REMOVED);

	nodes.erase(wid, (int8_t)nltype::WIRE);
	node_names.remove_byfirst(wid);

	if (wid == vddid) vddid = -1;
	if (wid == gndid) gndid = -1;

	clear_topsort();
}

void circuit::swap_wirenames(id w1, id w2) {
	string temp1 = node_names.at(w1);
	string temp2 = node_names.at(w2);
	node_names.remove_byfirst(w1);
	node_names.remove_byfirst(w2);

	node_names.add_pair(w1, temp2);
	node_names.add_pair(w2, temp1);
}

/*
 * pramouds code emits $enc that I fix here
 */
void circuit::fix_output(void) {
	for (auto& x : prim_output_set) {
		if (node_names.at(x) == "out$enc") {
			swap_wirenames(x, find_wire("out"));
		}
	}
	reparse();
}

void circuit::setwirename(id wid, const string& newName, bool force) {
	id widdup = find_wire(newName);
	if (widdup != -1) {
		if (force) {
			setwirename(widdup);
		}
		else {
			neos_abort("wire name " << newName << " already exists");
		}
	}
	node_names.remove_byfirst(wid);
	node_names.add_pair(wid, newName);
}

void circuit::setgatefun(id gid, fnct fun) {

	const auto& g = getgate(gid);

	// some checks
	if ( g.gfun() == fnct::BUF || g.gfun() == fnct::NOT) {
		assert(fun == fnct::BUF || fun == fnct::NOT);
	}

	if (fun == fnct::DFF)
		dffs.insert(gid);
	else if (g.gfun() == fnct::DFF)
		dffs.erase(gid);

	getgate(gid).setgfun(fun);

}

string circuit::get_new_wname(id hint) const {
	id post = (hint == -1) ? nWires():hint;
	string retstr;
	do {
		retstr = "n" + std::to_string(post++);
	}
	while (find_wire(retstr) != -1);

	return retstr;
}

string circuit::get_new_gname(id hint) const {
	id post = (hint == -1) ? nGates():hint;
	string retstr;
	do {
		retstr = "g" + std::to_string(post++);
	}
	while (find_gate(retstr) != -1);

	return retstr;
}

string circuit::get_new_iname(id hint) const {
	id post = (hint == -1) ? nGates():hint;
	string retstr;
	do {
		retstr = "u" + std::to_string(post++);
	}
	while (find_gate(retstr) != -1);

	return retstr;
}

id circuit::get_random_wireid(wtype wt) const {
	id rgid, lgid;
	rgid = lgid = rand() % get_max_wid();
	if (wire_exists(rgid) && (wt == wtype::REMOVED || getwtype(rgid) == wt))
		return rgid;
	while(true) {
		rgid++; lgid--;
		if (wire_exists(rgid) && (wt == wtype::REMOVED || getwtype(rgid) == wt)) {
			return rgid;
		}
		if (wire_exists(lgid) && (wt == wtype::REMOVED || getwtype(lgid) == wt)) {
			return lgid;
		}
	}
}

id circuit::get_random_gateid() const {
	id rgid, lgid;
	rgid = lgid = rand() % get_max_gid();
	if (gate_exists(rgid))
		return rgid;
	while(true) {
		if (gate_exists(++rgid)) {
			return rgid;
		}
		if (gate_exists(--lgid)) {
			return lgid;
		}
	}
}

void circuit::setwiretype(id x, wtype wt) {

	assert(wire_exists(x));

	wtype prev_type = wrtype(x);

	if (wt == wtype::IN && prev_type == wtype::OUT) {
		neos_abort("trying to change output to input");
	}
	if (wt == wtype::OUT && prev_type == wtype::IN) {
		neos_abort("trying to change input to output");
	}

	if ( wt != prev_type ) {
		getwire(x).ncat1 = (int8_t)wt;
		unregister_wiretype(x, prev_type);
		register_wiretype(x, wt);
	}
}

void circuit::setwirename(id wid) {
	setwirename(wid, get_new_wname(wid));
}

void circuit::remove_node(id nid, bool update_surroundings) {
	switch (getnode(nid).ndtype()) {
	case nltype::GATE:
		remove_gate(nid, update_surroundings);
		break;
	case nltype::WIRE:
		remove_wire(nid, update_surroundings);
		break;
	case nltype::INST:
		remove_instance(nid, update_surroundings);
		break;
	default:
		neos_abort("bad ncat0");
	}
}

void circuit::setnodeid_toabsentid(id oldid, id newid) {

	assert(!node_exists(newid));
	assert(node_exists(oldid));
	nlnode ond = getnode(oldid);
	string oname = ndname(oldid);
	ond.nid = newid;
	node_names.remove_byfirst(oldid);
	node_names.add_pair(newid, oname);
	nodes.insert_entry(newid, ond, (int8_t)ond.ncat0);

	//std::cout << oldid << " setting wire type " << newid << " -> " << (int)ond.wrtype() << "\n";
	if (ond.isWire()) {
		if (oldid == vddid) vddid = newid;
		if (oldid == gndid) gndid = newid;
		register_wiretype(newid, ond.wrtype());
	}
	else
		setgatefun(newid, ond.gfun());

	for (auto fanin : ond.fanins) {
		for (auto& fanout : getwire(fanin).fanouts) {
			if (fanout == oldid) {
				fanout = newid;
			}
		}
	}
	for (auto fanout : ond.fanouts) {
		for (auto& fanin : getwire(fanout).fanins) {
			if (fanin == oldid) {
				fanin = newid;
			}
		}
	}

	remove_node(oldid, false);

	assert(!node_exists(oldid));
}

void circuit::setnodeid(id oldid, id newid) {

	if (oldid == newid)
		return;

	assert(node_exists(oldid));
	// std::cout << "moving " << wname(oldid) << " " << oldid << " to " << newid << "\n";
	if (!node_exists(newid)) {
		setnodeid_toabsentid(oldid, newid);
	}
	else {
		// std::cout << "types: " << (int)wrtype(oldid) << " " << (int)wrtype(newid) << "\n";
		id dd1 = get_max_wid() + 1;
		id dd2 = get_max_wid() + 2;
		assert(!node_exists(dd1) && !node_exists(dd2));
		assert(dd1 != newid && dd2 != newid);
		assert(dd1 != oldid && dd2 != oldid);
		setnodeid_toabsentid(newid, dd1);
		setnodeid_toabsentid(oldid, dd2);
		setnodeid_toabsentid(dd2, newid);
		setnodeid_toabsentid(dd1, oldid);
	}

}

void circuit::simplify_wkey(const boolvec& key) {
	id2boolHmap valmap;
	assert (key.size() == key_input_set.size());
	auto it = key.begin();
	for (auto& x : key_input_set) {
		valmap[x] = *it++;
	}
	propagate_constants(valmap);
}

void circuit::remove_bufs() {
	idset to_remove;
	foreach_gate((*this), g, gid) {
		fnct gfun = g.gfun();
		if ( gfun == fnct::BUF && ( isInter(gfanin0(gid)) || isInter(gfanout(gid)) ) ) {
			to_remove.insert(gid);
			internal_merge_wires(gfanin0(gid), gfanout(gid));
		}
	}

	for (auto gid : to_remove)
		remove_gate(gid);
}

/*
 * remove not gates by absorbing them to their fanin
 */
void circuit::remove_nots() {
	idset to_remove;
	foreach_gate((*this), g, gid) {
		fnct gfun = g.gfun();
		if ( gfun == fnct::NOT ) {
			if (!isInput(gfanin0(gid)) && !isKey(gfanin0(gid))) {
				id faninGid = wfanin0(gfanin0(gid));
				switch (gfunction(faninGid)) {
				case fnct::AND:
					setgatefun(faninGid, fnct::NAND);
					break;
				case fnct::NAND:
					setgatefun(faninGid, fnct::AND);
					break;
				case fnct::OR:
					setgatefun(faninGid, fnct::NOR);
					break;
				case fnct::NOR:
					setgatefun(faninGid, fnct::OR);
					break;
				case fnct::XOR:
					setgatefun(faninGid, fnct::XNOR);
					break;
				case fnct::XNOR:
					setgatefun(faninGid, fnct::XOR);
					break;
				default:
					assert(0);
					break;
				}
				to_remove.insert(gid);
				internal_merge_wires(gfanin0(gid), gfanout(gid));
			}
		}
	}

	for (auto gid : to_remove)
		remove_gate(gid);
}

id circuit::join_outputs(fnct fun, const string& owname) {
	idvec outputs = utl::to_vec(prim_output_set);
	return join_outputs(outputs, fun, owname);
}

id circuit::join_outputs(const idvec& outids, fnct fun, const string& owname) {

	string outname;
	if (owname != "") {
		id found = find_wire(owname);
		if (found != -1) {
			setwirename(found, get_new_wname(found));
		}
		outname = owname;
	}
	else {
		outname = "";
	}

	clear_topsort();

	// take care of single output first
	id noid;
	if (outids.size() == 1) {
		switch (fun) {
		case fnct::NAND:
		case fnct::NOT:
		case fnct::XNOR:
		case fnct::NOR: {
			noid = add_wire(outname, wtype::OUT);
			setwiretype(outids[0], wtype::INTER);
			add_gate(outids, noid, fnct::NOT);
			break;
		}
		case fnct::AND:
		case fnct::OR:
		case fnct::XOR: {
			noid = outids[0];
			break;
		}
		default:
			noid = -1;
			assert(0);
		}

		setwirename(noid, outname);
		return noid;
	}

	noid = add_wire(outname, wtype::OUT);

	for (auto oid : outids) {
		setwiretype(oid, wtype::INTER);
	}

	switch (fun) {
	case fnct::AND :
	case fnct::NAND :
	case fnct::OR :
	case fnct::NOR : {
		add_gate(outids, noid, fun);
		break;
	}
	case fnct::XOR:
	case fnct::XNOR: {
		idvec stack = outids;
		while (!stack.empty()) {
			id oid1 = stack.back();
			stack.pop_back();
			id oid2 = stack.back();
			stack.pop_back();
			id nwid = stack.empty() ? noid : add_wire(wtype::INTER);
			if (!stack.empty())
				stack.push_back(nwid);
			add_gate({oid1, oid2}, nwid, fun);
		}
		break;
	}
	default:
		assert(0);
	}

	return noid;

}

void circuit::convert2generic() {
}

} // namespace ckt
