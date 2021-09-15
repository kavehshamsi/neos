#include "dec_sat.h"


namespace dec_ns {

using namespace utl;

bool dec_sat::is_combinational(const circuit& ckt, const id2boolmap& constsmap) {

	{
		sat_solver O;
		id2litmap_t omaps;
		add_to_sat(O, ckt, omaps);
		for (auto cpr : constsmap) {
			slit p = omaps.at(cpr.first);
			O.add_clause(cpr.second ? p:~p);
		}
		if (!O.solve()) {
			std::cout << "\ncircuit not satisfiable. cyclic with unbreakable internal ocillation\n";
			return false;
		}
	}

	auto feedback_arcs = ckt.find_feedback_arc_set();

	circuit opened_ckt = ckt;
	id2idmap w2wprime;

	for (auto&& p : feedback_arcs) {
		id w = ckt.gfanout(p.first);
		auto opr = opened_ckt.open_wire(w);
		w2wprime[opr.first] = opr.second;
	}

	circuit test_ckt = opened_ckt;

	test_ckt.add_circuit_byname(opened_ckt, "_$1");
	idvec vec1, vec2;
	for (auto oid : opened_ckt.outputs()) {
		id toid = test_ckt.find_wcheck(opened_ckt.wname(oid));
		id toidp = test_ckt.find_wcheck(opened_ckt.wname(oid) + "_$1");
		vec1.push_back(toid); vec2.push_back(toidp);
	}
	id out_comp = build_comparator(vec1, vec2, test_ckt, "out_ineq");

	vec1.clear(); vec2.clear();
	for (auto w2wp : w2wprime) {
		id w2wp_out = test_ckt.add_wire(wtype::INTER);
		test_ckt.add_gate({w2wp.first, w2wp.second}, w2wp_out, fnct::XNOR);
		vec1.push_back(w2wp_out);
	}
	id w_neq_wp = test_ckt.add_wire(wtype::INTER);
	test_ckt.add_gate(vec1, w_neq_wp, fnct::OR);

	// test_ckt.write_bench();

	// create sat instance
	sat_solver S;
	std::vector<slit> assumps;
	id2litmap_t test_maps;
	add_to_sat(S, test_ckt, test_maps);

	for (auto&& cpr : constsmap) {
		id xid = test_ckt.find_wcheck(ckt.wname(cpr.first));
		slit p = test_maps.at(xid);
		S.add_clause(cpr.second ? p:~p);
	}


	if (S.solve(test_maps.at(out_comp))) {
		std::cout << "circuit is stateful. circuit is cyclic\n";
		return false;
	}
	/*
	if (S.solve(test_maps.at(w_neq_wp))) {
		std::cout << "W and Wprime mismatch\n";
	}
	 */

	// std::cout << "circuit cycle cannot reach output\n";
	return true;

}

void dec_sat::_get_key_only_logic() {

	idvec stack;

	foreach_keyid(*enc_cir, kid) {
		stack.push_back(kid);
		key_only_logic.insert(kid);
	}

	while (!stack.empty()) {
		id ckid = stack.back();
		stack.pop_back();

		for (auto gid : enc_cir->wfanout(ckid)) {
			bool is_key_only = true;

			const auto& gt = enc_cir->getcgate(gid);

			for (auto gfanin : gt.fi()) {
				if (!enc_cir->isKey(gfanin)) {
					is_key_only = false;
					break;
				}
			}

			if (is_key_only && _is_not_in(gt.fo0(), key_only_logic)) {
				key_only_logic.insert(gt.fo0());
				stack.push_back(gt.fo0());
			}
		}
	}

/*	std::cout << "key only wires are: \n";
	for (auto kid : key_only_logic) {
		std::cout << enc_ckt->wname(kid) << "\n";
	}
	std::cout << "\n";*/

}

/*
// cyclic + cyclic condition
void sat_dec::_build_anticyc_condition2() {

	_get_key_only_logic();

	for (auto x : feedback_arc_set) {
		id w = enc_ckt->gfanout(x.first);
		std::cout << "\npropagating for wire "
					<< enc_ckt->wname(w) << ":\n";
		_propagate_block_condition2(w);
	}

	nc_tip = Fi.create_new_var();
	add_logic_clause(Fi, nc_ends, nc_tip, fnct::AND);

	Fi_assumps.push_back(nc_tip);

	if (!Fi.solve(Fi_assumps)) {
		neos_error("unsatisfiable non-cyclic constraints\n");
	}
	else {
	}

}

void sat_dec::_build_anticyc_condition3() {

	std::cout << "\nUsing Logic-CycSAT\n";

	_get_key_only_logic();

	for (auto& x : feedback_arc_set) {
		id w = enc_ckt->gfanout(x.first);
		feedback_wires.insert(w);
		std::cout << "\npropagating for wire "
					<< enc_ckt->wname(w) << ":\n";
		_block_feedback_arc3(w);
	}

	id nc_tip_wid = nc_cir.add_wire("nc_tip$$out", wtype::OUT);
	nc_cir.add_gate(nc_end_wids, nc_tip_wid, fnct::AND);

	id2litmap_t ncmap;
	foreach_inid(nc_cir, xid) {
		ncmap[xid] = _get_mitt_lit(nc_cir.wname(xid));
	}
	foreach_keyid(nc_cir, kid) {
		ncmap[kid] = _get_mitt_lit(nc_cir.wname(kid));
	}

	add_ckt_to_sat_necessary(Fi, nc_cir, ncmap);

	nc_tip = ncmap.at(nc_tip_wid);
	Fi_assumps.push_back(nc_tip);

	if (!Fi.solve(Fi_assumps)) {
		neos_error("unsatisfiable non-cyclic constraints\n");
	}
	else {
	}
}

void cycsat_cond_builder::_lcycsat_second_phase() {

	circuit gcir = *enc_ckt;
	for (auto& x : feedback_arc_set) {
		id w = enc_ckt->gfanout(x.first);
		gcir.open_wire(w);
	}

	Fi_assumps.pop_back(); // mitt_tip
	Fi_assumps.pop_back(); // io_tip
	assert(Fi_assumps.back() == nc_tip);
	Fi_assumps.pop_back();
	assert(Fi_assumps.empty());

	std::vector<slit> g_assumps;
	id2litmap_t glitmap;

	foreach_keyid(*enc_ckt, kid) {
		glitmap[kid] = _get_mitt_lit(enc_ckt->wname(kid));
	}

	foreach_inid(*enc_ckt, xid) {
		glitmap[xid] = _get_mitt_lit(enc_ckt->wname(xid));
	}

	add_ckt_to_sat_necessary(Fi, gcir, glitmap);

	assert(Fi.solve());

	uint lcyc_iter = 0;
	while ( Fi.solve(~nc_tip, io_tip) ) {

		if (lcyc_iter++ > 20)
			break;
		circuit ban_nc = nc_cir;
		std::cout << "NC_bar baning iteration x = ";
		id2litmap_t nc_ban_map;
		id2boolHmap constmap;
		foreach_inid(gcir, xid) {
			bool xval = Fi.get_value(glitmap.at(xid));
			std::cout << xval;
			id nc_xid = ban_nc.find_wcheck(gcir.wname(xid));
			constmap[nc_xid] = xval;
		}
		std::cout << "\n";
		ban_nc.tie_to_consts(constmap);

		foreach_keyid(ban_nc, keyid) {
			nc_ban_map[keyid] = _get_mitt_lit(ban_nc.wname(keyid));
		}
		assert(ban_nc.nOutputs() == 1);
		id outid = *(ban_nc.outputs().begin());
		nc_ban_map[outid] = io_tip;
		add_ckt_to_sat_necessary(Fi, ban_nc, nc_ban_map);

	}
	assert(Fi.solve());
	std::cout << "lcyc done\n";
	Fi_assumps.push_back(nc_tip);
	Fi_assumps.push_back(io_tip);
	Fi_assumps.push_back(mitt_tip);

}


void cycsat_cond_builder::_block_feedback_arc4(id w) {

	idset w_fanoutset, w_faninset, key_set, input_set;
	idset key_fanoutset, input_fanoutset;
	enc_ckt->get_trans_fanoutset(w, w_fanoutset);
	enc_ckt->get_trans_fanin(w, w_faninset);

	for (auto wid : w_faninset) {
		auto w = enc_ckt->getcwire(wid);
		if (w.isKey()) {
			key_set.insert(wid);
		}
		else if (w.isInput()) {
			input_set.insert(wid);
		}
	}

	enc_ckt->get_trans_fanoutset(key_set, key_fanoutset);
	enc_ckt->get_trans_fanoutset(input_set, input_fanoutset);

	// new wire that gets created as a result of openning the arc
	// the wire is the sat_end of the anticyclic condition : wprime
	nc_cir_map.clear();

	id wprime_X = enc_ckt->get_max_wid() + ++num_extra_vars;
	id wprime_K = enc_ckt->get_max_wid() + ++num_extra_vars;

	nc_cir_map[wprime_X] = nc_cir.add_wire("nc_end$X_" + std::to_string(w), wtype::INTER);
	nc_cir_map[wprime_K] = nc_cir.add_wire("nc_end$K_" + std::to_string(w), wtype::INTER);

	// this is the only zero in the condition and it is w
	nc_cir_map[w] = nc_cir.get_const0();

	std::cout << "Blocking on X\n";
	{ // block on X
		idset visited;
		idque Q;
		id cur_w;
		Q.push(w);
		visited.insert(w);

		while (!Q.empty()) {
			cur_w = Q.front(); Q.pop();

			id gid = enc_ckt->wfanin0(cur_w);
			if (gid == -1)
				continue;

			gate gt = enc_ckt->getcgate(gid);

			if (gt.fo0() == w) {
				gt.fo0() = wprime_X;
			}

			idvec blockee, blocker;
			std::cout << "\nnow looking at ";
			enc_ckt->print_gate(gid);

			for (auto x : gt.fi()) {
				const auto& xw = enc_ckt->getcwire(x);
				if (_is_not_in(x, visited)) {
					visited.insert(x);
					if (x == w) {}
					else if (xw.isInput()) {
						id xid = nc_cir.find_wire(enc_ckt->wname(x));
						nc_cir_map[x] = ( xid != -1) ? xid:nc_cir.add_wire(enc_ckt->wname(x), wtype::IN);
					}
					else if (xw.isKey()) {
						nc_cir_map[x] = nc_cir.get_const1();
					}
					else {
						if ( _is_in(x, input_fanoutset) || _is_in(x, w_fanoutset) ) {
							Q.push(x);
							nc_cir_map[x] = nc_cir.add_wire(wtype::INTER);
						}
						else {
							nc_cir_map[x] = nc_cir.get_const1();
						}
					}
				}

				if ( (x == w || _is_in(x, w_fanoutset) || _is_not_in(x, input_fanoutset)) && !xw.isInput() ) {
					std::cout << "gate input is blockee: " << enc_ckt->wname(x) << "\n";
					blockee.push_back(x);
				}
				else {
					std::cout << "gate input is blocker: " << enc_ckt->wname(x) << "\n";
					blocker.push_back(x);
				}

				//assert(_is_in(x, cycmap));
			}

			// _add_gate_blocking_condition(gt, blocker, blockee);
			_add_gate_blocking_condition_to_cir(gt, blocker, blockee);
		}
	}

	std::cout << "now blocking for K\n";

	{ // block on K
		for (auto kc : {0, 1}) {
			idset visited;
			idque Q;
			id cur_w;
			Q.push(w);
			visited.insert(w);

			while (!Q.empty()) {
				cur_w = Q.front(); Q.pop();

				id gid = enc_ckt->wfanin0(cur_w);
				if (gid == -1)
					continue;

				gate gt = enc_ckt->getcgate(gid);

				if (gt.fo0() == w) {
					gt.fo0() = wprime_K;
				}

				idvec blockee, blocker;
				std::cout << "\nnow looking at ";
				enc_ckt->print_gate(gid);

				for (auto x : gt.fi()) {
					const auto& xw = enc_ckt->getcwire(x);
					if (_is_not_in(x, visited)) {
						visited.insert(x);
						if (x == w) {}
						else if (xw.isInput()) {
							nc_cir_map[x] = nc_cir.get_const1();
						}
						else if (xw.isKey()) {
							std::string kname = enc_ckt->wname(x) + (kc == 0 ? "":"_$1");
							id kid = nc_cir.find_wire(kname);
							nc_cir_map[x] = (kid != -1) ? kid:nc_cir.add_wire(kname, wtype::KEY);
						}
						else {
							if ( _is_in(x, key_fanoutset) || _is_in(x, w_fanoutset) ) {
								Q.push(x);
								nc_cir_map[x] = nc_cir.add_wire(wtype::INTER);
							}
							else {
								nc_cir_map[x] = nc_cir.get_const1();
							}
						}
					}

					if ( (x == w || _is_in(x, w_fanoutset) || _is_not_in(x, key_fanoutset)) && !xw.isKey() ) {
						std::cout << "gate input is blockee: " << enc_ckt->wname(x) << "\n";
						blockee.push_back(x);
					}
					else {
						std::cout << "gate input is blocker: " << enc_ckt->wname(x) << "\n";
						blocker.push_back(x);
					}

					//assert(_is_in(x, cycmap));
				}

				// _add_gate_blocking_condition(gt, blocker, blockee);
				_add_gate_blocking_condition_to_cir(gt, blocker, blockee);
			}
		}
	}

	id nc_end = nc_cir.add_wire("nc_end$_" + std::to_string(w), wtype::INTER);
	nc_end_wids.push_back(nc_end);
	nc_cir.add_gate({nc_cir_map.at(wprime_X), nc_cir_map.at(wprime_K)}, nc_end, fnct::OR);

}
*/

void cycsat_cond_builder::_block_feedback_arc3(id w) {

	idset w_fanoutset, w_faninset, key_set, input_set;
	idset key_fanoutset, input_fanoutset;
	enc_cir->get_trans_fanoutset(w, w_fanoutset);
	enc_cir->get_trans_fanin(w, w_faninset);

	/*
	for (auto wid : w_faninset) {
		auto w = enc_ckt->getcwire(wid);
		if (w.isKey()) {
			key_set.insert(wid);
		}
		else if (w.isInput()) {
			input_set.insert(wid);
		}
	}

	enc_ckt->get_trans_fanoutset(key_set, key_fanoutset);
	enc_ckt->get_trans_fanoutset(input_set, input_fanoutset);
 	 */


	// new wire that gets created as a result of openning the arc
	// the wire is the sat_end of the anticyclic condition : wprime
	nc_cir_map.clear();
	id wprime = _tp.enc_cir->get_max_wid() + ++num_extra_vars;
	nc_cir_map[wprime] = nc_cir.add_wire("nc_end$_" + std::to_string(w), wtype::INTER);
	nc_end_wids.push_back(nc_cir_map[wprime]);

	// this is the only zero in the condition and it is w
	nc_cir_map[w] = nc_cir.get_const0();

	for (auto kc : {0, 1}) {

		idset visited;
		idque Q;
		id cur_w;
		Q.push(w);
		visited.insert(w);

		while (!Q.empty()) {
			cur_w = Q.front(); Q.pop();

			id gid = _tp.enc_cir->wfanin0(cur_w);
			if (gid == -1)
				continue;

			gate gt = _tp.enc_cir->getcgate(gid);

			if (gt.fo0() == w) {
				gt.fo0() = wprime;
			}

			idvec blockee, blocker;
			std::cout << "\nnow looking at ";
			enc_cir->print_gate(gid);
			for (auto x : gt.fi()) {
				if (_is_not_in(x, visited)) {
					visited.insert(x);
					const auto& xw = enc_cir->getcwire(x);
					if (x == w) {}
					else if (xw.isInput()) {
						id xid = nc_cir.find_wire(enc_cir->wname(x));
						nc_cir_map[x] = (xid != -1) ? xid:nc_cir.add_wire(enc_cir->wname(x), wtype::IN);
					}
					else if (xw.isKey()) {
						std::string kname = enc_cir->wname(x) + (kc == 0 ? "":"_$1");
						id kid = nc_cir.find_wire(kname);
						nc_cir_map[x] = (kid != -1) ? kid:nc_cir.add_wire(kname, wtype::KEY);
					}
					else {
						nc_cir_map[x] = nc_cir.add_wire(wtype::INTER);
						Q.push(x);
					}
				}

				if (_is_in(x, w_fanoutset)) {
					std::cout << "gate input is blockee: " << enc_cir->wname(x) << "\n";
					blockee.push_back(x);
				}
				else {
					std::cout << "gate input is blocker: " << enc_cir->wname(x) << "\n";
					blocker.push_back(x);
				}

				//assert(_is_in(x, cycmap));
			}

			if (_is_not_in(gt.fo0(), w_fanoutset)) {
				assert (gt.fo0() == wprime || blockee.empty());
			}
			// _add_gate_blocking_condition(gt, blocker, blockee);
			_add_gate_blocking_condition_to_cir(gt, blocker, blockee);
		}
	}
}



void dec_sat::_build_anticyc_condition() {
	cycptr = std::make_unique<cycsat_cond_builder>(*this);
	cycptr->build_anticyc_condition();
}

void cycsat_cond_builder::build_anticyc_condition() {

	std::cout << "Adding non-cyclic condition using IcySAT-I\n";

	for (auto& x : _tp.feedback_arc_set) {
		id w = _tp.enc_cir->gfanout(x.first);
		/*std::cout << "\npropagating for wire "
					<< enc_ckt->wname(w) << ":\n";*/
		if (_tp.sim_cyclic && _tp.enc_cyclic)
			_block_feedback_arc3(w);
		else
			_block_feedback_arc(w);
	}

	/*nc_tip = Fi.create_new_var();
	add_logic_clause(Fi, nc_ends, nc_tip, fnct::AND);
	 */

	//push_all(Fi_assumps, nc_assumps);
	id nc_tip_wid = nc_cir.add_wire("nc_tip$$out", wtype::OUT);
	nc_cir.add_gate(nc_end_wids, nc_tip_wid, fnct::AND);

	// nc_cir.write_bench();

	id2litmap_t ncmap;

	nc_tip = ncmap[nc_tip_wid] = _tp.Fi.create_new_var();

	foreach_keyid(nc_cir, kid) {
		ncmap[kid] = _tp._get_mitt_lit(nc_cir.wname(kid));
	}
	add_ckt_to_sat_necessary(_tp.Fi, nc_cir, ncmap);

	_tp.precond_assumps.push_back(nc_tip);

	if (!_tp.Fi.solve(nc_tip)) {
		neos_error("unsatisfiable non-cyclic constraints\n");
	}
	else {
		_tp.Fi.add_clause(nc_tip);
	}
}

void cycsat_cond_builder::_block_feedback_arc(id w) {

	idset fanoutset, faninset;
	enc_cir->get_trans_fanoutset(w, fanoutset);
	enc_cir->get_trans_fanin(w, faninset);

	nc_cir_map.clear();
	// new wire that gets created as a result of openning the arc
	// the wire is the sat_end of the anticyclic condition : wprime
	id wprime = enc_cir->get_max_wid() + ++num_extra_vars;
	id nc_end_wid = nc_cir_map[wprime] = nc_cir.add_wire("nc_end$_" + std::to_string(w), wtype::INTER);
	nc_end_wids.push_back(nc_cir_map.at(wprime));

	// this is the only zero in the condition and it is w
	nc_cir_map[w] = nc_cir.get_const0();

	for (auto kc : {0, 1}) {

		idset visited;

		idque Q;

		id cur_w;
		Q.push(w);
		visited.insert(w);

		idset is_blockee, is_blocker;
		is_blockee.insert(w);

		while (!Q.empty()) {
			cur_w = Q.front(); Q.pop();

			id gid = enc_cir->wfanin0(cur_w);
			if (gid == -1)
				continue;

			gate gt = enc_cir->getcgate(gid);

			if (gt.fo0() == w) {
				gt.fo0() = wprime;
			}

			idvec blockee, blocker;
			V1(std::cout << "\nnow looking at ";
			enc_cir->print_gate(gid);)
			idset one_wires, key_only_wires;
			for (auto x : gt.fi()) {
				if (_is_not_in(x, visited)) {
					visited.insert(x);
					const auto& xw = enc_cir->getcwire(x);
					if (x == w) {}
					else if (xw.isKey()) {
						is_blocker.insert(x);
						std::string kname = enc_cir->wname(x) + (kc == 0 ? "":"_$1");
						id kid = nc_cir.find_wire(kname);
						nc_cir_map[x] = (kid != -1) ? kid:nc_cir.add_wire(kname, wtype::KEY);
					}
					else if (_is_not_in(x, fanoutset)) {
						if (_is_in(x, _tp.key_only_logic)) {
							is_blocker.insert(x);
							nc_cir_map[x] = nc_cir.add_wire(wtype::INTER);
							Q.push(x);
						}
						else {
							is_blockee.insert(x);
							nc_cir_map[x] = nc_cir.get_const1();
						}
					}
					else {
						is_blockee.insert(x);
						nc_cir_map[x] = nc_cir.add_wire(wtype::INTER);
						Q.push(x);
					}
				}

				assert(_is_in(x, is_blockee) || _is_in(x, is_blocker));

				if (_is_in(x, is_blockee)) {
					V1(std::cout << "gate input is blockee: " << enc_cir->wname(x) << "\n");
					blockee.push_back(x);
				}
				else {
					V1(std::cout << "gate input is blocker: " << enc_cir->wname(x) << "\n");
					blocker.push_back(x);
				}

				assert(_is_in(x, nc_cir_map));
			}

			if (_is_not_in(gt.fo0(), fanoutset)) {
				assert (gt.fo0() == wprime || blockee.empty());
			}
			// _add_gate_blocking_condition(gt, blocker, blockee);
			_add_gate_blocking_condition_to_cir(gt, blocker, blockee);
		}
	}
}


void cycsat_cond_builder::_block_feedback_arc_stable(id w) {

	idset fanoutset, faninset;
	enc_cir->get_trans_fanoutset(w, fanoutset);
	enc_cir->get_trans_fanin(w, faninset);

	idvec S;

	cycmap.clear();
	// new wire that gets created as a result of openning the arc
	// the wire is the sat_end of the anticyclic condition : wprime
	id wprime = enc_cir->get_max_wid() + ++num_extra_vars;
	slit cycsat_end = create_variable(Fi, cycmap, wprime);
	nc_assumps.push_back(cycsat_end);

	// this is the only zero in the condition and it is w
	slit wvar = create_variable(Fi, cycmap, w);
	nc_assumps.push_back(~wvar);

	idset visited;

	id cur_w;
	S.push_back(w);
	visited.insert(w);

	idset key_wires, one_wires, free_wires;

	while (!S.empty()) {
		cur_w = S.back();
		S.pop_back();

		for (auto& x : enc_cir->wfaninWires(cur_w)) {
			if (_is_not_in(x, visited)) {

				visited.insert(x);

				if (enc_cir->isKey(x)) {
					key_wires.insert(x);
				}
				else if (_is_not_in(x, fanoutset)) {
					if (x == w) {} // avoid w
					else if (_is_not_in(x, _tp.key_only_logic)) {
						one_wires.insert(x);
					}
					else {
						free_wires.insert(x);
						S.push_back(x);
					}
				}
				else {
					free_wires.insert(x);
					S.push_back(x);
				}
			}
		}
	}

	_build_cycsat_cone(key_wires, one_wires, free_wires, wprime, w);
}


void cycsat_cond_builder::_propagate_block_condition2(id w) {

	id wprime1 = enc_cir->get_max_wid() + ++num_extra_vars;
	id wprime2 = enc_cir->get_max_wid() + ++num_extra_vars;
	assert(!enc_cir->wire_exists(wprime1) && !enc_cir->wire_exists(wprime2));

	wprimes1.push_back(wprime1);
	wprimes2.push_back(wprime2);

	for (auto kc : {0, 1}) {

		idset visited;
		idque Q;
		id wprime = kc == 0 ? wprime1:wprime2;

		Q.push(enc_cir->wfanin0(w));
		assert(enc_cir->wfanin0(w) != -1);

		std::cout << "now adding w as " << kc << "\n";
		slit p = cycmap[w] = Fi.create_new_var();
		nc_ends.push_back(kc == 0 ? p:~p);
		visited.insert(w);

		while ( !Q.empty() ) {
			id gid = Q.front(); Q.pop();

			gate g = enc_cir->getcgate(gid);
			// deal with fanout
			if (g.fo0() == w) {
				std::cout << "fanout is " << wprime << "\n";
				g.fo0() = wprime;
				cycmap[g.fo0()] = create_variable(Fi, cycmap, wprime);
			}

			// deal with fanin
			for (auto wid : g.fanins) {
				auto& wobj = enc_cir->getcwire(wid);

				if ( _is_not_in(wid, visited) ) {
					visited.insert(wid);

					id gin = enc_cir->wfanin0(wid);
					if (gin != -1)
						Q.push(gin);

					if (wobj.isInput()) {
						//std::cout << "adding input " << enc_ckt->wname(wid) << "\n";
						id mitt_xid = _tp.mitt_ckt.find_wcheck(enc_cir->wname(wid));
						cycmap[wid] = _tp.mitt_maps.at(mitt_xid);
					}
					else if (wobj.isKey()) {
						//std::cout << "adding key " << enc_ckt->wname(wid) << "\n";
						id mitt_kid = _tp.mitt_ckt.find_wcheck(enc_cir->wname(wid) + (kc == 0 ? "_$1":""));
						cycmap[wid] = _tp.mitt_maps.at(mitt_kid);
					}
					else {
						//std::cout << "adding intermediate " << enc_ckt->wname(wid) << "\n";
						cycmap[wid] = create_variable(Fi, cycmap, wid);
					}
				}
			}

			add_gate_clause(Fi, g, cycmap);
		}
	}

	// add XOR to compare wprime1 and wprime2
	slit out = Fi.create_new_var();
	add_logic_clause(Fi, {cycmap.at(wprime1), cycmap.at(wprime2)}, out, fnct::XNOR);

	nc_ends.push_back(out);
}


void cycsat_cond_builder::_build_cycsat_cone(const idset& key_wires, const idset& one_wires,
		const idset& free_wires, id wprime, id w) {


	// set of gates that will become part of SAT cone
	idset inner_gates;
	auto& wfanins = enc_cir->wfanins(w);
	inner_gates.insert(wfanins.begin(), wfanins.end());

	// first NC condition instance
	// create variables for gate wires
	std::cout << "free wires: ";
	for (auto x : free_wires) {
		std::cout << enc_cir->wname(x) << " ";
		create_variable(Fi, cycmap, x);
		auto& xfanins = enc_cir->wfanins(x);
		inner_gates.insert(xfanins.begin(), xfanins.end());
	}

	std::cout << "\none wires: ";
	for (auto x : one_wires) {
		std::cout << enc_cir->wname(x) << " ";
		slit s = create_variable(Fi, cycmap, x);
		Fi.add_clause(s);
	}
	std::cout << "\nkey wires: ";
	for (auto k : key_wires) {
		std::cout << enc_cir->wname(k) << " ";
		id mitt_kid = _tp.mitt_ckt.find_wcheck(enc_cir->wname(k));
		cycmap[k] = _tp.mitt_maps.at(mitt_kid);
	}

	// add gate clauses
	for (auto g : inner_gates) {
		gate gt = enc_cir->getcgate(g);
		_add_nc_condition_for_gate(gt, w, wprime);
	}
	std::cout << "\n";

	// add second NC condition instance
	for (auto x : free_wires) {
		create_variable(Fi, cycmap, x);
	}
	for (auto k : key_wires) {
		id mitt_kid = _tp.mitt_ckt.find_wcheck(enc_cir->wname(k) + "_$1");
		cycmap[k] = _tp.mitt_maps.at(mitt_kid);
	}

	for (auto g : inner_gates) {
		gate gt = enc_cir->getcgate(g);
		_add_nc_condition_for_gate(gt, w, wprime);
	}
}

void cycsat_cond_builder::_add_gate_blocking_condition_to_cir(const gate& gt,
		const idvec& blocker_fanins, const idvec& blockee_fanins) {

	assert(blockee_fanins.size() + blocker_fanins.size() == gt.fi().size());

	if (blockee_fanins.empty()) {
		// is a key-only gate. no change needed
		assert(blocker_fanins == gt.fi());
		//std::cout << "key only gate: " << enc_ckt->wname(gt.fanout) << "\n";
		nc_cir.add_gate("", gt, nc_cir_map);
	}
	else if (!blockee_fanins.empty() && !blocker_fanins.empty()) {
		// is input-and-key gate
		switch (gt.gfun()) {
		// for xor/xnor -> BUF and key is discarded
		case fnct::XOR:
		case fnct::XNOR: {
			nc_cir.add_gate(idvec(1, blockee_fanins[0]), gt.fo0(), fnct::BUF, nc_cir_map);
			break;
		}
		// for AND/NAND -> use NAND
		case fnct::NAND:
		case fnct::AND: {
			id dwid1 = nc_cir.add_wire(wtype::INTER);
			id dwid2 = nc_cir.add_wire(wtype::INTER);

			idvec mp_blockee_fanins, mp_blocker_fanins;

			V1(std::cout << "adding AND with inputs: ";)
			for (auto nk : blockee_fanins) {
				V1(std::cout << "nk: " << enc_cir->wname(nk) << "   ";)
				mp_blockee_fanins.push_back(nc_cir_map.at(nk));
			}
			for (auto nk : blocker_fanins) {
				V1(std::cout << "  k : " << enc_cir->wname(nk) << "   ";)
				mp_blocker_fanins.push_back(nc_cir_map.at(nk));
			}
			V1(std::cout << "\n");

			nc_cir.add_gate(mp_blocker_fanins, dwid1, fnct::NAND);
			nc_cir.add_gate(mp_blockee_fanins, dwid2, fnct::AND);
			nc_cir.add_gate({dwid1, dwid2}, nc_cir_map.at(gt.fo0()), fnct::OR);
			break;
		}
		// for OR/NOR -> OR of AND
		case fnct::OR:
		case fnct::NOR: {
			id dwid1 = nc_cir.add_wire(wtype::INTER);
			id dwid2 = nc_cir.add_wire(wtype::INTER);

			idvec mp_blockee_fanins, mp_blocker_fanins;

			V1(std::cout << "adding AND with inputs: ");
			for (auto nk : blockee_fanins) {
				V1(std::cout << "nk: " << enc_cir->wname(nk) << "   ");
				mp_blockee_fanins.push_back(nc_cir_map.at(nk));
			}
			for (auto nk : blocker_fanins) {
				V1(std::cout << "  k : " << enc_cir->wname(nk) << "   ");
				mp_blocker_fanins.push_back(nc_cir_map.at(nk));
			}
			V1(std::cout << "\n");

			nc_cir.add_gate(mp_blocker_fanins, dwid1, fnct::OR);
			nc_cir.add_gate(mp_blockee_fanins, dwid2, fnct::AND);
			nc_cir.add_gate({dwid1, dwid2}, nc_cir_map.at(gt.fo0()), fnct::OR);
			break;
		}
		// only for key-controlled MUXes
		case fnct::MUX : {
			V1(
			std::cout << "adding mux-logic with inputs : ";
			for (auto fanin : gt.fi()) {
				std::cout << enc_cir->wname(fanin) << "  ";
			}
			std::cout << "\n";
			)
			if ( _is_in(gt.fi()[0], blocker_fanins) ) {
				if (blocker_fanins.size() == 1) {
					nc_cir.add_gate(gt.fi(), gt.fo0(), fnct::MUX, nc_cir_map);
				}
				else {
					assert(blocker_fanins.size() != 3);
					std::cout << "special mux condition\n";
					id blocker_fanin =  _is_in(gt.fi()[1], blocker_fanins) ? gt.fi()[1]:gt.fi()[0];
					id blockee_fanin = (blocker_fanin == gt.fi()[1]) ? gt.fi()[0]:gt.fi()[1];
					if (blockee_fanin == gt.fi()[1]) {
						nc_cir.add_gate({blocker_fanin, blockee_fanin}, gt.fo0(), fnct::OR, nc_cir_map);
					}
					else {
						id dwid1 = nc_cir.add_wire(wtype::INTER);
						nc_cir.add_gate({nc_cir_map.at(blockee_fanin)}, dwid1, fnct::NOT);
						nc_cir.add_gate({dwid1, nc_cir_map.at(blocker_fanin)}, nc_cir_map.at(gt.fo0()), fnct::NAND);
					}
				}
				break;
			}
			else {
				// nc_cir.add_gate({gt.fanin[0]}, gt.fanout, fnct::BUF, nc_cir_map);
				neos_abort("this kind of mux gate not supported");
			}
			break;
		}
		// for tri-buf -> use tri-buf
		case fnct::TBUF : {
			V1( std::cout << "adding TBUF\n"; )
			nc_cir.add_gate(gt.fi(), gt.fo0(), fnct::TBUF, nc_cir_map);
			break;
		}
		default: {
			neos_abort("unsuported gate type " <<
					funct::enum_to_string(gt.gfun())
			<< " " << enc_cir->wname(gt.fo0())
					<< " in non-cyclic condition\n");
		}

		}
	}
	else if (blocker_fanins.empty()) {
		V1( std::cout << "adding and-logic with inputs : ";
		for (auto fanin : gt.fi()) {
			std::cout << enc_cir->wname(fanin) << "  ";
		}
		std::cout << "\n";
		)
		// is free_wire-only gate. is always AND gate
		nc_cir.add_gate(gt.fi(), gt.fo0(), fnct::AND, nc_cir_map);
	}
}


void cycsat_cond_builder::_add_gate_blocking_condition(const gate& gt,
		const idvec& blocker_fanins, const idvec& blockee_fanins) {

	assert(blockee_fanins.size() + blocker_fanins.size() == gt.fi().size());

	if (blockee_fanins.empty()) {
		// is a key-only gate. no change needed
		assert(blocker_fanins == gt.fi());
		//std::cout << "key only gate: " << enc_ckt->wname(gt.fanout) << "\n";
		add_gate_clause(Fi, gt.fi(), gt.fo0(), gt.gfun(), cycmap);
	}
	else if (!blockee_fanins.empty() && !blocker_fanins.empty()) {
		// is input-and-key gate
		switch (gt.gfun()) {
		// for xor/xnor -> BUF and key is discarded
		case fnct::XOR:
		case fnct::XNOR: {
			add_gate_clause(Fi, idvec(1, blockee_fanins[0]), gt.fo0(), fnct::BUF, cycmap);
			break;
		}
		// for AND/NAND -> use NAND
		case fnct::NAND:
		case fnct::AND: {
			id dwid1 = enc_cir->get_max_wid() + ++num_extra_vars;
			id dwid2 = enc_cir->get_max_wid() + ++num_extra_vars;

			create_variable(Fi, cycmap, dwid1);
			create_variable(Fi, cycmap, dwid2);

			std::cout << "adding AND with inputs: ";
			for (auto nk : blockee_fanins) {
				std::cout << "nk: " << enc_cir->wname(nk) << "   ";
			}
			for (auto nk : blocker_fanins) {
				std::cout << "  k : " << enc_cir->wname(nk) << "   ";
			}
			std::cout << "\n";

			add_gate_clause(Fi, blocker_fanins, dwid1, fnct::NAND, cycmap);
			add_gate_clause(Fi, blockee_fanins, dwid2, fnct::AND, cycmap);
			add_gate_clause(Fi, {dwid1, dwid2}, gt.fo0(), fnct::OR, cycmap);
			break;
		}
		// for OR/NOR -> OR of AND
		case fnct::OR:
		case fnct::NOR: {
			id dwid1 = enc_cir->get_max_wid() + ++num_extra_vars;
			id dwid2 = enc_cir->get_max_wid() + ++num_extra_vars;

			create_variable(Fi, cycmap, dwid1);
			create_variable(Fi, cycmap, dwid2);

			std::cout << "adding AND with inputs: ";
			for (auto nk : blockee_fanins) {
				std::cout << "nk: " << enc_cir->wname(nk) << "   ";
			}
			for (auto nk : blocker_fanins) {
				std::cout << "  k : " << enc_cir->wname(nk) << "   ";
			}
			std::cout << "\n";

			add_gate_clause(Fi, blocker_fanins, dwid1, fnct::OR, cycmap);
			add_gate_clause(Fi, blockee_fanins, dwid2, fnct::AND, cycmap);
			add_gate_clause(Fi, {dwid1, dwid2}, gt.fo0(), fnct::OR, cycmap);
			break;
		}
		// only for key-controlled MUXes
		case fnct::MUX : {
			std::cout << "adding mux-logic with inputs : ";
			for (auto fanin : gt.fi()) {
				std::cout << enc_cir->wname(fanin) << "  ";
			}
			std::cout << "\n";
			add_gate_clause(Fi, gt.fi(), gt.fo0(), fnct::MUX, cycmap);
			break;
		}
		// for tri-buf -> use tri-buf
		case fnct::TBUF : {
			std::cout << "adding TBUF\n";
			add_gate_clause(Fi, gt.fi(), gt.fo0(), fnct::TBUF, cycmap);
			break;
		}
		default: {
			std::cout << "unsuported gate type " <<
					funct::enum_to_string(gt.gfun())
			<< " " << enc_cir->wname(gt.fo0())
					<< " in non-cyclic condition\n";
			exit(EXIT_FAILURE);
		}

		}
	}
	else if (blocker_fanins.empty()) {
		std::cout << "adding and-logic with inputs : ";
		for (auto fanin : gt.fi()) {
			std::cout << enc_cir->wname(fanin) << "  ";
		}
		std::cout << "\n";
		// is free_wire-only gate. is always AND gate
		add_gate_clause(Fi, gt.fi(), gt.fo0(), fnct::AND, cycmap);
	}
}

void cycsat_cond_builder::_add_nc_condition_for_gate(gate gt, id w, id wprime) {

	auto& wfanins = enc_cir->wfanins(w);
	if (_is_in(gt.nid, wfanins)) {
		gt.fo0() = wprime;
		std::cout << "gate controls wprime  ";
	}

	idvec nonkey_fanins, key_fanins;
	for (auto x : gt.fi()) {
		if (enc_cir->isKey(x) || _is_in(x, _tp.key_only_logic))
			key_fanins.push_back(x);
		else
			nonkey_fanins.push_back(x);
	}

	_add_gate_blocking_condition(gt, key_fanins, nonkey_fanins);

}

void dec_sat_besat::solve() {

	//duplicate circuit for DI
	build_mitter(*enc_cir);

	// runs in O(n) for circuits with tribufs
	_add_no_float_condition();

	iovecckt = *enc_cir;

	//initialization phase
	int iter = 0;

	Fi_assumps.push_back(io_tip);
	Fi_assumps.push_back(mitt_tip);

	// some randomization for better simulation resutls
	Fi.reseed_solver();

	//SAT attack Loop
	while ( true ) {
		iter++;
		print_stats(iter);

		iopair_t dp;
		int ret = solve_for_dip(dp);

		if (ret == 0) {
			break;
		}

		last_dip = cur_dip;
		cur_dip = dp;

		get_inter_key();

		if (_besat_check_key()) {
			print_stats(iter);
			//Record observations in SAT
			dp.y = query_oracle(dp.x);
			create_ioconstraint(dp, iovecckt);
		}

		// check iteration cap
		if (iteration_limit != -1
				&& iter >= iteration_limit)
			break;
	}

	std::cout << "banned keys: " << banned_keys << "\n";

	//Extract key from SAT
	solve_key();
	return;
}


bool dec_sat_besat::_besat_check_key() {

	if (cur_dip.x == last_dip.x) {
		//std::cout << "banning key " << banned_keys++ << "\n";

		for (auto kc : {0, 1}) {
			id2boolmap kmap;
			for (auto&& kp : mitt_keyid2lit_maps[kc]) {
				kmap[kp.first] = Fi.get_value(kp.second);
			}

			//if (!is_combinational(*enc_ckt, kmap)) {
				std::vector<slit> ban_clause;
				//std::cout << "clause : ";
				for (auto&& kp : mitt_keyid2lit_maps[kc]) {
					// std::cout << (Fi.get_value(kp.second) ? "~":"") << enc_ckt->wname(kp.first) << "  ";
					ban_clause.push_back(Fi.get_value(kp.second) ? ~kp.second:kp.second);
				}
				Fi.add_clause(ban_clause);
			//}
		}
		return false;
	}

	return true;
}


uint dec_sat::_get_cyclic_depth(const circuit& enc_ckt) {


	auto feedback_arc_pairs = enc_ckt.find_feedback_arc_set();
	idset feedback_wires;
	for (auto& p : feedback_arc_pairs) {
		id w = enc_ckt.gfanout(p.first);
		feedback_wires.insert(w);
	}

	uint max_depth = 0;
	for (auto w : feedback_wires) {

		idque Q;
		idset visited;

		Q.push(w);
		visited.insert(w);
		uint num_feedbacks = 0;

		while (!Q.empty()) {
			id cur_w = Q.front(); Q.pop();

			for (auto fow : enc_ckt.wfanoutWires(cur_w)) {
				if (_is_not_in(fow, visited)) {
					visited.insert(fow);
					if (_is_in(fow, feedback_wires))
						num_feedbacks++;
					Q.push(fow);
				}
			}
		}

		std::cout << "for feedback wire " << enc_ckt.wname(w) << " depth is " << num_feedbacks << "\n";
		max_depth = MAX(max_depth, num_feedbacks);
	}

	std::cout << "unrolling with depth: " << max_depth + 1 << "\n";
	return max_depth + 1;

}

}
