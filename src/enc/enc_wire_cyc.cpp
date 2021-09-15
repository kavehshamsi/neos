#include "enc.h"
#include "stat/stat.h"

namespace enc {

// static member initialization

void encrypt::cyclify(circuit& ckt, int num_edges) {

	circuit ckt_copy = ckt;

	int added_edges = 0;
	idset candidate_gates;
	foreach_gate(ckt, g, gid) {

		// nand and cyclification using MUX
		if (g.gfun() == fnct::AND || g.gfun() == fnct::NAND) {
			candidate_gates.insert(gid);
		}
	}

	while (added_edges < num_edges) {
		if (candidate_gates.empty()) {
			std::cout << "not enough gates for cyclification goal\n";
			break;
		}
		id gid = utl::rand_from_set(candidate_gates);
		candidate_gates.erase(gid);

		const gate& g = ckt.getcgate(gid);

		idset fanout_wires;
		ckt.get_trans_fanoutset(g.fo0(), fanout_wires);
		if (fanout_wires.empty())
			continue;

		id feedback_wr = utl::rand_from_set(fanout_wires);

		gate cycmux;
		cycmux.fanins.push_back(feedback_wr);
		cycmux.fanins.push_back(g.fanins[0]);
		id or_out = ckt.add_wire(wtype::INTER);
		cycmux.fo0() = or_out;
		ckt.addWire_to_gateFanin(gid, or_out);
		cycmux.setgfun(fnct::OR);
		string gname = "cycgate_" + std::to_string(added_edges++);
		ckt.add_gate(gname, cycmux);
	}

	assert(dec_ns::check_equality_sat(ckt, ckt_copy));
}

boolvec encrypt::enc_loopy_wMUX (id width, id minlooplen, circuit& ckt) {

	const id CPU_LIMIT = 100;
	id CPU = 0;

	id count = 0, edge_count = 0;
	id orig_key_count = ckt.nKeys();

	while (count < width && CPU < CPU_LIMIT) {

		id ugate = ckt.get_random_gateid();
		assert(ckt.isGate(ugate));

		idvec pathwires;
		id vgate = _find_path(ugate, ckt, minlooplen, pathwires);
		if (vgate != -1) {
			_feed_back(ugate, vgate, ckt, edge_count);
			_make_path_removable(pathwires, ckt, edge_count);
			count++;
		}
		CPU++;
	}

	id new_key_count = ckt.nKeys();

	return boolvec(new_key_count - orig_key_count, 0);

}

bool encrypt::_feed_back_edgeonly (id gu, id gv, circuit& ckt) {
	ckt.getgate(gu).addfanin(ckt.getgate(gv).fo0());
	return true;
}

bool encrypt::_feed_back(id gu, id gv, circuit& ckt, id& edge_count) {

	id u = ckt.gfanin0(gv);
	id v = ckt.gfanout(gu);
	insert_2muxgate(u, v, ckt, edge_count++);

	/*

	functID kgateFunct;
	switch (ckt.getgates()[gu].functionality) {

	case wtype::OR:
	case NOR:
		kgateFunct = fnct::AND;
		break;
	case fnct::AND:
	case NAND:
		kgateFunct = OR;
		break;
	default:
		return false;
	}

	id extended_in = _inc_input_width(gu, ckt);
	id key_in = ckt.new_key_wire();
	idvec gatefanins;
	gatefanins.push_back(key_in);
	gatefanins.push_back(ckt.getgates()[gv].fanout);

	name gatename = "fbgate_" + std::to_string(ckt.getnumber_of_gates());
	id gateid = ckt.add_gate(
			gatename,
			gatefanins,
			extended_in,
			kgateFunct);
	*/
	return true;
}

id encrypt::_inc_input_width(id gu, circuit& ckt) {
	id wr = ckt.add_wire(
			"ext" + std::to_string(ckt.nGates()), wtype::INTER);
	ckt.getwire(wr).addfanout(gu);
	ckt.getgate(gu).fanins.push_back(wr);
	return wr;
}

id encrypt::_find_path (id u, circuit& ckt, uint len, idvec& wirepath) {

	std::map<id, uint> lenmap;
	std::map<id, id> parmap;
	uint l = 0;
	id v = u;

	idstack s;
	s.push(v);

	idset visited;
	idvec gatepath;

	lenmap[v] = 1;

	while (l < len && !s.empty()) {

		bool found = false;

		v = s.top();
		l = lenmap[v];

		for (auto& x : ckt.gfanoutGates(v)) {
			bool key_gate = false;
			for ( auto gf : ckt.gfanin(x) ) {
				if (ckt.isKey(gf)) {
					key_gate = true;
				}
			}
			if (_is_not_in(x, visited) && !key_gate) {
				parmap[x] = v;
				lenmap[x] = l+1;
				s.push(x);
				found = true;
			}
			visited.insert(x);
		}

		if (!found) {
			s.pop();
			l = lenmap[v];
		}
	}

	if (l < len) {
		return -1;
	}
	else {
		id par = v;
		gatepath.push_back(par);
		do {
			par = parmap[par];
			gatepath.push_back(par);
		} while (par != u);

		std::cout << "\npath: {";
		for (auto x : gatepath) {
			std::cout <<
				ckt.wname(ckt.gfanout(x)) << ", ";
		}
		std::cout << " }\n";

		for (auto& x : gatepath) {
			wirepath.push_back(ckt.gfanout(x));
		}

		return v;
	}

}

void encrypt::_make_path_removable(idvec& pathwires,
		circuit& ckt, id& edge_count) {

	id num_wires = ckt.nWires();
	for (auto& x : pathwires) {
		id r = 0; bool singlefanout = false;
		if (ckt.wfanout(x).size() == 1)
			singlefanout = true;

		r = ckt.get_random_wireid(wtype::INTER);
		insert_2muxgate(x, r, ckt, edge_count++);

		if (singlefanout) {
			r = ckt.get_random_wireid(wtype::INTER);
			insert_2muxgate(r, x, ckt, edge_count++);
		}
	}
}

void encrypt::insert_2mux(id wr, id otherend, circuit& ckt, id namestamp) {

	id2idmap added2new;

	ckt_block::mux_t mux = ckt_block::nmux(2, 1);

	// open wire
	id muxout = ckt.open_wire(wr).second;

	// mux output
	added2new[mux.out] = muxout;

	// mux input
	added2new[mux.in_vec[0]] = wr;
	added2new[mux.in_vec[1]] = otherend;

	//add first mux
	ckt.add_circuit(mux, added2new, std::to_string(namestamp));

}


//#define CAMOU // for P&R camouflaging or key-based locking
void encrypt::insert_2muxgate(id wr, id otherend, circuit& ckt, id namestamp) {

	// open wire
	auto pr = ckt.open_wire(wr);
	id muxout = pr.second;
	id muxin = pr.first;
	idvec muxins;


#ifdef CAMOU
	string gatename0 = "cam0via" + std::to_string(namestamp);
	string gatename1 = "cam1via" + std::to_string(namestamp);

	muxins.push_back(wr);
	ckt.add_gate(gatename0, muxins, muxout, fnct::CVIA);

	muxins.clear();
	muxins.push_back(otherend);
	ckt.add_gate(gatename1, muxins, muxout, fnct::CVIA);

#else
	string gatename = "cam_mux" + std::to_string(namestamp);

	// create key for logic locking
	id key = ckt.add_key();

	muxins.push_back(key);
	muxins.push_back(muxin);
	muxins.push_back(otherend);

	// add first mux
	ckt.add_gate(gatename, muxins, muxout, fnct::MUX);
#endif

}

bool encrypt::enc_interconnect(circuit& cir, enc_wire::enc_wire_options& opt) {

	id original_n_wires = cir.nWires();
	assert(original_n_wires > opt.num_muxs);
	assert(original_n_wires > opt.mux_size);
	assert(opt.mux_size >= opt.num_muxs);

	enc_wire ew(cir, opt);

	if (opt.method == "crlk")
		ew._pick_crosslock_wires();
	else if (opt.method == "mmux")
		ew._pick_manymux_wires();
	else
		neos_error("unknwon method " << opt.method << "\n");

	enc_wire::_insert_muxtrees(cir, ew.muxtrees, opt.mux_cell_type);

	return true;
}

void enc_wire::enc_wire_options::print_options() {

	std::cout << "num muxes: " << num_muxs << "\n";
	std::cout << "mux size: " << mux_size << "\n";
	std::cout << "mux cell: " << mux_cell_type << "\n";
	std::cout << "num units: " << num_cbs << "\n";
	std::cout << "locking scheme: " << method << "\n";
	std::cout << "pick strategy: " << selection_method << "\n";

}

int enc_wire::enc_wire_options::parse_command_line(
		const std::vector<std::string>& pos_args) {

	if (pos_args.size() < 1) {
		neos_error("usage: neos -e int <input_file> <output_file> method [args]");
	}
	method = pos_args[0];
	if (method == "crlk") {
		if (pos_args.size() != 7) {
			neos_error("usage: neos -e int <input_file> <output_file> crlk [num_cbs] [num_muxs] [mux_size] [selection_method] [keep_acyclic] [cell]");
		}
		num_cbs = stoi(pos_args[1]);
		num_muxs = stoi(pos_args[2]);
		mux_size = stoi(pos_args[3]);
		selection_method = pos_args[4];
		keep_acyclic = (bool)stoi(pos_args[5]);
		mux_cell_type = pos_args[6];
	}
	else if (method == "mmux") {
		if (pos_args.size() != 7) {
			neos_error("usage: neos -e int <input_file> <output_file> mmux [num_cbs] [num_muxs] [mux_size] [selection_method] [keep_acyclic] [cell]");
		}
		num_cbs = stoi(pos_args[1]);
		num_muxs = stoi(pos_args[2]);
		mux_size = stoi(pos_args[3]);
		selection_method = pos_args[4];
		keep_acyclic = (bool)stoi(pos_args[5]);
		mux_cell_type = pos_args[6];
	}
	else if (method == "cg") {
		if (pos_args.size() != 2) {
			neos_error("usage: neos -e int <input_file> <output_file> cg [intensity] [keep_acyclic=0]");
		}
		intensity = stof(pos_args[1]);
	}

	return 0;

}

// opens wires and inserts muxtrees
void enc_wire::_insert_muxtrees(circuit& ckt, std::map<id, idvec>& mux_maps,
		const std::string& mux_cell_type) {

	// open wires first

	idvec muxouts;
	for (auto& p : mux_maps)
		muxouts.push_back(p.first);

	for (id old_muxout : muxouts) {
		auto open_pair = ckt.open_wire(old_muxout);
		mux_maps[open_pair.second] = mux_maps.at(old_muxout);
		if (open_pair.second != old_muxout)
			mux_maps.erase(old_muxout);
		mux_maps[open_pair.second][0] = open_pair.first;
	}

	if (mux_cell_type == "mux" || mux_cell_type == "tbuf" || mux_cell_type == "nmux") {

		uint count = 0;
		for (auto mux_map : mux_maps) {

			id muxsize = mux_map.second.size();
			std::cout << "mux size: " << muxsize << "\n";
			ckt_block::mux_t mymux;
			if (mux_cell_type == "tbuf") {
				mymux = ckt_block::key_tmux(muxsize);
			}
			else if (mux_cell_type == "nmux") {
				mymux = ckt_block::nmux(muxsize);
			}
			else {
				mymux = ckt_block::key_nmux_w2mux(muxsize);
			}

			id2idmap added2new;

			for (auto sel : mymux.sel_vec) {
				added2new[sel] = ckt.add_key();
			}

			// mux output
			added2new[mymux.out] = mux_map.first;

			// the rest are picked from the muxin-wires set
			uint i = 0;
			for (auto muxin : mux_map.second) {
				// mux input
				added2new[mymux.in_vec[i++]] = muxin;
			}

			//add mux
			ckt.add_circuit(mymux, added2new, std::to_string(count++));
		}
	}
	else {
		int i = 0;
		for (auto mux_map : mux_maps) {
			// std::cout << "muxout : " << ckt.wname(mux_map.first) << ":";
			for (auto muxin : mux_map.second) {
				// std::cout << "	muxin: " << ckt.wname(muxin) << "\n";
				idvec ports = {muxin, mux_map.first};
				vector<string> cell_pins = {"A", "B"};
				ckt.add_instance("pvia" + std::to_string(i++), mux_cell_type, ports, cell_pins);
			}
		}
	}
}

void enc_wire::_get_all_wires_vec() {
	all_wires_vec.reserve(ckt.nWires());
	foreach_wire(ckt, w, wid)
		all_wires_vec.push_back(wid);
}

/*
 * implements wire selection strategies for wire_obfuscation
 */
void enc_wire::_pick_crosslock_wires() {

	// vector of all wires is useful
	_get_all_wires_vec();
	candidate_wire_sets = std::vector<idset>(opt.num_cbs, idset());

	// random selection
	if (opt.selection_method == "rnd") {
		// all wires are legal. this is quite inefficient
		for (uint i = 0; i < opt.num_cbs; i++)
			foreach_wire(ckt, w, wid)
				candidate_wire_sets[i].insert(wid);
	}

	// diagonal expansion from random gate
	else if (opt.selection_method == "wrad") {
		_get_candidate_wires_diag();
	}

	// k-cut selection
	else if (opt.selection_method == "kcut") {
		_get_candidate_wires_kcut();
	}

	// wirecone selection
	else if (opt.selection_method == "wcut") {
		_get_candidate_wires_wirecone();
	}

	else {
		neos_error("error: invalid wire selection option " << opt.selection_method << "\n");
	}

	_muxtrees_from_candidates_crosslock();
}


void enc_wire::_muxtrees_from_candidates_crosslock() {


	for (uint cb_count = 0; cb_count < opt.num_cbs; cb_count++) {

		// assert(candidate_wire_sets[cb_count].size() > opt.num_muxs + opt.mux_size);

		int num_trial = 1000;
		int trial = 0;
		while (trial++ < num_trial) {

			muxtrees.clear();

			// pick random muxouts
			idset muxouts = utl::rand_from_set(candidate_wire_sets[cb_count], opt.num_muxs);

			// get union of fanoutsets
			idset muxout_fanouts;
			for (auto muxout : muxouts) {
				idset fanoutset;
				ckt.get_trans_fanoutset(muxout, fanoutset);
				add_all(muxout_fanouts, fanoutset);
				muxtrees[muxout].push_back(muxout);
			}

			// search for a set of muxins
			uint found_x = 0;
			for_rand_set_traversal(x, candidate_wire_sets[cb_count], id) {
				if (!opt.keep_acyclic || _is_not_in(x, muxout_fanouts)) {
					for (auto muxout : muxouts) {
						muxtrees[muxout].push_back(x);
					}
					found_x++;
				}
				if (found_x == opt.mux_size) {
					break;
				}
			}

			if (found_x == opt.mux_size)
				break;
		}

		if (trial == num_trial) {
			neos_error("could not find enough wires in candidate set for mux-insertion.");
		}
	}
}


/*
 * uses BFS to find distance of all wires from current wire
 * also sorts them according to distance
 */
void enc_wire::_find_depthmap(id root, const circuit& ckt,
		std::map<id, int>& depth_map, idvec& sorted_wires) {

	// assumes no removed wires
	boolvec visited(ckt.nWires(), false);

	id cur_wid = root;

	// BFS queue
	idque cur_wque;

	cur_wque.push(cur_wid);
	int depth = 1;
	depth_map[cur_wid] = depth;

	while (!cur_wque.empty()) {
		for (id wid : ckt.wfanoutWires(cur_wid)) {
			if (!visited[wid]) {
				depth_map[wid] = depth_map[cur_wid] + 1;
				sorted_wires.push_back(wid);
				cur_wque.push(wid);
				visited[wid] = true;
			}
		}
		for (id wid : ckt.wfaninWires(cur_wid)) {
			if (!visited[wid]) {
				depth_map[wid] = depth_map[cur_wid] + 1;
				sorted_wires.push_back(wid);
				cur_wque.push(wid);
				visited[wid] = true;
			}
		}
		cur_wid = cur_wque.front();
		cur_wque.pop();
	}
}

/*
 * many mux encryption based on inserting a bunch of multiplexers
 */
boolvec encrypt::enc_manymux(circuit& ckt, enc_wire::enc_wire_options& opt) {


	id original_n_wires = ckt.nWires();
	assert(original_n_wires > opt.mux_size);

	id orig_n_keys = ckt.nKeys();

	enc_wire ew(ckt, opt);

	ew._pick_manymux_wires();

	enc_wire::_insert_muxtrees(ckt, ew.muxtrees, opt.mux_cell_type);

	id new_n_keys = ckt.nKeys();

	return boolvec(new_n_keys - orig_n_keys, 0);
}

void enc_wire::_pick_manymux_wires() {

	_get_all_wires_vec();
	candidate_wire_sets = std::vector<idset>(opt.num_cbs, idset());

	// random selection
	if (opt.selection_method == "rnd") {
		// all wires are legal. this is quite inefficient
		for (uint i = 0; i < opt.num_cbs; i++)
			foreach_wire(ckt, w, wid)
				candidate_wire_sets[i].insert(wid);
		_muxtrees_from_candidates_mmux();
	}

	// diagonal expansion from random gate
	else if (opt.selection_method == "wrad") {
		_get_candidate_wires_diag();
		_muxtrees_from_candidates_mmux();
	}

	// k-cut selection
	else if (opt.selection_method == "kcut") {
		_get_candidate_wires_kcut();
		_muxtrees_from_candidates_mmux();
	}

	// wire cone selection
	else if (opt.selection_method == "wcut") {
		_get_candidate_wires_wirecone();
		_muxtrees_from_candidates_mmux();
	}

	// topsort maxcyclic
	else if (opt.selection_method == "tpsrt") {
		_topsort_mmux_pick();
	}

	// complete graph allow cyclic
	else if (opt.selection_method == "cg") {
		_cg_mmux_pick(opt.intensity);
	}

	else {
		std::cout << "error: invalid pick strategy!\n";
		exit(EXIT_FAILURE);
	}
}


void enc_wire::_muxtrees_from_candidates_mmux() {


	for (uint cb_count = 0; cb_count < opt.num_cbs; cb_count++) {

		// assert(candidate_wire_sets[cb_count].size() > opt.num_muxs + opt.mux_size);

		int num_trial = 1000;
		int trial = 0;
		while (trial++ < num_trial) {

			muxtrees.clear();

			// pick random muxouts
			idset muxouts = utl::rand_from_set(candidate_wire_sets[cb_count], opt.num_muxs);

			// get union of fanoutsets
			idset muxout_fanouts;
			for (auto muxout : muxouts) {
				idset fanoutset;
				ckt.get_trans_fanoutset(muxout, fanoutset);
				add_all(muxout_fanouts, fanoutset);
				muxtrees[muxout].push_back(muxout);
			}

			// search for a set of muxins
			uint found_x = 0, found_y = 0;
			for (auto muxout : muxouts) {
				found_x = 0;
				for_rand_set_traversal(x, candidate_wire_sets[cb_count], id) {
					if (x != muxout && (!opt.keep_acyclic || _is_not_in(x, muxout_fanouts)) ) {
						muxtrees[muxout].push_back(x);
						found_x++;
					}
					if (found_x == opt.mux_size) {
						found_y++;
						break;
					}
				}
				if (found_y == opt.num_muxs) {
					break;
				}
			}


			if (found_y == opt.num_muxs) {
				break;
			}
		}

		if (trial == num_trial) {
			neos_error("could not find enough wires in candidate set for mux-insertion.");
		}

		for (auto& muxtree : muxtrees) {
			std::cout << "muxtree size: " << muxtree.second.size() << "\n";
		}
	}

}


void enc_wire::_cg_mmux_pick(float cgness) {

	assert (ckt.nInputs() < 10);

	circuit track_ckt = ckt;

	// complexity may be in O(num_wires^3) which is bad
	for_rand_vec_traversal(x, all_wires_vec, id) {
		idset trans_fanout_set;
		track_ckt.get_trans_fanoutset(x, trans_fanout_set);
		for_rand_vec_traversal(y, all_wires_vec, id) {
			if (
				(!opt.keep_acyclic || _is_not_in(y, trans_fanout_set)) &&
				y != x &&
				((rand() % 100) < (cgness * 100))) {

				wire& x_wr = track_ckt.getwire(x);
				wire& y_wr = track_ckt.getwire(y);
				for (auto x : x_wr.fanouts) {
					y_wr.addfanout(x);
				}

				muxtrees[x].push_back(y);
			}
		}
		if (muxtrees[x].size() == 1)
			muxtrees.erase(x);
	}

	for (auto x : muxtrees) {
		/*
		std::cout << "\nmuxout : " << ckt.getwire_name(x.first) << "\n";
		for (auto y : x.second) {
			std::cout << "  " << ckt.getwire_name(y) << "\n";
		}*/
		x.second.push_back(x.first);
	}
}

void enc_wire::_topsort_mmux_pick() {

	const idvec& gate_ordering = ckt.get_topsort_gates();

	uint x = (opt.keep_acyclic) ? (gate_ordering.size() - 1) : 0;

	if (opt.keep_acyclic) { // acyclic only
		do {
			idset fanout_set;
			id muxout = ckt.getcgate(gate_ordering[x--]).fo0();
			ckt.get_trans_fanoutset(muxout, fanout_set);
			uint y = 0;
			idvec muxins;
			do {
				id wr = ckt.getcgate(gate_ordering[y++]).fo0();
				if ( _is_not_in(wr, fanout_set) )
					muxins.push_back(wr);
			} while (y != gate_ordering.size() - 1 &&
					muxins.size() != opt.mux_size);
			if (y != gate_ordering.size()-1) {
				muxtrees[muxout] = muxins;
			}
		} while (x != 0 && muxtrees.size() != opt.num_muxs);
	}
	else { // mostly cyclic
		do {
			idset fanin_set;
			id muxout = ckt.getcgate(gate_ordering[x++]).fo0();
			ckt.get_trans_fanin(muxout, fanin_set);
			id y = gate_ordering.size() - 1;
			idvec muxins;
			do {
				id wr = ckt.getcgate(gate_ordering[y--]).fo0();
				if ( _is_not_in(wr, fanin_set) )
					muxins.push_back(wr);
			} while (y != -1 &&
					muxins.size() != opt.mux_size);
			if (y != -1) {
				muxtrees[muxout] = muxins;
			}
		} while ((x != gate_ordering.size() - 1)
				&& muxtrees.size() != opt.num_muxs);
	}

	if (x == 0 || x == gate_ordering.size()-1) {
		std::cout << "error: not enough wires in design!\n";
		exit(EXIT_FAILURE);
	}

}

void enc_wire::_build_transfanin_sizemap(const circuit& cir) {

	auto vis = faninwire_visitor(cir);

	foreach_visitor_elements(vis, cir.outputs(), x) {
		idset faninset;
		cir.get_trans_fanin(x, faninset);
		fanin_size_map[x] = faninset.size();
	}

	assert(fanin_size_map.size() == (uint)cir.nWires());

}

bool enc_wire::_check_wobf_feasibility(const idset& wrset) {

	if (opt.keep_acyclic) {

		// feasibility checking with keep_acyclic is tricky
		id met_constraint = 0;
		for (auto& x : wrset) {

			idset faninset;
			ckt.get_trans_fanin(x, faninset);

			id possible_muxin = 0;

			idvec muxins;
			for (auto& y : wrset) {
				if (y != x) {
					if (_is_in(y, faninset)) {
						if (++possible_muxin == opt.mux_size - 1) {
							met_constraint++;
							break;
						}
					}
				}
			}
			if (met_constraint == opt.num_muxs)
				return true;
		}
	}
	else {
		if (wrset.size() > opt.mux_size + opt.num_muxs)
			return true;
	}
	return false;
}

void enc_wire::_get_candidate_wires_wirecone() {

	uint i = 0;
	for_rand_vec_traversal(rootwr, all_wires_vec, id) {

		auto vis = faninwire_visitor(ckt);
		foreach_visitor_elements(vis, rootwr, wid) {

			candidate_wire_sets[i].insert(wid);

			if (_check_wobf_feasibility(candidate_wire_sets[i])) {
				i++;
				break;
			}
		}
		if (i == opt.num_cbs)
			break;
	}

	if (i < opt.num_cbs) {
		std::cout << "error: large enough wire-cone not found!\n";
		exit(EXIT_FAILURE);
	}
}

void enc_wire::_get_candidate_wires_diag() {

	bool success = false;

	uint i = 0;
	for_rand_vec_traversal(rootwr, all_wires_vec, id) {

		// get depth map strating from root
		idvec sorted_wires;
		std::map<id, int> depth_map;
		_find_depthmap(rootwr,
				ckt, depth_map, sorted_wires);

		// starting circle
		uint num_wires_in_circle = opt.mux_size;
		candidate_wire_sets[i].clear();

		while (true) {
			idvec circle_wires = utl::subvec(sorted_wires, 0,
					num_wires_in_circle - 1);

			add_all(candidate_wire_sets[i], sorted_wires);

			if (_check_wobf_feasibility(candidate_wire_sets[i])) {
				i++;
				break;
			}
			// grow circle by 20%
			num_wires_in_circle *= 1.2;
			if (num_wires_in_circle >= sorted_wires.size())
				break;
		}
		if (i == opt.num_cbs) {
			success = true;
			break;
		}
	}
	if (!success) {
		std::cout << "error: wire circle that satisfies conditions not found\n";
		exit(EXIT_FAILURE);
	}
}

void enc_wire::_get_candidate_wires_kcut() {

	// iteratively grows kcut size
	uint kcut_input_size = 7;
	uint i = 0;

	if (opt.num_cbs == 0)
		return;

	for (id kcut_input_size = 4;
			kcut_input_size < ckt.nInputs(); kcut_input_size++) {

		// scan entire circuit for such a cut
		for_rand_vec_traversal(rootwr, all_wires_vec, id) {

			idset kcut_gset, kcut_inputs;

			// std::cout << "root wire : " << wname(ckt, rootwr) << "\n";
			if ( ckt.get_kcut(rootwr, kcut_input_size, kcut_gset, kcut_inputs, candidate_wire_sets[i], false) ) {

				//std::cout << "candidate wire set size: "
				//	<< candidate_wire_sets[i].size() << "\n";

				if (_check_wobf_feasibility(candidate_wire_sets[i])) {

					std::cout << "\nkcut rooted at: " << ckt.wname(rootwr) << "\n";
					std::cout << "number of gates in kcut: " << kcut_gset.size() << "\n";
					std::cout << "number of wires in kcut: "
							<< candidate_wire_sets[i].size()<< "\n\n";

					i++;
					if (i == opt.num_cbs) {
						break;
					}
				}
			}
		}

		if(i == opt.num_cbs)
			break;
	}

	if (kcut_input_size >= ckt.nInputs() || i != opt.num_cbs) {
		std::cout << "error: kcut with sufficient wires not found\n";
		exit(EXIT_FAILURE);
	}

}




} // namespace enc
