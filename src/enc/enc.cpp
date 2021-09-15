#include "enc.h"
#include "stat/stat.h"

//////////////// The circuit class ////////////////////
namespace enc {

boolvec encrypt::enc_lut(uint num_luts, uint lut_width, circuit& cir) {

	// initialization
	std::srand((uint)std::clock());

	// for k-cut detection
	circuit cut;
	idset kcut;
	idset fanins;

	boolvec corrkey;

	/*
	// get k-cuts
	Hmap<id, vector<circuit::cir_cut_t> > cuts;
	cir.enumerate_cuts(cuts, lut_width, true);
 	 */

	// get LUT of correct size;
	ckt_block::tree lut = ckt_block::lut(lut_width);

	//std::cout << lut.in_vec.size() << "\n";

	idvec candvec;

	foreach_wire(cir, w, wid) {
		if (w.isInter())
			candvec.push_back(wid);
	}

	// keep track of visited wires
	idset visited;
	uint added_luts = 0;

	for_rand_vec_traversal(rid, candvec, id) {

		if ( !cir.wire_exists(rid) || !cir.isInter(rid) || _is_in(cir.wfanin0(rid), visited)  ) {
			continue;
		}

		if (verbose) std::cout << "\n r was : " << rid << "\n";

		if ( cir.get_kcut(rid, lut_width, kcut, fanins, true) ) {

			//connection maps
			id2idmap added2new;

			// connections
			int i = 0;
			assert(fanins.size() == lut.in_vec.size());
			for (auto xid : fanins) {
				added2new[lut.in_vec[i++]] = xid;
			}

			for (auto gid : kcut) {
				visited.insert(gid);
			}

			// remove k-cut
			cir.remove_kcut(kcut, fanins);

			//insert LUT
			added2new[lut.out] = rid;

			cir.add_circuit(lut, added2new, std::to_string(added_luts));
			added_luts++;
		}

		if (added_luts == num_luts) {
			return corrkey;
		}

	}

	printred("failed to find enought k-cuts.\n");

	return corrkey;
}

boolvec encrypt::enc_shfk(circuit& ckt) {
	return enc_swbox(ckt, ckt.keys());
}

boolvec encrypt::enc_swbox(circuit& ckt, const oidset& shuffle_set) {

	id2idmap added2new;

	ckt_block::swbox nswb =
			ckt_block::n_switchbox(shuffle_set.size());

	for (auto& x : nswb.sel_vec){
		std::cout << nswb.wname(x) << "\n";
		string kname = std::string("keyinput")
			+ std::to_string(ckt.nKeys());
		id kid = ckt.add_wire(kname, wtype::KEY);
		added2new[x] = kid;
	}

	uint i = 0;
	for (auto& x : shuffle_set){
		idpair p = ckt.open_wire(x);
		added2new[nswb.in_vec[i]] = x;
		added2new[nswb.out_vec[i++]] = p.second;
	}

	ckt.add_circuit(nswb, added2new, std::to_string(ckt.nKeys()));

	return boolvec(0);
}



boolvec encrypt::enc_xor_rand(circuit& cir, const boolvec& key, fnct gtype, const idset& avoid_set) {

	uint iteration = 0;

	uint width = key.size();

	idset candidates;
	foreach_wire(cir, w, wid) {
		if ( (w.isInter() || w.isOutput()) && _is_not_in(wid, avoid_set) )
			candidates.insert(wid);
	}

	if (width > candidates.size()) {
		width = candidates.size();
		std::cout << "number of key-gates to insert is larger than number of wires (" << candidates.size() << ")."
				" Trimming key-width to " << width << "\n";
	}

	idset select_set = rand_from_set(candidates, width);

	for (auto wid : select_set) {
		insert_key_gate(cir, wid, key[iteration], gtype);
		iteration++;
		//ckt.write_bench();

	} while( iteration < width );

	return key;
}

gcon_pair encrypt::insert_key_gate(circuit& cir, id wid, bool key, fnct gtype) {
	// insert XOR
	gcon_pair gcp;
	if (gtype == fnct::XOR) {
		if (key) {
			gcp = insert_gate(cir, wid, fnct::XNOR, "xenc");
		}
		else {
			gcp = insert_gate(cir, wid, fnct::XOR, "xenc");
		}
	}
	else if (gtype == fnct::AND) {
		if (key) {
			gcp = insert_gate(cir, wid, fnct::AND, "xenc");
		}
		else {
			gcp = insert_gate(cir, wid, fnct::OR, "xenc");
		}
	}
	else {
		// unsuported function
		assert(false);
	}

	id xorB = gcp.gate_B;

	// set name and type
	string wname;
	wname = "keyinput" + std::to_string(cir.nKeys());
	cir.setwiretype(xorB, wtype::KEY);
	cir.setwirename(xorB, wname);

	return gcp;
}

boolvec encrypt::enc_xor_fliprate(circuit& cir, const boolvec& key, uint32_t num_patts, const idset& avoid_set) {
	using namespace stat_ns;

	id2realmap impMap;
	flip_impact_sim(cir, num_patts, impMap);

	Omap<real, idvec> flip2wid;

	foreach_wire(cir, w, wid) {
		if (w.isInter())
			flip2wid[impMap.at(wid)].push_back(wid);
	}

	for (auto& p : flip2wid) {
		std::cout << p.first << " : ";
		for (auto& wid : p.second) {
			std::cout << wid << " ";
		}
		std::cout << "\n";
	}

	assert(key.size() < (cir.nWires() - cir.nInputs() - cir.nOutputs()));

	uint nk = 0;
	auto mit = flip2wid.rbegin();
	uint i = 0;
	do {
		std::cout << "inserting key at " << mit->second[i] << "\n";
		insert_key_gate(cir, mit->second[i], key[nk++], fnct::XOR);
		if (i++ >= mit->second.size() - 1) {
			i = 0;
			if (++mit == flip2wid.rend())
				break;
		}
	} while(nk < key.size());

	return key;
}

boolvec encrypt::enc_xor_prob(circuit& cir, const boolvec& key, const idset& avoid_set){

	uint iteration = 0;
	uint width = key.size();

	using namespace stat_ns;

	id2realmap prbmap;
	propagate_prob(cir, prbmap);

	// convert signal probability values to activity
	for (auto p : prbmap) {
		p.second = fabs(p.second - 0.5);
	}

	Omap<real, id> sorted_probs;
	for (auto p : prbmap) {
		sorted_probs[p.second] = p.first;
	}

	vector< std::pair<id, real> > sorted_pairvec;
	for (auto& p : sorted_probs) {
		std::cout << cir.wname(p.second) << " : " << p.first << "\n";
		sorted_pairvec.push_back({p.second, p.first});
	}


	id r = 0;
	auto it = sorted_pairvec.begin();
	do{
		// pick wire considering range
		r = (*it).first;
		it++;
		if (it > sorted_pairvec.end()) break;
		std::cout << cir.wname(r) << std::endl;
		if ( cir.isKey(r) )
			continue;

		// insert XOR
		id xorB;
		if (key[iteration])
			xorB = insert_gate(cir, r, fnct::XNOR, "xenc").gate_B;
		else
			xorB = insert_gate(cir, r, fnct::XOR, "xenc").gate_B;

		// set name and type
		string wname;
		wname = "keyinput" + std::to_string(cir.nKeys());
		cir.setwiretype(xorB, wtype::KEY);
		cir.setwirename(xorB, wname);

		iteration++;
		//ckt.write_bench();

	} while( iteration < width );

	return key;
}



boolvec encrypt::enc_parity(circuit& ckt,
		uint tree_size, uint keys_per_tree,
		uint num_trees){

	assert(tree_size > keys_per_tree);

	ckt_block::tree xor_tree = ckt_block::get_tree(tree_size, fnct::XOR);

	id2idmap added2new;

	id original_n_wires = ckt.nWires();

	idset visited;

	using namespace stat_ns;

	idvec gate_ordering = ckt.get_topsort_gates();

	id count = 0, cpu = 0;
	do {
		cpu++;

		if (gate_ordering.empty()) {
			std::cout << "no more wires found!";
			break;
		}

		id gid = gate_ordering.back();
		gate_ordering.pop_back();
		id r = ckt.gfanout(gid);

		idset fanin_set;
		id fanin_size = tree_size - keys_per_tree;
		if (!_pick_from_fanin(fanin_set, ckt, r,
				tree_size, original_n_wires)) {
			continue;
		}

		// first insert xor_tree
		uint i = 0;
		for (auto& x : fanin_set)
			added2new[xor_tree.in_vec[i++]] = x;

		for (uint i = fanin_size; i < tree_size; i++)
			added2new[xor_tree.in_vec[i]] = ckt.add_key();

		id xor_tree_new_out = xor_tree.out + ckt.nWires();

		//add xor_tree
		ckt.add_circuit(xor_tree, added2new, std::to_string(count++));

		// get rid of stray outputs
		ckt.setwiretype(xor_tree_new_out, wtype::INTER);

//#		define PARWMUX
#		ifdef PARWMUX
		// insert mux
		bool new_left = (ckt.getcwires()[r].wiretype == OUT);
		id new_wr = ckt.open_wire(r, new_left);

		idvec muxins;

		string gatename = "parr_mux" + std::to_string(count);

		// create key for logic locking
		id key = ckt.new_key_wire();

		muxins.push_back(key);
		muxins.push_back((new_left) ? new_wr:r);
		muxins.push_back(xor_tree_new_out);

		// add first mux
		ckt.add_gate(gatename, muxins, (new_left) ? r : new_wr, MUX);
#		else
		// for ADN+XOR parity encryption

		// add the XOR
		id xor_b = insert_gate(ckt, r, fnct::XOR, "par_xor").gate_B;


		// no add the fnct::AND along with its key
		string gatename = "par_and" + std::to_string(count);
		id key = ckt.add_key();

		idvec gfanins;
		gfanins.push_back(key);
		gfanins.push_back(xor_tree_new_out);

		ckt.add_gate(gatename, gfanins, xor_b, fnct::AND);
		ckt.setwiretype(xor_b, wtype::INTER);

#		endif

		if (cpu > 1000) {
			std::cout << "key-limit not reached!\n";
			break;
		}

	} while ((count < num_trees));

	return boolvec(0);
}

void encrypt::turn_to_keywire(circuit& ckt, id wr, int offset) {
	string wname;
	wname = "keyinput" + ((offset == -1) ?
			std::to_string(ckt.nKeys()) :
			std::to_string(offset + ckt.nKeys()));
	ckt.setwiretype(wr, wtype::KEY);
	ckt.setwirename(wr, wname);
}


gcon_pair encrypt::insert_gate(circuit& ckt, id wr, fnct funct, string str) {

	//FIXME: this blocks business is no good
	circuit gate2insert;
	switch (funct) {
	case fnct::XOR:
		gate2insert = ckt_block::encXOR();
		break;
	case fnct::XNOR:
		gate2insert = ckt_block::encXNOR();
		break;
	case fnct::OR:
		gate2insert = ckt_block::encOR();
		break;
	case fnct::AND:
		gate2insert = ckt_block::encAND();
		break;
	case fnct::ANDN:
		gate2insert = ckt_block::encANDN();
		break;
	case fnct::NOT:
		gate2insert = ckt_block::Inverter();
		break;
	default:
		std::cout << "ERROR : unsupported gate type\n";
		exit(EXIT_FAILURE);
		break;
	}

	static id count = 0;

	id2idmap added2new;
	gcon_pair gp;

	idpair p = ckt.open_wire(wr, str + std::to_string(count++));

	added2new[gate2insert.find_wcheck("out")] = p.second;
	added2new[gate2insert.find_wcheck("A")] = p.first;

	ckt.add_circuit(gate2insert, added2new);

	gp.gate_B = (funct == fnct::NOT) ? -1 : added2new.at(gate2insert.find_wcheck("B"));
	gp.gate_O = p.second;

	return gp;
}


bool encrypt::_pick_from_fanin(idset& retset, const circuit& ckt,
		id wr, id size, id wrange) {

	const id LIMIT = 100;

#	define MMUXCYCLIC

	if ( size > ckt.nWires()) {
		std::cout
			<< "size of random subset larger than number of wires!\n";
		exit(EXIT_FAILURE);
	}

	if (wrange == -1){
		wrange = ckt.nWires();
	}

	idset transfanout;
	ckt.get_trans_fanoutset(wr, transfanout);

	retset.clear();
	id cnt = 0, cpu = 0;
	do {
		id r = rand() % wrange;
		if ( !ckt.isOutput(r) &&
#			ifndef MMUXCYCLIC
			transfanout.find(r) == transfanout.end() &&
#			endif
			r != wr								  &&
			retset.find(r) == retset.end()) {

			retset.insert(r);
			cnt++;
		}
		cpu++;
		if (cpu > LIMIT) {
			std::cout << "pick muxin limit reached\n";
			return true;
		}

	} while (cnt < size);

	return true;
}

bool encrypt::_pick_wires_random(idvec& retvec, const circuit& ckt,
		id count, int opt, id wrange) {
	idset retset;
	_pick_wires_random(retset, ckt, count, opt, wrange);
	retvec = idvec(retset.begin(),  retset.end());
	return true;
}

// a bad way to pick random wires
bool encrypt::_pick_wires_random(idset& retset, const circuit& ckt,
		id count, int opt, id wrange) {

	const id LIMIT = 100;

	assert(count < ckt.nWires());

	if (wrange == -1) {wrange = ckt.nWires();}

	retset.clear();

	id cnt = 0, cpu = 0;
	do {
		id r = rand() % wrange;
		if (
			( (opt & OPT_OUT) || !ckt.isOutput(r) ) &&
			( (opt & OPT_INP) || !ckt.isInput(r) ) &&
			( (opt & OPT_KEY) || !ckt.isKey(r) ) &&
			( (opt & OPT_INT) || !ckt.isInter(r) ) &&
			( retset.find(r) == retset.end() ) ) {

			retset.insert(r);
			cnt++;
		}
		cpu++;
		if (cpu > LIMIT) {
			std::cout << "pick wires limit reached\n";
			return true;
		}

	} while (cnt < count);

	return true;
}

}
