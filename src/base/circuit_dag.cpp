
#include "circuit.h"

#include "boost/functional/hash.hpp"
//////////////// The circuit class ////////////////////

namespace ckt {

using namespace utl;


// get wire fanout set
void circuit::get_trans_fanoutset (id source, idset& fanoutset) const {
	get_trans_fanoutset(idset({source}), fanoutset);
}

void circuit::get_trans_fanoutset (const idset& source_set, idset& fanoutset) const {

	idque Q;
	idset visited;

	fanoutset.clear();
	for (auto wid : source_set) {
		Q.push(wid);
		visited.insert(wid);
	}

	while (Q.size() != 0) {
		id cur_wid = Q.front(); Q.pop();
		const auto& w = getcwire(cur_wid);
		for (auto gid : w.fanouts) {
			const auto& g = getcgate(gid);
			if (!g.isLatch()) {
				if (_is_not_in(g.fo0(), visited)) {
					visited.insert(g.fo0());
					Q.push(g.fo0());
					fanoutset.insert(g.fo0());
				}
			}
		}
	}
}


void circuit::get_gate_mffc(id rootgid, idset& mffc_gates) const {

	// performs backward topsort
	Hmap<id, int> degmap;

	idque deg_zero;
	mffc_gates.clear();
	mffc_gates.insert(rootgid);
	deg_zero.push(rootgid);

	while(!deg_zero.empty()) {
		id curgid = deg_zero.front(); deg_zero.pop();

		// init degmap in vicinity
		for (auto ingid : gfaninGates(curgid)) {
			if (_is_not_in(ingid, degmap)) {
				degmap[ingid] = gfanoutGates(ingid).size();
				if ( isOutput(gfanout(ingid)) )
					degmap.at(ingid)++;
			}
			if (--degmap.at(ingid) == 0) {
				deg_zero.push(ingid);
				mffc_gates.insert(ingid);
			}
		}

	}

}


void circuit::enumerate_cuts(Hmap<id, vector<cir_cut_t> >& cuts, int k, int N, bool isolated) const {

	//std::cout << "N " << N << "\n";

	for (auto gid : get_topsort_gates()) {
		const auto& g = getcgate(gid);
		// std::cout << "at " << wname(g.fanout) << " ";
		enumerate_cuts(g.fo0(), cuts[g.fo0()], k, N, isolated);
		// std::cout << cuts.at(g.fanout).size() << "\n";
	}


/*	for (auto gid : get_topsort_gates()) {
		std::cout << wname(gid) << " : \n";
		uint i = 0;
		for (const auto& c : cuts.at(gfanout(gid))) {
			std::cout << "c" << i++ << ": ins { ";
			for (auto wid : c.inputs) {
				std::cout << wname(wid) << " ";
			}
			std::cout << "}\n";
			for (auto gid2 : c.gates) {
				print_gate(gid2);
				if (gid2 != gid && gfanoutGates(gid2).size() > 1) {
					std::cout << "  \n to extra out ";
					for (auto goid : gfanoutGates(gid2)) {
						std::cout << "   -> ";
						print_gate(goid);
					}
				}
			}
		}
		std::cout << "\n\n";
	}*/

}

/*
 * tries to find a cut of a given size by
 */
struct cut_t {
	oidset inputs;
	oidset cgates;

	bool operator==(const cut_t& c1) {
		return (inputs == c1.inputs) && (cgates == c1.cgates);
	}
};

bool operator==(const cut_t& c0, const cut_t& c1) {
	return (c0.inputs == c1.inputs) && (c0.cgates == c1.cgates);
}

// single node cut enumeration
void circuit::enumerate_cuts(id root, vector<cir_cut_t>& cuts, int k, int N, bool isolated) const {

	cuts = { {root, oidset({root}), oidset()} };
	idvec S = {0};

	idset fanin_gids;

	if (isolated) {
		assert(wfanin0(root) != -1);
		get_gate_mffc(wfanin0(root), fanin_gids);
	}
	else {
		idset fanin_wids, fanin_pis;
		idvec fanin_gidvec;
		get_trans_fanin(root, fanin_wids, fanin_pis, fanin_gidvec);
		fanin_gids = to_hset(fanin_gidvec);
	}

	Oset<oidset> all_cut_ins;

	while(!S.empty()) {

		id curind = S.back(); S.pop_back();
		auto cp = cuts[curind].inputs;

		for (auto wid : cp) {

			id gid = wfanin0(wid);

			if (gid == -1 || _is_not_in(gid, fanin_gids))
				continue;

			const auto& g = getcgate(gid);
			auto newcut = cuts[curind];

			newcut.inputs.erase(wid);
			newcut.gates.insert(gid);

			for (auto gin : g.fanins) {
				newcut.inputs.insert(gin);
			}

			if ( !newcut.inputs.empty() && cuts.size() <= N
					&& newcut.inputs.size() <= k && newcut.inputs != cuts[curind].inputs
					&& _is_not_in(newcut.inputs, all_cut_ins) ) {
				// utl::printset(newcut.inputs);
				cuts.push_back(newcut);
				all_cut_ins.insert(newcut.inputs);
				S.push_back(cuts.size() - 1);
			}
		}
	}
}

bool circuit::find_kcut(
		id root,
		uint k,
		idset& cut_gates,
		idset& fanins,
		bool isolated) const {

	uint cut_size_limit = 10000;

	vector<cir_cut_t> cuts;
	enumerate_cuts(root, cuts, k, cut_size_limit, isolated);

	for (auto& p : cuts) {
		// utl::printset(p.first.inputs);
		if (p.inputs.size() == k) {
			fanins = to_hset(to_vec(p.inputs));
			cut_gates = to_hset(to_vec(p.gates));
			//std::cout << "cut of size " << k << " found\n";
			return true;
		}
	}

	return false;
}

bool circuit::wire_fanout_in_cone(idset& target,
		id wr, id sourceGate) const {
	std::queue<id> que;
	id curr_id = wr;

	if (wfanout(wr).size() == 1)
		return true;

	do {
		for (auto& x : wfanout(curr_id)) {
			if (x != sourceGate){
				if (target.find(x) != target.end())
					return true;
				que.push(x);
			}
		}
		curr_id = gfanout(que.front());
		que.pop();
	} while (!que.empty());

	return false;
}


bool circuit::get_kcut (id wid, uint N, idset& kcut_gset,
		idset& fanins, bool isolated, circuit *kcut_ckt) const {

	idset kcut_wset;
	return get_kcut(wid, N, kcut_gset, fanins, kcut_wset, isolated, kcut_ckt);
}

void circuit::get_kcut_topsrot(const idset& kcut_gset, const idset& fanins, idvec& sorted_gates) {

	id2idHmap degmap;

	id cur_gid = -1;
	idque Q;

	while (true) {
		for (auto xid : fanins) {
			for (auto gid : wfanout(xid)) {
				if (_is_in(gid, kcut_gset)) {
					if (_is_not_in(gid, degmap)) {
						degmap[gid] = gfanin(gid).size();
					}
					else {
						if (--degmap.at(gid) == 0) {
							sorted_gates.push_back(gid);
						}
					}
				}
			}
		}
	}

}

void circuit::get_kcut_truth_table(const idset& kcut_gset, const idset& fanins, boolvec& truth_table) {

	assert(fanins.size() <= 10);

	for (auto fanin : fanins) {

	}

}


bool circuit::get_kcut (id rootwr, uint K, idset& kcut_gset, idset& fanins,
		idset& kcut_wset, bool isolated, circuit *kcut_ckt) const {

	fanins.clear();
	kcut_gset.clear();
	kcut_wset.clear();

	if( !find_kcut(rootwr, K, kcut_gset, fanins, isolated) ) {
		// std::cout << "no cut found.  "
		//		<< "max fanin found: " << fanins.size() << "\n";
		return false;
	}

	for ( auto x : kcut_gset ) {
		for ( auto y :  gfanin(x) ) {
			kcut_wset.insert(y);
		}
	}
	kcut_wset.insert(rootwr);


	// for when a kcut circuit object is to be generated
	if (kcut_ckt != NULL) {

		id2idmap wiremap;

		for ( auto& x : kcut_wset ) {
			wiremap[x] = kcut_ckt->add_wire(node_names.at(x), wtype::INTER);
		}

		kcut_ckt->setwiretype( wiremap[rootwr], wtype::OUT);

		for ( auto& x : fanins ) {
			kcut_ckt->setwiretype( wiremap[x], wtype::IN);
		}

		for ( auto& x : kcut_gset ) {
			kcut_ckt->add_gate(gname(x), nodes.at(x), wiremap);
		}

		// kcut_ckt->write_bench();
	}
	return true;
}

idset circuit::get_dominator_gates() const {

	idset retset;
	for (auto x : prim_input_set)
		for (auto gid : wfanout(x))
			retset.insert(gid);
	add_all(retset, dffs);

	return retset;
}

void circuit::make_combinational(circuit& cir) {

	oidset dffset = cir.dffs;
	for (auto dffid : dffset) {
		id dffout = cir.gfanout(dffid);
		id dffin = cir.gfanin0(dffid);
		if (cir.isOutput(dffout)) {
			idpair op = cir.open_wire(dffout);
			cir.add_gate({op.first}, op.second, fnct::BUF);
			cir.setwiretype(op.first, wtype::IN);
		}
		else {
			cir.setwiretype(dffout, wtype::IN);
		}

		if (cir.isInput(dffin)) {
			idpair op = cir.open_wire(dffin);
			cir.add_gate({op.first}, op.second, fnct::BUF);
			cir.setwiretype(op.second, wtype::OUT);
		}
		else {
			cir.setwiretype(dffin, wtype::OUT);
		}
		cir.remove_gate(dffid);
	}

}

// TODO: what the hell is this?
id2idHmap circuit::find_feedback_arc_set2() const {

	assert(nLatches() == 0);

	idset visited;
	Hmap<id, idset> fanin_cone;

	idque Q;
	id2idHmap feedback_arc_set; // set of pairs of gate ids (u, v)

	idset dominator_gates = get_dominator_gates();

	for (auto gid : dominator_gates) {
		Q.push(gid);
		visited.insert(gid);
	}

	id cur_gid;

	while (!Q.empty()) {
		cur_gid = Q.front(); Q.pop();
		visited.insert(cur_gid);
		fanin_cone[cur_gid].insert(cur_gid);
		std::cout << "now at :" << gname(cur_gid) << "\n";
		std::cout << " with fanin set: ";
		for (auto fanin : fanin_cone.at(cur_gid)) {
			std::cout << " " << gname(fanin);
		}
		std::cout << "\n";
		std::cout << " fanout gates: ";
		for (auto goid : gfanoutGates(cur_gid)) {
			std::cout << " " << gname(goid);
			if ( _is_in(goid, visited) ) {
				if	( _is_in(goid, fanin_cone.at(cur_gid)) ) {
					std::cout << "found arc\n";
					feedback_arc_set[cur_gid] = goid;
				}
				else
					continue;
			}
			else {
				std::cout << "adding to fanin_cone\n";
				add_all(fanin_cone[goid], fanin_cone.at(cur_gid));
				Q.push(goid);
			}
		}
		std::cout << "\n";
	}

	/*
	for (auto& x : feedback_arc_set) {
		printf("(%s, %s) : %s\n", getgate_name(x.first).c_str(),
							getgate_name(x.second).c_str(),
							getwire_name(gates[x.first].fanout).c_str());
	}
	*/

	return feedback_arc_set;
}

idvec circuit::find_wire_path(id swid, id dwid) const {

	assert(swid != dwid);

	idque Q;
	Q.push(swid);
	idset visited = {swid};

	id2idHmap parent;

	bool found_path = false;
	while (!Q.empty()) {
		id cwid = Q.front(); Q.pop();
		for (auto wid : wfanoutWires(cwid)) {
			if ( _is_not_in(wid, visited) ) {
				visited.insert(wid);
				Q.push(wid);
				parent[wid] = cwid;
				if (wid == dwid) {
					found_path = true;
					break;
				}
			}
		}
		if (found_path)
			break;
	}

	if (!found_path)
		return idvec();

	idvec path = {dwid};
	id pwid = dwid;
	while (pwid != swid) {
		pwid = parent.at(pwid);
		path.push_back(pwid);
	}

	std::reverse(path.begin(), path.end());

	return path;
}

id2idHmap circuit::find_feedback_arc_set() const {

	idset visited;
	idset ontrace;

	Hmap<id, int> entry;
	Hmap<id, int> exit;

	idvec trace;
	id2idHmap feedback_arc_set; // set of pairs of gate ids (u, v)

	idset dominators = get_dominator_gates();

	id time = 1;

	if (dominators.empty())
		return feedback_arc_set;

	id cur = *(dominators.begin());
	dominators.erase(cur);

	trace.push_back(cur);
	ontrace.insert(cur);
	visited.insert(cur);

	foreach_latch((*this), dff, dffout, dffin)
		visited.insert(dff);

	while (true) {
		bool found = false;
		for (auto v : gfanoutGates(cur)) if (!isLatch(v)) {
			if ( _is_in(v, visited) && _is_in(v, ontrace) ) {
				feedback_arc_set[cur] = v;
			}
			if ( !_is_in(v, visited) ) {
				found = true;
				cur = v;
				entry[cur] = time++;
				visited.insert(cur);
				ontrace.insert(cur);
				trace.push_back(v);
				break;
			}
		}

		if (!found) {
			trace.pop_back();
			exit[cur] = time++;
			ontrace.erase(cur);
			if (trace.empty()) {
				if (dominators.empty())
					break;
				cur = *(dominators.begin());
				dominators.erase(cur);
				trace.push_back(cur);
			}
			else {
				cur = trace.back();
			}
		}
	}

	/*
	for (auto& x : feedback_arc_set) {
		printf("(%s, %s) : %s\n", getgate_name(x.first).c_str(),
							getgate_name(x.second).c_str(),
							getwire_name(gates[x.first].fanout).c_str());
	}
	*/

	return feedback_arc_set;
}

bool circuit::find_sccs(vector<idset>& all_sccs) const {

	id index = 0;
	id2idmap gate2index;
	id2idmap lowlink;
	idvec S;
	idset onstack;

	foreach_gate(*this, g, gid) {
		id v = gid;
		if (gate2index.find(v) == gate2index.end()) {
			idset scc = _strongconnect(v, index, gate2index,
					lowlink, onstack, S, all_sccs);
			if (scc.size() != 0) all_sccs.push_back(scc);
		}
	}

	/*
	if (all_sccs.size() != (uint)number_of_gates) {
		std::cout << "circuit has cycles\n";
	}
	for (auto& scc : all_sccs) {
		for (auto& g : scc) {
			std::cout << getgate_name(g) << " ";
		}
		std::cout << "\n";
	}
	*/

	return true;
}

void circuit::clear_topsort() const {
	const_cast<circuit&>(*this).topsort_needs_update = true;
}

/*
 * with non-const references this method will store the ordering
 * for fast access
 */
const idvec& circuit::get_topsort_wires() const {
	circuit& tc = const_cast<circuit&>(*this);
	if (topsort_needs_update) {
		tc.top_sort_g.clear(); tc.top_sort_w.clear();
		topsort(tc.top_sort_g, tc.top_sort_w);
		tc.topsort_needs_update = false;
	}

	return top_sort_w;
}

const idvec& circuit::get_topsort_gates() const {
	circuit& tc = const_cast<circuit&>(*this);
	if (topsort_needs_update) {
		tc.top_sort_g.clear(); tc.top_sort_w.clear();
		topsort(tc.top_sort_g, tc.top_sort_w);
		tc.topsort_needs_update = false;
	}

	return top_sort_g;
}

void circuit::topsort(idvec& gate_ordering, idvec& wire_ordering) const {

	topsort_gates(gate_ordering);
	push_all(wire_ordering, inputs());
	push_all(wire_ordering, keys());

	if (has_const0())
		wire_ordering.push_back(get_cconst0());
	if (has_const1())
		wire_ordering.push_back(get_cconst1());

	foreach_latch(*this, dffid, dffout, dffin) {
		wire_ordering.push_back(dffout);
	}

	for (auto gid : gate_ordering) {
		if ( !isLatch(gid) )
			wire_ordering.push_back(gfanout(gid));
	}

	assert(wire_ordering.size() == nWires());
}

void circuit::topsort_wires(idvec& wire_ordering) const {
	idvec gate_ordering;
	topsort(gate_ordering, wire_ordering);
}

void circuit::topsort_gates(idvec& gate_ordering) const {

	Hmap<id, id> ckt_indegmap;

	assert(gate_ordering.empty());

	idvec gateinst;
	foreach_gate(*this, g, gid) {
		gateinst.push_back(gid);
		//std::cout << ndname(gid) << " gate " << (int)g.ncat0 << "\n";
	}

	foreach_instance(*this, g, gid) {
		gateinst.push_back(gid);
		//std::cout << ndname(gid) << " instance " << (int)g.ncat0 << "\n";
	}

	for (auto gid : gateinst) {
		const auto& g = getcgate(gid);
		if (g.isGate() || g.isInst()) {
			assert(g.isGood());
			ckt_indegmap[gid] = g.fanins.size();
			g.setlevel(-1);
			for (id wid : g.fanouts) {
				getcwire(wid).setlevel(-1);
			}
		}
	}

	// add all zero fanin nodes
	idset gen_inset;
	add_all(gen_inset, inputs());
	add_all(gen_inset, keys());
	add_all(gen_inset, dff_outs());
	add_constants(gen_inset, *this);

	idque deg_zero;
	for (auto wid : gen_inset) {
		getcwire(wid).setlevel(0);
		for (auto gid : wfanout(wid)) {

			// decrement in_degree map
			ckt_indegmap.at(gid)--;

			const auto& g = getcgate(gid);

			if (!g.isLatch() && (ckt_indegmap.at(gid) == 0)) {
				deg_zero.push(gid);
				g.setlevel(1);
				for (id wid : g.fanouts)
					getcwire(wid).setlevel(1);
			}
		}
	}

	while (!deg_zero.empty()) {

		// dequeue and push to ordering
		id gid = deg_zero.front(); deg_zero.pop();
		gate_ordering.push_back(gid);
		// std::cout << "added g " << ndname(gid) << "\n";

		// reduce indegree for adjacent vertices
		const auto& g = getcgate(gid);
		for (auto wout : g.fanouts) {
			for (auto gid2 : wfanout(wout)) {

				// decrement in_degree map
				ckt_indegmap.at(gid2)--;

				// enque if the deg reaches 0
				if (!isLatch(gid2) && (ckt_indegmap.at(gid2) == 0)) {
					deg_zero.push(gid2);
					id max_level = -1;
					const auto& g2 = getcgate(gid2);
					for (auto gin : g2.fanins) {
						assert(getcwire(gin).level != -1);
						max_level = MAX(max_level, getcwire(gin).level);
					}
					id glvl = max_level + 1;
					g2.setlevel(glvl);
					for (id wout : g2.fanouts)
						getcwire(wout).setlevel(glvl);
					assert(glvl != -1);
				}
			}
		}
	}

	// std::cout << "gate ordering " << gate_ordering.size() << " " << (nGates() - nLatches()) << "\n";
	if ( gate_ordering.size() != (nGates() + nInst() - nLatches()) ) {
		auto gate_ordering_set = utl::to_hset(gate_ordering);
		for (id gid = 0; gid < this->get_max_gid(); gid++) {
			if (node_exists(gid) && (isGate(gid) || isInst(gid)) && _is_not_in(gid, gate_ordering_set)) {
				std::cout << "wire " << ndname(getcgate(gid).fo0()) << " not reached in topsort\n";
			}
		}
		neos_abort( fmt("problem during topsort : (gate_order_size) %d != %d (num non-latch gates)",
				gate_ordering.size() % (nGates() + nInst() - nLatches()) ) );
	}
}

idset circuit::_strongconnect(id v, id& index,
		id2idmap& gate2index, id2idmap& lowlink, idset& onstack,
		idvec& S, vector<idset>& all_sccs) const {

	idset scc;

	gate2index[v] = lowlink[v] = index++;
	S.push_back(v);
	onstack.insert(v);

	for (auto& w : gfanoutGates(v)) {
		if (gate2index.find(w) == gate2index.end()) {
			idset scc = _strongconnect(w, index, gate2index,
					lowlink, onstack, S, all_sccs);
			if (scc.size() != 0) all_sccs.push_back(scc);
			lowlink[v] = MIN(lowlink[v], lowlink[w]);
		}
		else if (_is_in(w, onstack) ) {
			lowlink[v] = MIN(lowlink[v], gate2index[w]);
		}
	}

	if (lowlink[v] == gate2index[v]) {
		id w;
		do {
			w = S.back();
			S.pop_back();
			onstack.insert(w);
			scc.insert(w);
		} while (w != v);
	}

	return scc;
}

circuit circuit::get_fanin_cone(id root) const {

	assert(isInter(root) || isOutput(root));

	circuit retcir = *this;

	idset fanin_wires, fanin_pis;
	idvec fanin_gates_vec;
	get_trans_fanin(root, fanin_wires, fanin_pis, fanin_gates_vec);
	fanin_wires.insert(root);

	idset fanin_gates = to_hset(fanin_gates_vec);

	idset dead_wires, dead_gates;
	foreach_wire(*this, w, wid) {
		if ( _is_not_in(wid, fanin_wires) ) {
			dead_wires.insert(wid);
		}
	}
	foreach_gate(*this, g, gid) {
		if ( _is_not_in(gid, fanin_gates)) {
			dead_gates.insert(gid);
		}
	}

	for (auto wid : dead_wires) {
		retcir.remove_wire(wid);
	}
	for (auto gid : dead_gates) {
		retcir.remove_gate(gid);
	}

	retcir.error_check();

	return retcir;
}

void circuit::get_trans_fanin (
		id wid,
		idset& faninset,
		idset& faninpis,
		idvec& faningates) const {

	std::queue<id> que;
	idset visited;

	faninset.clear();
	faninpis.clear();
	faningates.clear();

	auto w = getcwire(wid);

	if (wfanin0(wid) == -1 || isLatch(wfanin0(wid)))
		return;

	for (auto& gid : wfanins(wid)) {
		if (!isLatch(gid)) que.push(gid);
	}

	visited.insert(wid);
	id cgid;

	do {
		cgid = que.front();
		que.pop();

		faningates.push_back(cgid);

		for ( auto wid : gfanin(cgid) ) {
			if ( _is_not_in(wid, visited) ) {
				faninset.insert(wid);
				visited.insert(wid);
				if ( isInput(wid) || isKey(wid) )
					faninpis.insert(wid);
				if (!wfanins(wid).empty())
					for (auto gin : wfanins(wid))
						if (!isLatch(gin))
							que.push(gin);
			}
		}

	} while (!que.empty());

}

void circuit::get_layers(vector<idvec>& glayers, vector<idvec>& wlayers) {
	idvec start_wires = utl::to_vec(allins());
	get_layers(glayers, wlayers, start_wires);
}

void circuit::get_layers(vector<idvec>& glayers, vector<idvec>& wlayers, const idvec& start_wires) {

	wlayers = vector<idvec>(1);
	glayers = vector<idvec>(1);
	idset wvisited, gvisited;

	wlayers[0] = start_wires;
	utl::add_all(wvisited, wlayers[0]);

	uint i = 0;

	while (true) {
		bool found = false;
		for (auto wid : wlayers[i]) {
			for (auto wid2 : wfanoutWires(wid)) {
				if ( _is_not_in(wid2, wvisited) && _is_not_in(wid2, wlayers[i]) ) {
					found = true;
					if (wlayers.size() <= i+1)
						wlayers.push_back(idvec());
					wlayers[i+1].push_back(wid2);
					wvisited.insert(wid2);
				}
			}
			for (auto gid : wfanout(wid)) {
				if ( _is_not_in(gid, gvisited) ) {
					found = true;
					if (glayers.size() <= i)
						glayers.push_back(idvec());
					glayers[i].push_back(gid);
					gvisited.insert(gid);
				}
			}
		}

		if (!found)
			break;
		else
			i++;
	}

	std::cout << "\n";
	for (uint i = 0; i < wlayers.size(); i++) {
		std::cout << "layer " << i << " : ";
		uint layer_inputs = 0, layer_keys = 0, layer_inters = 0;
		for (auto wid : wlayers[i]) {
			if (isInput(wid))
				layer_inputs++;
			else if (isKey(wid))
				layer_keys++;
			else if (isInter(wid))
				layer_inters++;
			std::cout << wname(wid) << " ";
		}
		std::cout << "-> #inputs: " << layer_inputs << "  #keys: " << layer_keys << "  #inters: "
				<< layer_inters << " #gates: " << (float)glayers[i].size() / (float)nGates() << "\n";
	}

	std::cout << "\n";
	for (uint i = 0; i < glayers.size(); i++) {
		std::cout << "glayer " << i << " : ";
		for (auto gid : glayers[i]) {
			std::cout << wname(gfanout(gid)) << " ";
		}
		std::cout << "\n";
	}

}

id visitor::start(id start_node) {
	cur_node = start_node;

	if (stg == BFS) {
		que.push(cur_node);
		visited.insert(cur_node);
	}
	else {
		stack.push_back(cur_node);
		visited.insert(cur_node);
	}
	return cur_node;
}


void visitor::_visit(id node) {
	if (stg == BFS) {
		que.push(node);
	}
	else if (stg == DFS) {
		stack.push_back(node);
	}
	visited.insert(node);
}

id visitor::_pop_node() {
	id ret = 0;
	if (stg == BFS) {
		ret = que.front();
		que.pop();
	}
	else if (stg == DFS) {
		ret = stack.back();
		stack.pop_back();
	}
	return ret;
}


idset visitor::_neighbors(id node) {
	idset ret = idset();
	if (dir == direction::BACK) {
		if (nd == nodes::WIRES) {
			const auto& fanin_set = ckt.wfaninWires(node);
			ret.insert(fanin_set.begin(), fanin_set.end());
		}
		else if (nd == nodes::GATES) {
			auto& fanin_set = ckt.wfanins(node);
			ret.insert(fanin_set.begin(), fanin_set.end());
		}
	}
	else if (dir == direction::FWD) {
		if (nd == nodes::WIRES) {
			const auto& fanout_set = ckt.wfanoutWires(node);
			ret.insert(fanout_set.begin(), fanout_set.end());
		}
		else if (nd == nodes::GATES) {
			auto& fanout_set = ckt.wfanout(node);
			ret.insert(fanout_set.begin(), fanout_set.end());
		}
	}
	return ret;
}


id visitor::next() {
	if ((stg == strategy::BFS && que.empty())
		|| (stg == strategy::DFS && stack.empty()) ) {
		finish = true;
		return cur_node;
	}

	cur_node = _pop_node();
	for (auto& node : _neighbors(cur_node))
		if (_is_not_in(node, visited))
			_visit(node);

	return cur_node;
}

} // namespace ckt
