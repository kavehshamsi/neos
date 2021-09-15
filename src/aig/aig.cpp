/*
 * aig.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: kaveh
 */

#include "aig/aig.h"

namespace aig_ns {

aig_t::aig_t(const circuit& ckt) {
	id2alitmap wid2almap;
	init_from_ckt(ckt, wid2almap);
}

aig_t::aig_t(const circuit& ckt, id2alitmap& wid2almap) {
	init_from_ckt(ckt, wid2almap);
}

void aig_t::init_from_ckt(const circuit& cir, id2alitmap& wid2almap) {

	//ckt.remove_bufs();

	for (auto xid : cir.inputs()) {
		wid2almap[xid] = add_input(xid, cir.wname(xid));
		// std::cout << cir.wname(xid) << " : " << wid2almap.at(xid) << "\n";
	}

	for (auto kid : cir.keys()) {
		id kaid = add_key(kid, cir.wname(kid));
		wid2almap[kid] = kaid;
		//std::cout << cir.wname(kid) << " : " << wid2almap.at(kid) << "\n";
	}

	for (auto oid : cir.outputs()) {
		wid2almap[oid] = add_output(oid, cir.wname(oid), -1, 0);
	}

	if (cir.has_const0())
		wid2almap[cir.get_cconst0()] = get_const0();
	if (cir.has_const1())
		wid2almap[cir.get_cconst1()] = ~get_const0();

	const idvec& gate_ordering = cir.get_topsort_gates();

	foreach_latch(cir, dffid, dffout, dffin) {
		//std::cout << "adding latch " << cir.wname(dffout) << "\n";
		id laid = add_latch(cir.wname(dffout));
		wid2almap[dffout] = laid;
	}

	for (auto gid : gate_ordering) {
		if (!cir.isLatch(gid)) {
			id gout = cir.gfanout(gid);
			if (cir.isOutput(gout)) {
				wid2almap[gout] = add_complex_gate(cir, gid, wid2almap);
				//std::cout << cir.wname(gout) << " with mask " << wid2almap.at(gout) << "\n";
			}
			else { // this named-internal-node will slow down conversion
				wid2almap[gout] = add_complex_gate(cir, gid, wid2almap, cir.wname(gout));
			}
		}
	}

	foreach_latch(cir, dffid, dffout, dffin) {
		id laid = wid2almap.at(dffout).lid();
		nodeMap.at(laid).fanin0 = nodeMap.at(laid).fanin1 = wid2almap.at(dffin);
	}

	for (auto oid : cir.outputs()) {
		//std::cout << "adding output\n";
		// freeup_node_name(cir.wname(oid));
		// std::cout << "connecting " << wid2almap.at(oid) << " " << oid << " " << to_str(wid2almap.at(oid).lid()) << " " << to_str(oid) << "\n";
		add_edge(wid2almap.at(oid), oid);
		wid2almap[oid] = oid;
		// wid2almap[oid] = add_output(cir.wname(oid), wid2almap.at(oid));
	}

	/*for (auto nid : cir.allports()) {
		std::cout << nid << " " << cir.wname(nid) << " " << wid2almap.at(nid) << " " << find_node(cir.wname(nid)) << "\n";
		assert(nid == wid2almap.at(nid).lid());
	}*/

	top_name = cir.top_name;

}

aig_t::aig_t(const string& filename, char format) {
	circuit cir(filename, format);
	id2alitmap dummy;
	init_from_ckt(cir, dummy);
}

circuit aig_t::to_circuit() const {
	circuit tmp_ckt;
	to_circuit(tmp_ckt);
	return tmp_ckt;
}

// will keep the input-output-key ids the same
void aig_t::to_circuit(circuit& cir) const {

	assert(cir.nWires() == 0);

	//std::cout << "converting to circuit\n";
	id2idmap aid2widmap;
	using std::to_string;
	// add internal nodes
	for (auto aid : inputs()) {
		//std::cout << "adding input " << ndname(aid) <<"\n";
		aid2widmap[aid] = cir.add_wire(ndname(aid), wtype::IN, aid);
	}

	for (auto kid : keys()) {
		//std::cout << "setting type to key " << ndname(kid) << "\n";
		cir.setwiretype(aid2widmap.at(kid), wtype::KEY);
	}

	for (auto oid : outputs()) {
		aid2widmap[oid] = cir.add_wire(ndname(oid), wtype::OUT, oid);
	}

	if (has_const0()) {
		aid2widmap[get_cconst0id()] = cir.get_const0();
	}

	for (auto nid : latches()) {
		aid2widmap[nid] = cir.add_wire(wtype::INTER);
	}

	foreach_and((*this), nd, nid) {
		aid2widmap[nid] = cir.add_wire(wtype::INTER);
	}

	foreach_node((*this), nd, nid) {

		if (nd.isLatch()) {
			id wfanin = aid2widmap.at(nd.fi0());
			if (nd.cm0()) {
				string nn = cir.wname(wfanin) + "b";
				id win_b = cir.find_wire(nn);
				if (win_b == -1) {
					win_b = cir.add_wire(nn, wtype::INTER);
					cir.add_gate({wfanin}, win_b, fnct::NOT);
				}
				wfanin = win_b;
			}
			cir.add_gate("g" + to_string(cir.nGates()), {wfanin}, aid2widmap.at(nd.nid), fnct::DFF);
		}
		if (nd.isAnd()) {
			idvec fanins = {nd.fi0(), nd.fi1()};
			idvec wfanins = {aid2widmap.at(fanins[0]), aid2widmap.at(fanins[1])};
			boolvec cmasks = {nd.cm0(), nd.cm1()};
			id out = aid2widmap.at(nd.nid);
			// std::cout << "wname win0 " << ckt.wname(win0) << " wname win1 " << ckt.wname(win1) << "\n";
			for (int i : {0, 1}) {
				if (cmasks[i]) {
					string nn = cir.wname(wfanins[i]) + "b";
					id win_b = cir.find_wire(nn);
					if (win_b == -1) {
						win_b = cir.add_wire(nn, wtype::INTER);
						cir.add_gate({wfanins[i]}, win_b, fnct::NOT);
					}
					wfanins[i] = win_b;
				}
			}
			cir.add_gate({wfanins[0], wfanins[1]}, out, fnct::AND);
		}
	}

	// connect outputs with buffers
	for (auto oid : outputs()) {
		auto& nd = getcNode(oid);
		id owid = aid2widmap.at(oid);
		assert(nd.fanin0 == nd.fanin1);
		id outInwid = aid2widmap.at(nd.fi0());
		cir.add_gate({outInwid}, owid, nd.cm0() ? fnct::NOT:fnct::BUF);
	}

	cir.top_name = top_name;

}

// if circuit is OK returns true
bool aig_t::error_check() const {

#define _aig_error_block(str)  std::cout << str << "\n"; return false;
#define _aig_error_nonblock(str)  std::cout << str << "\n"; good = false;

	bool good = true;
	if (number_of_nodes != nodeMap.size()) {
		_aig_error_nonblock("node count mismatch");
	}

	for (auto aid : inputs()) {
		if ( _is_not_in(aid, nodeMap) ) {
			_aig_error_nonblock("input node-id " << aid << " not in nodemap");
		}
	}
	for (auto aid : outputs()) {
		if ( _is_not_in(aid, nodeMap) ) {
			_aig_error_nonblock("output node-id " << aid << " not in nodemap");
		}

		for (id fanin : {nfanin0(aid), nfanin1(aid)}) {
			if (!node_exists(fanin)) {
				_aig_error_nonblock("output " << aid << " fanin " << fanin << " not in nodemap\n");
			}
		}

		if (!getcNode(aid).fanouts.empty()) {
			_aig_error_nonblock("output has non-empty fanout\n");
		}

	}

	for (auto kid : keys()) {
		if ( _is_not_in(kid, input_set)) {
			_aig_error_nonblock("key input with id:" << kid << " not in inputs");
		}
	}

	foreach_node((*this), nd, nid) {

		if (nd.nid == -1) {
			_aig_error_block("node has an id of -1");
		}

		if (nd.nid != nid) {
			_aig_error_block("node id mismatch");
		}

		if (!nd.isOutput() && nfanouts(nd.nid).empty()) {
			//std::cout << "non output node " << ndname(nid) << " has empty fanoutset\n";
			// error = false; // allow dead nodes
		}

		if (nd.isOutput()) {
			if (nd.fanin0 != nd.fanin1) {
				_aig_error_block("output node " << nid << " fanins not equal");
			}
		}

		if (nd.isAnd()) {
			if (!_is_in(nd.nid, nfanouts(nd.fi0())) || !_is_in(nd.nid, nfanouts(nd.fi1()))) {
				_aig_error_block("incomplete fanoutset ");
			}

			if (nd.fi0() == nid || nd.fi1() == nid) {
				_aig_error_block("self looping AND node " << nid);
			}

			for (auto fanout : nd.fanouts) {
				if (node_exists(fanout)) {
					auto& fanout_nd = getcNode(fanout);
					if (fanout_nd.fanin0 != nid && fanout_nd.fanin1 != nid) {
						_aig_error_block("fanout mismatch for node " << nid);
					}
				}
				else {
					_aig_error_block("node " << ndname(nid) << " has invalid fanout " << fanout);
				}
			}

			if (nd.fanin0 == nd.fanin1) {
				_aig_error_nonblock("AND node " << nd.nid << " has same inputs");
			}
		}
	}

	for (auto& p : faninTable) {
		if (!node_exists(p.second)) {
			_aig_error_nonblock(p.first << " strashed to non-existent " << p.second);
		}
	}

	if constexpr (_two_level_hashing) {
		for (auto& p : fanin3Table) {
			if (!node_exists(p.second)) {
				_aig_error_nonblock(p.first << " strashed to non-existent " << p.second);
			}
		}
		for (auto& p : fanin4Table) {
			if (!node_exists(p.second)) {
				_aig_error_nonblock(p.first << " strashed to non-existent " << p.second);
			}
		}
	}

	return good;
}


std::string aig_t::get_new_name(id hint, ndtype_t nt) const {
	std::string retname;
	int i = (hint == -1) ? nNodes():hint;

	std::string bnm;
	switch(nt) {
	case ndtype_t::NTYPE_AND:
		bnm = "a";
		break;
	case ndtype_t::NTYPE_IN:
		bnm = "i";
		break;
	case ndtype_t::NTYPE_LATCH:
		bnm = "s";
		break;
	case ndtype_t::NTYPE_OUT:
		bnm = "o";
		break;
	default:
		assert(false);
	}

	do {
		retname = bnm + std::to_string(i++);
	} while (nodeNames.is_in_seconds(retname));
	return retname;
}

void aig_t::freeup_node_name(const std::string& nm) {
	id x = find_node(nm);
	if (x != -1)
		set_node_name(x, get_new_name());
}

aigFanin aig_t::getFaninObj(id aid) {
	auto& nd = getNode(aid);
	return aigFanin(nd.fanin0, nd.fanin1);
}

id aig_t::add_new_key() {
	id kid = add_input("keyinput" + std::to_string(key_set.size() + 1));
	key_set.insert(kid);
	return kid;
}

void aig_t::add_to_keys(id aid) {
	assert(_is_in(aid, nodeMap));
	key_set.insert(aid);
}

id aig_t::add_key(const string& inName) {
	id newin = add_input(inName);
	key_set.insert(newin);
	return newin;
}

id aig_t::add_key(id kid, const string& inName) {
	id newin = add_input(kid, inName);
	key_set.insert(newin);
	return newin;
}

id aig_t::add_input(const string& inName) {
	id nid = nodeMap.create_new_entry();
	return add_input(nid, inName);
}

id aig_t::add_input(id nid, const string& inName) {
	assert(!nodeNames.is_in_seconds(inName)); // ensure name does not exist
	nodeMap[nid] = aigNode(nid, -1, 0, -1, 0, NTYPE_IN);
	nodeNames.add_pair(nid, inName);
	input_set.insert(nid);
	number_of_nodes++;
	return nid;
}

id aig_t::add_output(const string& outname, alit faninlit) {
	return add_output(outname, faninlit.lid(), faninlit.sgn());
}

id aig_t::add_output(const string& outname, id fanin, bool mask) {
	id nid = nodeMap.create_new_entry();
	return add_output(nid, outname, fanin, mask);
}

id aig_t::add_output(id nid, const string& outname, id fanin, bool mask) {
	assert(!nodeNames.is_in_seconds(outname)); // ensure name does not exist
	nodeMap[nid] = aigNode(nid, fanin, mask, fanin, mask, NTYPE_OUT);
	nodeNames.add_pair(nid, outname);
	output_set.insert(nid);
	try {
		nodeMap.at(fanin).fanouts.insert(nid);
	}
	catch (std::out_of_range&) {}
	number_of_nodes++;
	return nid;
}

void aig_t::add_edge(alit ul, id vid, int fanin_index) {
	id uid = ul.lid();
	auto& und = getNode(uid);
	auto& vnd = getNode(vid);

	und.addfanout(vid);

	if (vnd.isOutput()) {
		vnd.fanin0 = ul;
		vnd.fanin1 = ul;
	}
	else {
		if (fanin_index == 0)
			vnd.fanin0 = ul;
		else if (fanin_index == 1)
			vnd.fanin1 = ul;
		else
			assert(false);
	}
}

id aig_t::add_output(const string& outName) {
	assert(!nodeNames.is_in_seconds(outName)); // ensure name does not exist
	id nid = nodeMap.create_new_entry();
	return add_output(nid, outName, -1, 0);
}

aigNode::aigNode() :
		nid(-1), fanin0(-1), fanin1(-1),
		markA(false), markB(false), markC(false),
		ndtype(NTYPE_EMPTY),
		depth(-1) {}

// real constructor
aigNode::aigNode(id nid, alit fanin0, alit fanin1, ndtype_t ntype) :
		nid(nid), fanin0(fanin0), fanin1(fanin1),
		markA(false), markB(false), markC(false), ndtype(ntype), depth(-1) {}

aigNode::aigNode(id nid, id fanin0id, bool cmask0, id fanin1id, bool cmask1, ndtype_t ntype) : // true node with nid
	nid(nid),  fanin0(fanin0id, cmask0), fanin1(fanin1id, cmask1),
	markA(false), markB(false), markC(false), ndtype(ntype), depth(-1) {}

alit aig_t::get_const0() {
	return alit(get_const0id(), 0);
}

alit aig_t::get_cconst0() const {
	return alit(get_cconst0id(), 0);
}

id aig_t::get_cconst0id() const {
	assert(aConst0 != -1);
	return aConst0;
}

id aig_t::get_const0id() {
	if (aConst0 == -1) {
		aConst0 = nodeMap.create_new_entry();
		nodeMap[aConst0] = aigNode(aConst0, -1, 0, -1, 0, NTYPE_CONST0);
		number_of_nodes++;
		nodeNames.add_pair(aConst0, "gnd");
		clear_orders();
	}
	return aConst0;
}

oidset aig_t::nonkey_inputs() const {
	oidset retset;
	for (auto nid : inputs())
		if (!isKey(nid))
			retset.insert(nid);
	return retset;
}

oidset aig_t::combins() const {
	oidset retset = inputs();
	utl::add_all(retset, latches());
	return retset;
}



bool aig_t::node_exists(alit al) const {
	return node_exists(al.lid());
}

bool aig_t::node_exists(id aid) const {
	if (_is_in(aid, nodeMap))
		return true;
	//std::cout << "node " << aid << " does not exist\n";
	return false;
}



bool aig_t::isLatch_input(id aid, id* latch) const {
	for (auto fanout : nfanouts(aid))
		if (_is_in(fanout, latch_set)) {
			if (latch)
				*latch = fanout;
			return true;
		}
	return false;
}

bool aig_t::isLatch_output(id aid, id* latch) const {

	const auto& nd = getcNode(aid);
	if ( _is_in(nd.fi0(), latch_set)) {
		if (latch)
			*latch = nd.fi0();
		return true;
	}

	if (_is_in(nd.fi1(), latch_set) ) {
		if (latch)
			*latch = nd.fi1();
		return true;
	}

	return false;
}


// the nName method will search for a name. If it does not exist it will
// add a new name that is "n + nodeId".
const std::string& aig_t::ndname(id aid) const {
	assert(node_exists(aid));
	try {
		return nodeNames.at(aid);
	}
	catch (std::out_of_range&) {
		const_cast<aig_t*>(this)->nodeNames.add_pair(aid, get_new_name(aid, getcNode(aid).ntype()));
	}
	return nodeNames.at(aid);
}

std::string aig_t::ndname(alit al) const {
	return (al.sgn() ? "~":"") + ndname(al.lid());
}

// change the name of a node
void aig_t::set_node_name(id aid, const std::string& nm) {
	assert(aid != aConst0);
	nodeNames.remove_byfirst(aid);
	nodeNames.add_pair(aid, nm);
}

// search for node through name lookup
id aig_t::find_node(const std::string& nm) const {
	auto it = nodeNames.getImap().find(nm);
	if (it != nodeNames.getImap().end())
		return (*it).second;
	else
		return -1;
}

// aborts if node not found
id aig_t::find_wcheck(const std::string& nm) const {
	auto it = nodeNames.getImap().find(nm);
	if (it != nodeNames.getImap().end())
		return (*it).second;
	else {
		neos_abort("node " << nm << " not found in aig");
		return -1;
	}
}

id aigNode::nfanout0() const {
	if (fanouts.empty())
		return -1;
	else
		return *(fanouts.begin());
}

const idset& aigNode::nfanouts() const {
	return fanouts;
}

id aig_t::nfanout0(id aid) const {
	auto& nd = getcNode(aid);
	if (nd.fanouts.empty())
		return -1;
	else
		return *(nd.fanouts.begin());
}

const idset& aig_t::nfanouts(id aid) const {
	return getcNode(aid).fanouts;
}


id aig_t::lookup_fanintable(const aigNode& nd, const aigFaninTable& faninTable) {
	return lookup_fanintable(aigFanin(nd), faninTable);
}

id aig_t::lookup_fanintable(const aigFanin& afanin, const aigFaninTable& faninTable) {

	auto it = faninTable.find(afanin);
	if (it == faninTable.end())
		return -1;
	else
		return it->second;
}

void aig_t::set_fanins(aigNode& nd, alit fanin01) {
	nd.fanin0 = nd.fanin1 = fanin01;
}

alit aig_t::add_and(const aigNode& nd, std::string nm, bool& strashed) {
	return add_and(nd.fanin0, nd.fanin1, nm);
}

// add AND node through mapping
alit aig_t::add_and(const aigNode& nd, const id2alitmap& mapping,
		const std::string& nm, bool& strashed) {
	return add_and(mapping.at(nd.fi0()) ^ nd.cm0(), mapping.at(nd.fi1()) ^ nd.cm1(), nm, strashed);
}

alit aig_t::add_and(id fanin0, bool cmask0, id fanin1, bool cmask1,
		const std::string& nm, bool& strashed) {
	return add_and(alit(fanin0, cmask0), alit(fanin1, cmask1), nm, strashed);
}

alit aig_t::add_and(alit f0, alit f1, const std::string& nm, bool& strashed) {

	assert(node_exists(f0) && node_exists(f1));

	clear_orders();
	strashed = false;

	alit res;
	if (check_constness(f0, f1, res)) {
		strashed = true;
		assert(res != -1);
		return res;
	}

	// Strashing
	id nid = -1;

	// one level strashing
	aigFanin fanin2(f0, f1);
	auto it = faninTable.find(fanin2);
	if (it != faninTable.end()) {
		// std::cout << "2-hashed to " << it->second << "\n"
		strashed = true;
		nid = it->second;
		assert(node_exists(nid));
	}

	if (nid == -1) {

		// two level hashing
		if constexpr (_two_level_hashing) {
			nid = two_level_strash_and(f0, f1, nm, strashed);
		}
		else {
			nid = create_new_and(f0, f1, nm);
			faninTable[fanin2] = nid;
		}
	}

	assert(node_exists(nid));
	return alit(nid, 0);
}

alit aig_t::lookup_and(alit f0, alit f1, bool& strashed) const {

	strashed = false;
	alit res = -1;

	if (!node_exists(f0) || !node_exists(f1)) {
		return res;
	}

	// a little violation of const. lookup can't create a stray constant node that can be cleaned up later.
	if (const_cast<aig_t*>(this)->check_constness(f0, f1, res)) {
		strashed = true;
		assert(res != -1);
		return res;
	}

	// one level strashing
	aigFanin fanin2(f0, f1);
	auto it = faninTable.find(fanin2);
	if (it != faninTable.end()) {
		// std::cout << "2-hashed to " << it->second << "\n"
		strashed = true;
		res = it->second;
		assert(node_exists(res));
	}

	return res;
}

id aig_t::two_level_strash_and(alit f0, alit f1, const string& nm, bool& strashed) {

	id nid = -1;

	const auto& nd0 = getcNode(f0.lid());
	const auto& nd1 = getcNode(f1.lid());

	bool has_fanin3_0 = nd0.isAnd();
	bool has_fanin3_1 = nd1.isAnd();
	bool has_fanin4 = has_fanin3_0 && has_fanin3_1;
	aigFanin3 fanin3_0, fanin3_1;
	aigFanin4 fanin4;

	if (has_fanin3_0) {
		fanin3_0 = aigFanin3(f1, f0.sgn(), nd0.fanin0, nd0.fanin1);
		auto it = fanin3Table.find(fanin3_0);
		if (it != fanin3Table.end()) {
			nid =  it->second;
		}
		else {
			nid = -1;
		}


		/*std::cout << "3-fanin " << fanin3_1.fl << " " << fanin3_1.cmr
				<< " " << fanin3_1.frl << " " << fanin3_1.frr << " \n";
		if ((it != fanin3Table.end())) {
			std::cout << ndname(nid) << " 3-strashed\n";
		}*/
	}
	if (has_fanin3_1) {
		fanin3_1 = aigFanin3(f0, f1.sgn(), nd1.fanin0, nd1.fanin1);
		auto it = fanin3Table.find(fanin3_1);
		if (it != fanin3Table.end()) {
			nid = it->second;
		}
		else {
			nid = -1;
		}
		/*std::cout << "3-fanin " << fanin3_1.fl << " " << fanin3_1.cmr
				<< " " << fanin3_1.frl << " " << fanin3_1.frr << " \n";
		if ((it != fanin3Table.end())) {
			std::cout << ndname(nid) << " 3-strashed\n";
		}*/
	}
	if (has_fanin4) {
		fanin4 = aigFanin4(nd0.fanin0, nd0.fanin1, nd1.fanin0, nd1.fanin1, f0.sgn(), f1.sgn());
		auto it = fanin4Table.find(fanin4);
		if (it != fanin4Table.end()) {
			nid = it->second;
		}
		else {
			nid = -1;
		}
		/*std::cout << "4-fanin " << fanin4.fll << " " << fanin4.flr
				<< " " << fanin4.cml << " " << fanin4.frl
				<< " " <<  fanin4.frr << " " << fanin4.cmr << " ";
		if ((it != fanin4Table.end())) {
			std::cout << " -> 4-strashed to " << nid << "\n";
		}*/
	}

	if (nid == -1) {
		nid = create_new_and(f0, f1, nm);
		//std::cout << " -> " << nid << "\n";
		strashed = false;
	}
	else {
		//std::cout << "two-level hashing happened\n";
		std::cout << "two level hashed to " << nid << "\n";
		assert(node_exists(nid));
		strashed = true;
	}

	if (has_fanin3_0)  {
		fanin3Table[fanin3_0] = nid;
	}
	if (has_fanin3_1) {
		fanin3Table[fanin3_1] = nid;
	}
	if (has_fanin4) {
		fanin4Table[fanin4] = nid;
	}

	faninTable[aigFanin(f0, f1)] = nid;

	return nid;
}

id aig_t::create_new_and(alit f0, alit f1, const string& nm) {
	// existing structure not present. create new node
	id nid = nodeMap.create_new_entry();
	auto& nd = nodeMap[nid] = aigNode(nid, f0, f1, NTYPE_AND);
	auto& fnd0 = nodeMap.at(f0.lid());
	auto& fnd1 = nodeMap.at(f1.lid());
	fnd0.fanouts.insert(nid);
	fnd1.fanouts.insert(nid);
	number_of_nodes++;
	if (nm != "") {
		if (nodeNames.is_in_seconds(nm)) {
			neos_abort("node name " << nm << " already exists");
		}
		nodeNames.add_pair(nid, nm);
	}
	nd.depth = MAX(fnd0.depth, fnd1.depth) + 1;
	return nid;
}


// traverses NOT/BUF chains to resolve the mask of an edge
void aig_t::resolve_mask(const circuit& ckt, id rootwid, bool& mask, id& aigwid) {

	aigwid = rootwid;

	id gid = ckt.wfanin0(rootwid);
	if (gid == -1)
		return;

	fnct gfun = ckt.gfunction(gid);
	if (gfun == fnct::NOT) {
		//std::cout << "traversing NOT " << ckt.wname(aigwid) << "\n";
		mask = !mask;
		resolve_mask(ckt, ckt.gfanin0(gid), mask, aigwid);
	}
	else if (gfun == fnct::OR || gfun == fnct::XOR || gfun == fnct::NAND || gfun == fnct::MUX) {
		mask = !mask;
	}

	return;
}

id aig_t::add_latch(const std::string& latchname) {




	id nid = nodeMap.create_new_entry();
	nodeMap[nid] = aigNode(nid, -1, 0, -1, 0, NTYPE_LATCH);

	if (latchname != "") {
		if (nodeNames.is_in_seconds(latchname)) {
			neos_abort("latch name " << latchname << " already exists");
		}
		nodeNames.add_pair(nid, latchname);
	}

	latch_set.insert(nid);
	number_of_nodes++;
	return nid;
}

id aig_t::add_latch(id fanin, bool cmask, const idset& fanout, const std::string& latchname) {

	id laid = add_latch(latchname);
	auto& nd = nodeMap.at(laid);
	nd.fanin0 = nd.fanin1 = alit(fanin, cmask);
	utl::add_all(nd.fanouts, fanout);

	return laid;
}

id aig_t::join_outputs(fnct gfun, const std::string& outname) {
	return join_outputs(utl::to_vec(outputs()), gfun, outname);
}

id aig_t::join_outputs(const idvec& outputs, fnct gfun,
		const std::string& outname) {

	// will overwrite existing node name to respect new outname
	if (outname != "")
		freeup_node_name(outname);

	idvec fanins;
	boolvec cmasks;
	for (auto outid : outputs) {
		auto& nd = getNode(outid);
		assert(nd.fanin0 == nd.fanin1);
		assert(nd.fanin0 != outid);
		fanins.push_back(nd.fi0());
		cmasks.push_back(nd.cm0());
		//std::cout << "removing for join-output\n";
		remove_node(outid);
	}

	assert(!fanins.empty());

	id out;
	if (outputs.size() == 1)
		out = add_output(outname, fanins[0], cmasks[0]);
	else {
		alit outlit = add_complex_gate(gfun, fanins, cmasks, "");
		out = add_output(outname, outlit);
	}

	return out;
}

alit aig_t::add_complex_gate(fnct gfun, const alitvec& fanins, const std::string& nodename) {

	alit out(-1);

	switch (gfun) {
	case fnct::AND:
	case fnct::NAND:
	case fnct::OR:
	case fnct::NOR: {
		alitque Q = utl::to_que(fanins);

		while (Q.size() != 1) {
			alit in0 = Q.front();
			Q.pop();
			alit in1 = Q.front();
			Q.pop();

			if (gfun == fnct::NAND || gfun == fnct::AND) {
				out = add_and(in0, in1);
				Q.push(out);
			} else if (gfun == fnct::NOR || gfun == fnct::OR) {
				out = add_and(~in0, ~in1);
				Q.push(~out);
			}
		}

		out = Q.back();
		out = (gfun == fnct::NOR || gfun == fnct::NAND) ? ~out:out;

		break;
	}
	case fnct::XOR:
	case fnct::XNOR: {
		alit out0 = add_and(fanins[0], ~fanins[1]);
		alit out1 = add_and(~fanins[0], fanins[1]);
		out = add_and(~out0, ~out1);

		out = (gfun == fnct::XOR) ? ~out:out;

		break;
	}
	case fnct::MUX: {
		alit s = fanins[0], a = fanins[1], b = fanins[2];
		alit out0 = add_and(a, ~s);
		alit out1 = add_and(b, s);
		out = ~add_and(~out0, ~out1);
		break;
	}
	case fnct::NOT:
		// skip inverters and buffers
		out = ~fanins[0];
		break;
	case fnct::BUF:
		out = fanins[0];
		break;
	case fnct::DFF:
		neos_error("cannot add latches with this method");
		break;
	default:
		neos_error("unknown gate type");
		break;
	}

	if (nodename != "" && !isInput(out.lid()) && !isConst0(out.lid()) && !isLatch(out.lid()))
		set_node_name(out.lid(), nodename);

	return out;
}

alit aig_t::add_complex_gate(fnct gfun, const idvec& fanins, const boolvec& masks, const std::string& nodename) {
	alitvec alv;
	assert(fanins.size() == masks.size());
	for (uint i = 0; i < fanins.size(); i++) {
		alv.push_back(alit(fanins[i], masks[i]));
	}
	return add_complex_gate(gfun, alv, nodename);
}

alit aig_t::add_complex_gate(const circuit& cir, id gid, const id2alitmap& wid2almap,
		const std::string& nodename) {

	// resolve complemented edges here
	alitvec aigfanin;
	const gate& g = cir.getcgate(gid);

	for (auto fanin : g.fanins) {
		try {
			alit al = wid2almap.at(fanin);
			aigfanin.push_back(al);
		}
		catch(std::out_of_range& e) {
			neos_abort("missing in map is wire " << cir.wname(fanin));
		}
	}

	return add_complex_gate(g.gfun(), aigfanin, nodename);
}

void aig_t::merge_nodes(id removeid, id keepid, bool invert, bool check_cycle) {

	assert(isAnd(removeid));

	//std::cout << "Merging " << nName(removeid) << " and " << nName(keepid) << "\n";

	if (check_cycle) {
		assert( _is_not_in(keepid, trans_fanout(removeid)) );
	}

	move_all_fanouts(removeid, keepid, invert);

	remove_node_recursive(removeid);

	if (node_exists(keepid))
		strash_fanout(keepid);
}


// remove structural hashes associated with node from tables
void aig_t::remove_surrounding_strashes(id aid) {

	const auto& nd = getcNode(aid);

	idset allfouts = {aid};

	if constexpr (_two_level_hashing) {
		for (auto fout : nd.fanouts) {
			allfouts.insert(fout);
			for (auto fout2 : getcNode(fout).fanouts) {
				allfouts.insert(fout2);
			}
		}
	}
	else {
		utl::add_all(allfouts, nd.fanouts);
	}


	for (auto x : allfouts) {
		remove_strashes(x);
	}
}


void aig_t::remove_strashes(id aid) {

	if (!node_exists(aid))
		return;

	//std::cout << "removing strashes for " << aid;

	const auto& nd = getcNode(aid);

	faninTable.erase(nd);

	if constexpr (_two_level_hashing) {

		bool node_present0 = node_exists(nd.fi0());
		bool node_present1 = node_exists(nd.fi1());

		alit f0 = nd.fanin0, f1 = nd.fanin1;

		if (node_present0) {
			const auto& nd0 = getcNode(f0.lid());
			fanin3Table.erase(aigFanin3(f1, f0.sgn(), nd0.fanin0, nd0.fanin1));
		}

		if (node_present1) {
			const auto& nd1 = getcNode(f1.lid());
			fanin3Table.erase(aigFanin3(f0, f1.sgn(), nd1.fanin0, nd1.fanin1));
		}

		if (node_present0 && node_present1) {
			const auto& nd0 = getcNode(f1.lid());
			const auto& nd1 = getcNode(f1.lid());
			fanin4Table.erase(aigFanin4(nd0.fanin0, nd0.fanin1, nd1.fanin0, nd1.fanin1, f0.sgn(), f1.sgn()));
		}

	}

	//std::cout << "\n";


}

void aig_t::remove_node(id aid) {

	aigNode nd = getNode(aid);

	switch (nd.ndtype) {
	case NTYPE_CONST0:
		aConst0 = -1;
		break;
	case NTYPE_IN:
		input_set.erase(nd.nid);
		key_set.erase(nd.nid);
		break;
	case NTYPE_OUT:
		output_set.erase(nd.nid);
		break;
	case NTYPE_LATCH:
		latch_set.erase(nd.nid);
		break;
	default:
		break;
	}


	try { nodeMap.at(nd.fi0()).fanouts.erase(nd.nid); }
		catch (std::out_of_range&) {}
	try { nodeMap.at(nd.fi1()).fanouts.erase(nd.nid); }
		catch (std::out_of_range&) {}

	// remove from data structures
	//std::cout << "   erasing node " << ndname(nd.nid) << "\n";

	remove_surrounding_strashes(nd.nid);

	nodeMap.erase(nd.nid);
	nodeNames.remove_byfirst(nd.nid);

	// invalidate the top orders
	clear_orders();

	// update the number of nodes
	number_of_nodes--;

}

// recursively removes nodes with no fanout
void aig_t::remove_node_recursive(id aid) {
	auto& nd = getNode(aid);
	remove_node_recursive(nd);
}

void aig_t::remove_node_recursive(const aigNode& nd) {
	id fanin0 = nd.fi0();
	id fanin1 = nd.fi1();
	ndtype_t nt = nd.ndtype;

	remove_node(nd.nid);

	if (nt == ndtype_t::NTYPE_AND || nt == ndtype_t::NTYPE_OUT) {
		auto& fanin0nd = getNode(fanin0);
		if (fanin0nd.fanouts.empty())
			remove_node_recursive(fanin0nd);

		if (node_exists(fanin1)) {
			auto& fanin1nd = getNode(fanin1);
			if (fanin1nd.fanouts.empty())
				remove_node_recursive(fanin1nd);
		}
	}

}


// does BFS from the outputs and deletes unreachable nodes
int aig_t::trim_aig_to(const oidset& outs) {

	idque Q;
	idset visited;

	id num_orig_nodes = nNodes();

/*
	idset out_to_remove;
	for (auto oid : outputs()) {
		if (_is_not_in(oid, outs)) {
			out_to_remove.insert(oid);
		}
	}

	for (auto orm : out_to_remove) {
		remove_node_recursive(orm);
	}

*/



	for (auto outid : outs) {
		Q.push(outid);
		visited.insert(outid);
	}

	while (!Q.empty()) {
		id curid = Q.front();
		Q.pop();
		const auto& curNode = getcNode(curid);
		for (id fanin : {curNode.fi0(), curNode.fi1()}) {
			if (fanin != -1 && _is_not_in(fanin, visited)) {
				Q.push(fanin);
				visited.insert(fanin);
			}
		}
	}

	idset deadnodes;
	for (auto oid : outputs())
		if (_is_not_in(oid, outs))
			deadnodes.insert(oid);

	foreach_node(*this, nd, nid) {
		if ( nd.isAnd() || nd.isInput() ) {
			if (_is_not_in(nid, visited) ) {
				deadnodes.insert(nid);
			}
		}
	}

	for (auto nid : deadnodes) {
		remove_node(nid);
	}


	return nNodes() - num_orig_nodes;

}

int aig_t::remove_dead_logic() {
	return trim_aig_to(outputs());
}

void aig_t::add_aig(const aig_t& aig2, id2alitmap& conMap, const string& postfix) {

	// add inputs and merge by name
	foreach_inaid(aig2, inaid2) {
		if (_is_in(inaid2, conMap))
			continue;
		id inaid1 = find_node(aig2.ndname(inaid2));
		if (inaid1 != -1) {
			conMap[inaid2] = inaid1;
		}
		else {
			if (aig2.isKey(inaid2))
				conMap[inaid2] = add_key(aig2.ndname(inaid2));
			else
				conMap[inaid2] = add_input(aig2.ndname(inaid2));
		}
	}

	// add latch nodes
	for (auto laid : aig2.latches()) {
		if (_is_in(laid, conMap))
			continue;
		conMap[laid] = add_latch(get_new_name());
	}

	// add AND gates
	foreach_and_ordered(aig2, nid2) {
		const auto& nd = aig2.getcNode(nid2);
		alit nnid = conMap[nd.nid] = add_and(nd, conMap);
	}

	// connect latches
	for (auto laid : aig2.latches()) {
		const auto& nd2 = aig2.getcNode(laid);
		auto& nd1 = getNode(conMap.at(laid).lid());

		idset fanouts;
		for (auto fanout2 : aig2.nfanouts(laid))
			fanouts.insert(conMap.at(fanout2).lid());

		set_fanins(nd1, conMap.at(nd2.fi0()));
		utl::add_all(nd1.fanouts, fanouts);
	}

	// add and connect outputs
	foreach_outaid(aig2, oaid2) {
		const auto& ond2 = aig2.getcNode(oaid2);
		auto fanin = conMap.at(ond2.fi0());
		std::string outname = aig2.ndname(oaid2) + postfix;
		if (find_node(outname) != -1) outname = get_new_name();
		conMap[oaid2] = add_output(outname, alit(fanin.lid(), fanin.sgn() ^ ond2.cm0()));
	}

}


void aig_t::add_aig(const aig_t& aig2, const std::string& postfix) {

	id2alitmap conMap;

	// add inputs and merge by name
	foreach_inaid(aig2, inaid2) {
		id inaid1 = find_node(aig2.ndname(inaid2));
		if (inaid1 != -1) {
			conMap[inaid2] = inaid1;
		}
		else {
			if (aig2.isKey(inaid2))
				conMap[inaid2] = add_key(aig2.ndname(inaid2));
			else
				conMap[inaid2] = add_input(aig2.ndname(inaid2));
		}
	}

	// add latch nodes
	for (auto laid : aig2.latches()) {
		conMap[laid] = add_latch(get_new_name());
	}

	// add AND gates
	foreach_and_ordered(aig2, nid2) {
		const auto& nd = aig2.getcNode(nid2);
		alit nnid = conMap[nd.nid] = add_and(nd, conMap);
	}

	// connect latches
	for (auto laid : aig2.latches()) {
		const auto& nd2 = aig2.getcNode(laid);
		auto& nd1 = getNode(conMap.at(laid).lid());

		idset fanouts;
		for (auto fanout2 : aig2.nfanouts(laid))
			fanouts.insert(conMap.at(fanout2).lid());

		set_fanins(nd1, conMap.at(nd2.fi0()));
		utl::add_all(nd1.fanouts, fanouts);
	}

	// add and connect outputs
	foreach_outaid(aig2, oaid2) {
		const auto& ond2 = aig2.getcNode(oaid2);
		auto fanin = conMap.at(ond2.fi0());
		std::string outname = aig2.ndname(oaid2) + postfix;
		if (find_node(outname) != -1) outname = get_new_name();
		conMap[oaid2] = add_output(outname, alit(fanin.lid(), fanin.sgn() ^ ond2.cm0()));
	}
}


// checks if node is constant
bool aig_t::check_constness(const aigNode& nd, alit& retnd) {
	return check_constness(nd.fanin0, nd.fanin1, retnd);
}

bool aig_t::check_constness(alit f0, alit f1, alit& retnd) {

	/*
	 * alt_node : -1: node is live
	 * 				x : connect to x
	 */

	retnd = -1;

	// detect node_state
	if (f0 == f1) {
		retnd = f0; // a.a = a
	}
	else if (f0 == ~f1) {
		retnd = get_const0(); // a.abar = 0
	}

	else if (has_const0()) {
		if (f0 == get_cconst0())
			retnd = get_cconst0();
		else if (f0 == ~get_cconst0())
			retnd = f1;
		else if (f1 == get_cconst0())
			retnd = get_cconst0();
		else if (f1 == ~get_cconst0())
			retnd = f0;
	}

	if (retnd != -1)
		return true;

	return false;
}

void aig_t::move_all_fanouts(id nid, id newfanin, bool invert) {
	idset fanouts = nfanouts(nid);
	for (auto fanout : fanouts) {
		//std::cout << "	now updating for fanout: " << ndname(fanout) << "\n";
		update_node_fanin(fanout, nid, newfanin, invert);
	}
}

void aig_t::strash_fanout(id aid) {

	idset fanouts = nfanouts(aid);

	// std::cout << "strashing output of " << ndname(aid) << "\n";
	for (id fanout : fanouts) {
		if (node_exists(fanout)) {
			const aigNode& fanout_nd = getNode(fanout);
			if (fanout_nd.isAnd()) {

				//std::cout << "   looking at " << ndname(fanout) <<  " with fanins: { "
						//<< ndname(fanout_nd.fi0()) << ", " << ndname(fanout_nd.fi1()) << "}\n";

				alit retnd(-1);
				if (check_constness(fanout_nd, retnd)) {
					merge_nodes(fanout_nd.nid, retnd.lid(), retnd.sgn());
					continue;
				}

				id ret = lookup_fanintable(aigFanin(fanout_nd), faninTable);
				if (ret != -1) {
					if (fanout == ret)
						continue;
					merge_nodes(fanout, ret);
				}
				else {
					faninTable[aigFanin(fanout_nd)] = fanout;
				}
			}
		}
	}
}


void aig_t::propagate_constants(const id2boolHmap& cstMap) {

	D1(circuit pre_prog = this->to_circuit();
	circuit tie_cir = pre_prog;
	id2boolmap preprogmap;
	)

	//std::cout << "now propagating constants\n";
	for (auto& cp : cstMap) {
		D1(
		// connect pre prof DEBUG only
		id cirid = pre_prog.find_wire(ndname(cp.first));
		preprogmap[cirid] = cp.second;
		)

		assert(isInput(cp.first));
		//std::cout << "setting input " << ndname(cp.first) << "\n";
		move_all_fanouts(cp.first, get_const0().lid(), cp.second);
		remove_node(cp.first);
	}

	//std::cout << "done updating fanins\n";

	if (has_const0())
		strash_fanout(get_cconst0id());

	//std::cout << "done strashinig\n";


  	D1(
  	pre_prog.simplify(preprogmap);
  	assert(ext::check_equivalence_abc(pre_prog, this->to_circuit()));
	)
}


void aig_t::update_node_fanin(id nid, id oldfanin, id newfanin, bool invert) {
	aigNode& nd = getNode(nid);
	update_node_fanin(nd, oldfanin, newfanin, invert);
}

void aig_t::update_node_fanin(aigNode& nd, id oldfanin, id newfanin, bool invert) {

	bool b0 = (nd.fi0() == oldfanin);
	bool b1 = (nd.fi1() == oldfanin);

	remove_surrounding_strashes(nd.nid);

	if (b0) {
		nd.fanin0.setid(newfanin);
		if (invert) nd.fanin0 ^= 1;
	}

	if (b1) {
		nd.fanin1.setid(newfanin);
		if (invert) nd.fanin1 ^= 1;
	}

	if (!b0 && !b1) {
		neos_abort("fanin update failed because no fanin equal to old value.");
	}

	getNode(oldfanin).removefanout(nd.nid);
	getNode(newfanin).addfanout(nd.nid);

}

// byte-sequential simulation
void aig_t::simulate_comb(const id2boolmap& inputmap, id2boolmap& simMap) const {

	for (auto inid : inputs()) {
		simMap[inid] = inputmap.at(inid);
	}
	simulate_comb(simMap);
}

void aig_t::simulate_comb(id2boolmap& simMap) const {

	add_const_to_map(simMap);

	const idvec& node_order = top_order();

	for (auto nid : node_order) {
		const auto& nd = getcNode(nid);
		if (nd.isAnd() || nd.isOutput())
			simMap[nid] = simulate_node(nid, simMap);
	}

}

bool aig_t::simulate_node(id aid, const id2boolmap& simMap) const {
	const auto& nd = getcNode(aid);
	try {
		bool in0 = simMap.at(nd.fi0());
		bool in1 = simMap.at(nd.fi1());
		bool res = ( in0 ^ nd.cm0() ) & ( in1 ^ nd.cm1() );
		return res;
	}
	catch (std::out_of_range&) {
		neos_abort("input not found in simMap at node " << to_str(aid));
	}
}

// word-parallel simulation
// in limited testing this did not appear to be faster than single-bit simulation
void aig_t::simulate_comb_word(const id2boolmap& inputmap, id2boolmap& simMap) const {

	for (auto inid : input_set) {
		simMap[inid] = inputmap.at(inid);
	}
	if (aConst0 != -1) simMap[aConst0] = 0;

	const idvec& node_order = top_order();


	uint64_t word0 = 0, word1 = 0, mask0 = 0, mask1 = 0, wordout = 0;
	uint i = 0, b = 0, s = 0;

	while (true) {
		id aid1 = node_order[i];
		id aid2 = node_order[i+1];
		const auto& nd = getcNode(aid1);
		setbit( word0, b, simMap.at(nd.fi0()) );
		setbit( word1, b, simMap.at(nd.fi1()) );
		setbit( mask0, b, nd.cm0());
		setbit( mask1, b, nd.cm1());

		if ( (i == node_order.size() - 1) || (b++ == 63) ||
				(nlevel(aid1) != nlevel(aid2)) ) {

			wordout = (word0 ^ mask0) & (word1 ^ mask1);

			b = 0;
			for (uint j = s, k = 0; j < i+1; j++, k++) {
				simMap[node_order[j]] = getbit(wordout, k);
			}

			if (i == node_order.size() - 1)
				break;

			s = i+1;
		}

		i++;
	}

}


void aig_t::sig_prob(uint num_patterns) {

	id2idmap onesMap;

	id2boolmap inputMap;
	id2boolmap simMap;

	auto start = _start_wall_timer();

	for (uint n = 0; n < num_patterns; n++) {

		for (auto inid : input_set) {
			inputMap[inid] = (rand() % 2 == 0);
		}

		simulate_comb(inputMap, simMap);

		for (auto pr : simMap) {
			onesMap[pr.first] = onesMap[pr.first] + (pr.second == 1);
		}
	}

	auto t = _stop_wall_timer(start);
	std::cout << "simulation time: " << t << "\n";

	for (auto pr : nodeNames.getmap()) {
		if (isInput(pr.first) || isOutput(pr.first))
			std::cout << pr.second << " : " << ((double)onesMap.at(pr.first)) / ((double)num_patterns) << "\n";
	}

}

void aig_t::print_ports() const {
	for (auto nid : inputs()) {
		std::cout << "INPUT(" << ndname(nid) << ")\n";
	}
	for (auto nid : outputs()) {
		std::cout << "OUTPUT(" << ndname(nid) << ")\n";
	}
}

void aig_t::convert_to_and2(circuit& cir) {
	aig_t aig_cir(cir);
	cir = aig_cir.to_circuit();
}


void aig_test() {
	std::cout << "testing aig package\n";
}


} // namespace aig


