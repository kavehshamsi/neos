#include "enc.h"

namespace enc {


bool encrypt::enc_tbuflock(circuit& ckt, id num_bufs, int method) {

	// double cost = std::numeric_limits<double>::max();

	std::map<id, idvec> muxins;

	std::set<idpair> edge_set;

	// ckt.write_bench();

	// random insertion
	if (method == 0) {
		id a_wid, b_gid;
		while(edge_set.size() < (uint)num_bufs) {
			a_wid = ckt.get_random_wireid();
			b_gid = ckt.get_random_gateid();

			if (_is_not_in(a_wid, ckt.getcgate(b_gid).fanins)
				&& (a_wid != ckt.getcgate(b_gid).fo0())
				&& _is_not_in(idpair(a_wid, b_gid), edge_set) ) {
				edge_set.insert(idpair(a_wid, b_gid));
			}
		}
	}
	// cost function insertion
	else
		_wire_enc_optimization(ckt, num_bufs, edge_set);

	for (auto& edge : edge_set) {
		/*std::cout << "edge pair: " <<
				ckt.getwire_name(edge.first)
				<< "  " << ckt.getgate_name(edge.second) << "\n";*/
		id b_wid = ckt.getcgate(edge.second).fo0();
		muxins[b_wid].push_back(edge.first);
	}

	/*
	for (auto& x : muxins) {
		std::cout << "muxout: " << ckt.getwire_name(x.first) << "\n";
		for (auto& y : x.second) {
			std::cout << "  " << ckt.getwire_name(y) << "\n";
		}
	}
	std::cout << "\n\n\n\n";
	 */

	enc_wire::_insert_muxtrees(ckt, muxins, "tbuf");

	return true;
}


void encrypt::_wire_enc_optimization(const circuit& ckt,
		id num_buf, std::set<idpair>& edge_set) {


	cktgph::cktgraph cktgp_1(ckt);
	cktgp_1.writeVizFile("dump/vizfile1.dot");

	edge_set.clear();

	wire_enc_score cur_score;
	_compute_score(ckt, cur_score);
	wire_enc_score new_score = cur_score;

	circuit step_ckt = ckt;

	uint added = 0;
	while(added < num_buf) {

		uint NTRIAL = ckt.nWires();
		uint trial = 0;
		id a_wid, b_gid;
		idpair best_edge;
		do {
			while(true) {
				a_wid = ckt.get_random_wireid();
				b_gid = ckt.get_random_gateid();

				if (_is_not_in(a_wid, step_ckt.getcgate(b_gid).fanins)
					&& (a_wid != step_ckt.getcgate(b_gid).fo0())
					&& _is_not_in(idpair(a_wid, b_gid), edge_set) )
					break;
			}

			// modify circuit
			step_ckt.addWire_to_gateFanin(b_gid, a_wid);

			_compute_score(step_ckt, new_score);
			// std::cout << "possible score : " << new_score << "\n";

			if (new_score.total > cur_score.total) {
				cur_score = new_score;
				best_edge = idpair(a_wid, b_gid);
			}

			// restore circuit
			step_ckt.remove_gatefanin(b_gid, a_wid);

		} while (trial++ < NTRIAL);

		std::cout << "score : " << cur_score.total << "\n";

		// apply best pair
		step_ckt.addWire_to_gateFanin(best_edge.second, best_edge.first);

		edge_set.insert(best_edge);
		added++;
	}

	std::cout << "\nfinal score : \n";
	cur_score.print_score();


	//step_ckt.write_bench();

	cktgp_1.writeVizFile_wcolorEdges("dump/vizfile2.dot", edge_set);

	// tmp_ckt.write_bench();

}

void encrypt::_compute_score(const circuit& tmp_ckt, wire_enc_score& scr) {

	// feedback arc set count
	id2idHmap feedback_arc_set = tmp_ckt.find_feedback_arc_set();

	scr.feedback_arc_size = feedback_arc_set.size();

	// scc size
	std::vector<idset> sccs;
	tmp_ckt.find_sccs(sccs);

	scr.scc_size = sccs.size();

	// scc density
	scr.max_scc_density = _max_scc_density(tmp_ckt, sccs);

	scr.compute_total();
}

/*
 * get key-mux/buf dependency graph then
 * in the dependency graph self-edges are removable
 */
void encrypt::mux_removal_attack(const circuit& ckt) {


	// map mux cone root to its inputs
	std::map<id, idvec> muxgid2inputs;


	foreach_gate(ckt, g, gid) {
		if (_is_root_key_mux(gid, ckt)) {
			idvec muxins;
			_get_mux_cone_inputs(gid, ckt, muxins);
			muxgid2inputs[gid] = muxins;
		}
	}

	uint num_removable_edges = 0;

	for (auto& out : ckt.outputs()) {

		boolvec visited(ckt.nWires(), false);
		boolvec ontrace(ckt.nWires(), false);

		idvec trace;

		trace.push_back(out);
		id cur_w = out;
		ontrace[cur_w] = true;
		visited[cur_w] = true;

		while (true) {
			bool found = false;
			/*for (auto& x : trace) {
				std::cout << wname(ckt, x) << " ";
			}
			std::cout << "\n";*/
			for (auto gid : ckt.wfanins(cur_w)) {
				auto& g = ckt.getcgate(gid);
				if (_is_in(gid, muxgid2inputs)) {
					std::cout << "hit mux \n";
					for (auto& muxin : muxgid2inputs.at(gid)) {
						//std::cout << " " << wname(ckt, muxin) << " ";
						if (ontrace[muxin]) {
							//std::cout << "\n" << wname(ckt, muxin) << " is on trace\n";
							num_removable_edges++;
							erase_byval(muxgid2inputs.at(gid), muxin);
						}
					}
				}
				else {
					for (auto& wid : g.fanins) {
						if (!visited[wid]) {
							found = true;
							cur_w = wid;
							visited[cur_w] = true;
							ontrace[cur_w] = true;
							trace.push_back(cur_w);
							break;
						}
					}
					if (found) break;
				}
			}

			if (!found) {
				trace.pop_back();
				ontrace[cur_w] = false;
				if (trace.empty()) {
					break;
				}
				else {
					cur_w = trace.back();
				}
			}
		}
	}

	std::cout << "num removable edges: " << num_removable_edges << "\n";
}

void encrypt::_wire_output_dependency(const circuit& ckt,
		std::vector<idset>& id2reachable) {

	// map that links the dependency end to its source mux
	id2reachable = std::vector<idset>(ckt.nWires(), idset());

	for (auto& x : ckt.outputs()) {
		id2reachable[x].insert(x);

		idvec stack;
		stack.push_back(x);
		boolvec visited(ckt.nWires(), false);

		id cur_w;
		while(!stack.empty()) {
			cur_w = stack.back();
			stack.pop_back();
			visited[cur_w] = true;

			for ( auto gid : ckt.wfanins(cur_w)) {
				const auto& g = ckt.getcgate(gid);
				if (!_is_key_mux(gid, ckt)) {
					for (auto& in : g.fanins) {
						if (!visited[in]) {
							add_all(id2reachable[in], id2reachable[cur_w]);
							stack.push_back(in);
						}
					}
				}
			}
		}
	}

	foreach_vecindex(id2reachable, x) {
		std::cout << "\nwire " << ckt.wname(x) << ":\n{";
		for (auto& y : id2reachable[x]) {
			std::cout << ckt.wname(y) << ", ";
		}
		std::cout << "}\n";
	}

}

bool encrypt::_is_root_key_mux(id gid, const circuit& ckt) {
	if (_is_key_mux(gid, ckt)) {
		auto& fanout_gates = ckt.gfanoutGates(gid);
		if (fanout_gates.size() == 1 &&
				_is_key_mux(*fanout_gates.begin(), ckt))
			return false;
		else
			return true;
	}
	return false;
}

void encrypt::_get_mux_cone_inputs(id gid, const circuit& ckt,
		idvec& muxins_vec) {

	auto& root_mux = ckt.getcgate(gid);
	idvec stack;
	idset muxins(root_mux.fanins.begin()+1, root_mux.fanins.end());
	push_all(stack, muxins);

	std::cout << "mux rooted at :" << ckt.wname(root_mux.fo0()) << "\n";

	id cur_w;
	while(!stack.empty()) {
		cur_w = stack.back();
		stack.pop_back();
		muxins.insert(cur_w);

		for (auto& x : ckt.wfanins(cur_w)) {
			if (_is_key_mux(x, ckt)) {
				muxins.erase(cur_w);
				stack.insert(stack.end(),
						ckt.getcgate(x).fanins.begin()+1,
						ckt.getcgate(x).fanins.end());
			}
		}
	}

	std::cout << "\n mux inputs:\n";
	for (auto& x : muxins) {
		std::cout << ckt.wname(x) << ", ";
	}
	std::cout << "\n";

	push_all(muxins_vec, muxins);
}


bool encrypt::_is_key_mux(id gid, const circuit& ckt) {
	return ( (ckt.gfunction(gid) == fnct::MUX) &&
			(ckt.wrtype(ckt.getcgate(gid).fanins[0]) == wtype::KEY) )
			|| (ckt.gfunction(gid) == fnct::TBUF);
}


double encrypt::_max_scc_density(const circuit& tmp_ckt,
		std::vector<idset>& sccs) {

	double max_density = 0;
	for (auto& scc : sccs) {
		if (scc.size() != 1) {

			uint num_edges = 0;
			for (auto& g : scc)
				num_edges += tmp_ckt.getcgate(g).fanins.size();

			double scc_density = num_edges / scc.size();
			max_density = MAX(scc_density, max_density);
		}
	}

	return max_density;
}

} // namespace enc
