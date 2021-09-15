/*
 * circuit_simplify.cpp
 *
 *  Created on: Jul 10, 2018
 *      Author: kaveh
 */

#include "base/circuit.h"

//#define NEOS_DEBUG_LEVEL 1

D1(
namespace ext {
	using namespace ckt;
	extern int check_equivalence_abc(const circuit& cir1, const circuit& cir2,
			bool fill_missing_ports = true, bool quiet = false);
}
)

namespace ckt {

void circuit::remove_dead_logic(bool remove_input) {

	idque Q;
	idset visited_w;
	idset visited_g;

	for (auto oid : outputs()) {
		Q.push(oid);
		visited_w.insert(oid);
	}

	while (!Q.empty()) {
		id wid = Q.front(); Q.pop();

		for (auto gid : wfanins(wid)) {
			visited_g.insert(gid);
		}

		for (auto wid2 : wfaninWires(wid)) {
			if (_is_not_in(wid2, visited_w)) {
				visited_w.insert(wid2);
				Q.push(wid2);
			}
		}

	}

	idset to_remove_g, to_remove_w;

	foreach_gate((*this), g, gid)
		if (_is_not_in(gid, visited_g))
			to_remove_g.insert(gid);

	foreach_instance((*this), g, gid)
		if (_is_not_in(gid, visited_g))
			to_remove_g.insert(gid);

	foreach_wire((*this), w, wid)
		if (_is_not_in(wid, visited_w))
			to_remove_w.insert(wid);

	for (auto wid : to_remove_w)
		if ( !isInput(wid) || remove_input )
			remove_wire(wid);
	for (auto gid : to_remove_g)
		remove_node(gid);

}

// subcircuit limited to subset of outputs
void circuit::trim_circuit_to(const idset& outs, bool remove_input) {

	oidset orig_outs = outputs();
	for (auto oid : orig_outs) {
		if (_is_not_in(oid, outs)) {
			setwiretype(oid, wtype::INTER);
		}
	}
	for (auto oid : outs) {
		setwiretype(oid, wtype::OUT);
	}

	remove_dead_logic(remove_input);
}

void circuit::tie_to_constant(const id2boolHmap& consts_map) {
	for (auto& p : consts_map) {
		int isc = isConst(p.first);
		if (isc != 0) {
			if ( isc - 1 != p.second ) {
				neos_error("trying to tie constant 0 to 1");
			}
			continue;
		}
		tie_to_constant(p.first, p.second);
	}
}

void circuit::tie_to_constant(id wid, bool v) {

	auto& w = getwire(wid);
	assert(w.isWire());
	assert(!isConst(wid));
	assert(w.fanins.empty());

	//std::cout << "tying wire " << wname(wid) << " to " << v <<  "\n";
	if (w.isOutput()) {
		//std::cout << "is output\n";
		id cwid = v ? get_const1():get_const0();
		assert(cwid != wid);
		add_gate({cwid}, wid, fnct::BUF);
	}
	else {
		//std::cout << "is input\n";
		id cwid = v ? get_const1():get_const0();
		assert(wid != cwid);
		move_fanouts_to(wid, cwid);
		getwire(wid).fanouts.clear();
		getwire(wid).fanins.clear();
		remove_wire(wid, false);
	}
	//assert(error_check());
}



void circuit::propagate_constants() {
	id2boolHmap consts_map;
	propagate_constants(consts_map);
}

// constant propagation
void circuit::propagate_constants(id2boolHmap consts_map) {

	D1(
	circuit tie_cir = *this;
	auto consts_map_copy = consts_map;
	)

	assert(error_check());

	tie_to_constant(consts_map);
	consts_map.clear();

	add_const_to_map(consts_map);

	idset cgates;
	for (auto& pr : consts_map) {
		utl::add_all(cgates, getwire(pr.first).fanouts);
	}

	while (!cgates.empty()) {
		id gid = *cgates.begin();
		cgates.erase(gid);
		if (!gate_exists(gid))
			continue;

		// simplify gate once and update const_map and Q
		auto g = getcgate(gid);

		int fixed_output = -1;

		switch (g.gfun()) {
		case fnct::NOR:
		case fnct::OR:
		case fnct::NAND:
		case fnct::AND: {
			bool is_zero_dom = g.gfun() == fnct::AND  || g.gfun() == fnct::NAND;
			bool is_negative = g.gfun() == fnct::NAND || g.gfun() == fnct::NOR;
			for (auto fanin : g.fanins) {
				if (_is_in(fanin, consts_map)) {
					if (consts_map.at(fanin) == !is_zero_dom) {
						fixed_output = is_negative ^ !is_zero_dom;
						break;
					}
					else if (consts_map.at(fanin) == is_zero_dom) {
						remove_gatefanin(gid, fanin);
						if (gfanin(gid).empty()) {
							fixed_output = is_negative ^ is_zero_dom;
							break;
						}
					}
				}
			}
			break;
		}

		case fnct::XOR:
		case fnct::XNOR: {
			int present = 0; // 0: none, 1:first input, 2:second input, 3:both
			bool val0, val1;
			if (_is_in(g.fanins[0], consts_map)) {
				if (_is_in(g.fanins[1], consts_map)) {
					present = 3;
					val0 = consts_map.at(g.fanins[0]);
					val1 = consts_map.at(g.fanins[1]);
				}
				else {
					present = 1;
					val0 = consts_map.at(g.fanins[0]);
				}
			}
			else if (_is_in(g.fanins[1], consts_map)) {
				present = 2;
				val1 = consts_map.at(g.fanins[1]);
			}

			if (present == 3) {
				if (g.gfun() == fnct::XOR)
					fixed_output = val0 ^ val1;
				else
					fixed_output = !(val0 ^ val1);
			}
			else if (present == 1 || present == 2) {
				remove_gatefanin(gid, g.fanins.at(present - 1));
				bool known_val = (present == 2) ? val1 : val0;
				setgatefun(gid, ((g.gfun() == fnct::XOR) ^ known_val) ? fnct::BUF : fnct::NOT);
			}
			break;
		}

		case fnct::BUF: {
			if (_is_not_in(g.fanins[0], consts_map)) {
				neos_error("problem at " << wname(g.fanins[0]) << " " << wname(g.fanouts[0]) );
			}
			fixed_output = consts_map.at(g.fanins[0]);
			break;
		}
		case fnct::NOT: {
			if (_is_not_in(g.fanins[0], consts_map)) {
				neos_error("problem at " << wname(g.fanins[0]) << " " << wname(g.fanouts[0]) );
			}
			fixed_output = !consts_map.at(g.fanins[0]);
			break;
		}
		case fnct::MUX: {
			auto it0 = consts_map.find(g.fanins[0]);
			auto it1 = consts_map.find(g.fanins[1]);
			auto it2 = consts_map.find(g.fanins[2]);

			if (it1 != consts_map.end() && it2 != consts_map.end()) {
				if (it1->second == it2->second) {
					fixed_output = it1->second;
				}
				/*else {
					remove_gatefanin(gid, g.fanins[1]);
					remove_gatefanin(gid, g.fanins[2]);
					setgatefun(gid, it1->second ? fnct::NOT : fnct::BUF);
				}*/
			}
			else if (it0 != consts_map.end()) {
				remove_gatefanin(gid, g.fanins.at(0));
				remove_gatefanin(gid, g.fanins.at(consts_map.at(g.fanins[0]) ? 1:2));
				setgatefun(gid, fnct::BUF);
			}
			break;
		}
		default: {
			neos_error("unsuported gate " << funct::enum_to_string(g.gfun()));
		}

		}

		if (fixed_output != -1) {
			id gout = g.fo0();
			//std::cout << "wire " << wname(gout) << " became fixed to " << fixed_output << "\n";
			for (auto gid : wfanout(gout)) {
				//std::cout << "gate " << wname(getcgate(gid).fo0()) << " is constant\n";
			}
			utl::add_all(cgates, wfanout(gout));
			remove_gate(gid);
			tie_to_constant(gout, fixed_output);
			add_const_to_map(consts_map);
		}

	}

	remove_dead_logic(); // remove logic which drives no output

	D1(
	tie_to_constant(consts_map_copy);


	for (auto in : tie_cir.inputs()) {
		if (find_wire(tie_cir.wname(in)) == -1) {
			add_wire(tie_cir.wname(in), wtype::IN);
		}
	}

	assert(ext::check_equivalence_abc(*this, tie_cir));
	)
	// remove_bufs();
}

void circuit::simplify_gate(const gate& g, gate& ng,
		const id2boolHmap& consts_map, int& output_val, bool& dead_gate) {

	dead_gate = false;
	output_val = -1;
	ng = g;

	switch (g.gfun()) {
	case fnct::NAND:
	case fnct::AND: {
		for (auto fanin : g.fanins) {
			if (_is_in(fanin, consts_map)) {
				if (consts_map.at(fanin) == 0) {
					output_val = (g.gfun() == fnct::AND) ? 0 : 1;
					dead_gate = true;
					break;
				}
				else if (consts_map.at(fanin) == 1) {
					ng.removefanin(fanin);
					if (ng.fanins.empty()) {
						output_val = (g.gfun() == fnct::AND) ? 1 : 0;
						dead_gate = true;
						break;
					}
				}
			}
		}
		break;
	}
	case fnct::NOR:
	case fnct::OR: {
		for (auto fanin : g.fanins) {
			if (_is_in(fanin, consts_map)) {
				if (consts_map.at(fanin) == 1 ) {
					output_val = (g.gfun() == fnct::OR) ? 1 : 0;
					dead_gate = true;
					break;
				}
				else if (consts_map.at(fanin) == 0) {
					ng.removefanin(fanin);
					if (ng.fanins.empty()) {
						output_val = (g.gfun() == fnct::OR) ? 0 : 1;
						dead_gate = true;
						break;
					}
				}
			}
		}
		break;
	}

	case fnct::XOR:
	case fnct::XNOR: {
		int present = 0; // 0: none, 1:first input, 2:second input, 3:both
		bool val0, val1;
		if (_is_in(g.fanins.at(0), consts_map)) {
			if (_is_in(g.fanins.at(1), consts_map)) {
				present = 3;
				val0 = consts_map.at(g.fanins[0]);
				val1 = consts_map.at(g.fanins[1]);
			}
			else {
				present = 1;
				val0 = consts_map.at(g.fanins[0]);
			}
		}
		else if (_is_in(g.fanins.at(1), consts_map)) {
			present = 2;
			val1 = consts_map.at(g.fanins.at(1));
		}

		if (present == 3) {
			if (g.gfun() == fnct::XOR)
				output_val = val0 ^ val1;
			else
				output_val = !(val0 ^ val1);

			dead_gate = true;
		}
		else if (present != 0) {
			ng.removefanin(g.fanins.at(present - 1));
			bool known_val = (present == 2) ? val1 : val0;
			ng.setgfun(((g.gfun() == fnct::XOR) ^ known_val) ? fnct::BUF : fnct::NOT);
		}
		break;
	}

	case fnct::BUF: {
		if (_is_in(g.fanins.at(0), consts_map)) {
			output_val = consts_map.at(g.fanins.at(0));
			dead_gate = true;
		}
		break;
	}
	case fnct::NOT: {
		if (_is_in(g.fanins.at(0), consts_map)) {
			output_val = !consts_map.at(g.fanins.at(0));
			dead_gate = true;
		}
		break;
	}
	case fnct::MUX: {
		if (_is_in(g.fanins.at(0), consts_map)) {
			ng.removefanin(g.fanins.at(0));
			ng.removefanin(g.fanins.at(consts_map.at(g.fanins[0]) ? 1 : 2));
			ng.setgfun(fnct::BUF);
		}
		break;
	}
	default: {
		neos_error("unsuported gate " << funct::enum_to_string(g.gfun()));
	}

	}

}


/*
 *  perform rewrite rules for simplification
 */
void circuit::rewrite_gates() {

	const idvec& gate_order = get_topsort_gates();

	for (auto gid : gate_order) {
		fnct fun = gfunction(gid);
		auto& fanins = gfanin(gid);
		// id fanout = ckt.gfanout(gid);
		if (fun != fnct::NOT && fun != fnct::BUF && fanins.size() == 1) {
			switch (fun) {
			case fnct::NAND:
			case fnct::NOR: setgatefun(gid, fnct::NOT); break;
			case fnct::AND:
			case fnct::OR: setgatefun(gid, fnct::BUF); break;
			default:
				break;
			}
		}
	}

	remove_bufs();
}

// break down gates with large fanins to smaller ones
void circuit::limit_fanin_size(int max_sz) {
	assert(max_sz >= 2);
	idset all_gates;
	foreach_gate(*this, g, gid) {
		all_gates.insert(gid);
	}
	for (auto gid : all_gates) {
		auto g = getgate(gid);
		if (g.fanins.size() > max_sz) {
			switch (g.gfun()) {
			case fnct::AND:
			case fnct::NAND:
			case fnct::OR:
			case fnct::NOR: {
				idque Q;
				utl::push_all(Q, g.fanins);
				while (Q.size() > 1) {

					idvec fanins;
					id fanout;
					uint qs = Q.size();
					for (uint i = 0; i < MIN(qs, max_sz); i++) {
						id fin = Q.front();
						Q.pop();
						fanins.push_back(fin);
					}
					fanout = (Q.empty() ? g.gfanout() : add_wire(wtype::INTER));

					bool is_inverted = (g.gfun() == fnct::NAND || g.gfun() == fnct::NOR);
					fnct inverse_fun = g.gfun();
					if (g.gfun() == fnct::NAND) {
						inverse_fun = fnct::AND;
					}
					else if (g.gfun() == fnct::NOR) {
						inverse_fun = fnct::OR;
					}

					id gid = add_gate(fanins, fanout,  !is_inverted ? g.gfun() : Q.empty() ? g.gfun() : inverse_fun);
					//print_gate(gid);

					if (!Q.empty()) Q.push(fanout);
				}

				remove_gate(gid);

				break;
			}
			default:
				break;
			}
		}
	}
}

}
