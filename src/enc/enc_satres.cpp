#include "enc.h"
#include "stat/stat.h"

//////////////// The circuit class ////////////////////
namespace enc {

boolvec encrypt::enc_antisat(circuit& cir, int ant_width, int num_trees,
		bool pick_internal_ins, bool pick_internal_outs, int xor_obf_tree, const idvec& roots) {

    srand(clock());

    boolvec corrkey;

    for (int ntree = 0; ntree < num_trees; ntree++) {

		id output = (roots.size() <= ntree) ? -1 : roots.at(ntree);
		idset treeins;

		if (!_pick_tree_input_outputs(output, treeins, cir, ant_width,
				pick_internal_ins, pick_internal_outs, false)) {
			std::cout << "could not find cone with enough wires to insert tree\n";
			break;
		}
		else {
			ckt_block::tree antisat = ckt_block::anti_sat(ant_width);
			boolvec key_half = utl::random_boolvec(ant_width);
			push_all(corrkey, key_half);
			push_all(corrkey, key_half);
			if (xor_obf_tree) {
				idset avoidset = utl::to_hset(antisat.key_vec);
				utl::add_all(avoidset, antisat.in_vec);
				avoidset.insert(antisat.out);
				for (auto xid : antisat.in_vec) {
					for (auto wid : antisat.wfanoutWires(xid)) {
						avoidset.insert(wid);
					}
				}
				std::cout << "avoid set: " << antisat.to_wstr(avoidset) << "\n";

				enc_xor_rand(antisat, boolvec(xor_obf_tree, 0), fnct::XOR, avoidset);
				push_all(corrkey, boolvec(xor_obf_tree, 0));
				antisat.write_bench();
				antisat.key_vec = utl::to_vec(antisat.keys());
			}
			gcon_pair gp = insert_tree(cir, antisat, utl::to_vec(treeins), output, fnct::XOR);
			// cir.setwirename(gp.gate_B, "antisat_tip_" + std::to_string(ntree));
		}
	}

	return corrkey;

}

void encrypt::diversify_tree(ckt_block::tree& tir, Omap<idpair, fnct> flipmap, bool is_double) {

	if (is_double) {
		int halfsz = tir.in_vec.size()/2;
		for (int shift : {0, halfsz}) {
			vector<idvec> glayers, wlayers;
			tir.get_layers(glayers, wlayers, utl::subvec(tir.in_vec, 0 + shift, halfsz + shift - 1));

			for (auto p : flipmap) {
				id lrnum = p.first.first;
				id lrpos = p.first.second;
				id gid = glayers.at(lrnum).at(lrpos);
				tir.setgatefun(glayers.at(lrnum).at(lrpos), p.second);
				std::cout << "changing gate with output " << tir.wname(tir.gfanout(gid)) <<
					" to " << funct::enum_to_string(p.second) << "\n";
			}
		}
	}
	else {
		vector<idvec> glayers, wlayers;
		tir.get_layers(glayers, wlayers);

		for (auto p : flipmap) {
			id lrnum = p.first.first;
			id lrpos = p.first.second;
			id gid = glayers.at(lrnum).at(lrpos);
			tir.setgatefun(gid, p.second);
			std::cout << "changing gate with output " << tir.wname(tir.gfanout(gid)) <<
				" to " << funct::enum_to_string(p.second) << "\n";
		}
	}

}

boolvec encrypt::enc_antisat_dtl(circuit& cir, int ant_width, int num_firstlayer_flips, fnct flip_fun, int num_trees,
		bool pick_internal_ins, bool pick_internal_outs, int xor_obf_tree, const idvec& roots) {

    srand(clock());

    boolvec corrkey;

    for (int ntree = 0; ntree < num_trees; ntree++) {

		id output = (roots.size() <= ntree) ? -1 : roots.at(ntree);
		idset treeins;

		if (!_pick_tree_input_outputs(output, treeins, cir, ant_width,
				pick_internal_ins, pick_internal_outs, false)) {
			std::cout << "could not find cone with enough wires to insert tree\n";
			break;
		}
		else {
			ckt_block::tree antisat = ckt_block::anti_sat(ant_width);
			antisat.write_bench();
			boolvec key_half = utl::random_boolvec(ant_width);
			push_all(corrkey, key_half);
			push_all(corrkey, key_half);
			Omap<idpair, fnct> flipmap;
			auto rinds = utl::rand_indices(num_firstlayer_flips, ant_width/2);
			for (auto rind : rinds)
				flipmap[{1, rind}] = flip_fun;

			diversify_tree(antisat, flipmap, true);

			if (xor_obf_tree) {
				idset avoidset = utl::to_hset(antisat.key_vec);
				utl::add_all(avoidset, antisat.in_vec);
				avoidset.insert(antisat.out);
				for (auto xid : antisat.in_vec) {
					for (auto wid : antisat.wfanoutWires(xid)) {
						avoidset.insert(wid);
					}
				}
				std::cout << "avoid set: " << antisat.to_wstr(avoidset) << "\n";
				enc_xor_rand(antisat, boolvec(xor_obf_tree, 0), fnct::XOR, avoidset);
				push_all(corrkey, boolvec(xor_obf_tree, 0));
				antisat.write_bench();
				antisat.key_vec = utl::to_vec(antisat.keys());
			}
			gcon_pair gp = insert_tree(cir, antisat, utl::to_vec(treeins), output, fnct::XOR);
			//cir.setwirename(gp.gate_B, "antisat_tip_" + std::to_string(ntree));
		}
	}

	return corrkey;

}


boolvec encrypt::enc_sfll_point(circuit& cir, int width, int num_trees,
		bool pick_internal_ins, bool pick_internal_outs, int xor_obf_tree, const idvec& roots) {

	boolvec corrkey;

	for (int ntree = 0; ntree < num_trees; ntree++) {

		id output = (roots.size() <= ntree) ? -1 : roots.at(ntree);
		idset treeins;

		if (!_pick_tree_input_outputs(output, treeins, cir, width,
				pick_internal_ins, pick_internal_outs, true)) {
			std::cout << "could not find cone with enough wires to insert tree\n";
			break;
		}
		else {
			std::cout << "inserting tree\n";
			boolvec flip_loc = utl::random_boolvec(width);
			utl::push_all(corrkey, flip_loc);
			ckt_block::tree fliptree = ckt_block::fixed_comparator(width, flip_loc);
			gcon_pair gp = insert_tree(cir, fliptree, utl::to_vec(treeins), output, fnct::XOR);
			id fliptip = gp.gate_O;
			ckt_block::tree crcttree = ckt_block::key_comparator(width);
			insert_tree(cir, crcttree, utl::to_vec(treeins), fliptip, fnct::XOR);
		}
	}

	return corrkey;

}

gcon_pair encrypt::insert_tree(circuit& cir, ckt_block::tree& tree,
		const idvec& inputs, id output, fnct insert_gatefun) {
	id2idmap added2new;
	return encrypt::insert_tree(cir, tree, inputs, output, insert_gatefun, added2new);
}

gcon_pair encrypt::insert_tree(circuit& cir, ckt_block::tree& tree, const idvec& inputs, id output,
		fnct insert_gatefun, id2idmap& added2new) {

	static int treetip = 0;
	gcon_pair gp = insert_gate(cir, output, fnct::XOR, "antop");
	cir.setwiretype(gp.gate_B, wtype::INTER);
	cir.setwirename(gp.gate_B, fmt("treetip%1%", treetip++));
	for (uint i = 0; i < tree.in_vec.size(); i++) {
		added2new[tree.in_vec[i]] = inputs[i];
	}
	for (uint i = 0; i < tree.key_vec.size(); i++) {
		added2new[tree.key_vec[i]] = cir.add_key();
	}
	added2new[tree.out] = gp.gate_B;
	cir.add_circuit(tree, added2new);
	for (auto kid : tree.key_vec) {
		std::cout << "key: " << tree.wname(kid) << " -> " << cir.wname(added2new.at(kid)) << "\n";
	}
	return gp;

}

bool encrypt::_pick_tree_input_outputs(id& output, idset& treeins, circuit& cir, int width,
		bool pick_internal_ins, bool pick_internal_outs, bool kcut) {

	if (output != -1) {
		if (!_pick_tree_inputs(treeins, cir, output, width, pick_internal_ins)) {
			return false;
		}
	}
	else {
		auto rindvec = utl::rand_indices(cir.nWires());
		bool inserted = false;
		idvec wtopsort = cir.get_topsort_wires();

		for (auto rind : rindvec) {

			output = wtopsort[rind];

			if (!pick_internal_outs && !cir.isOutput(output))
				continue;

			if (kcut) {
				idset kcut_gates;
				if (cir.get_kcut(output, width, kcut_gates, treeins, false, nullptr)) {
					inserted = true;
					break;
				}
			}
			else {
				idset faninset, faninpis;
				cir.get_trans_fanin(output, faninset, faninpis);

				if (_pick_tree_inputs(treeins, cir, output, width, pick_internal_ins)) {
					inserted = true;
					break;
				}
			}
		}

		if (!inserted) {
			std::cout << "could not find cone with enough wires to insert tree\n";
			return false;
		}
		else {
			std::cout << "picked root " << cir.wname(output) << "\n";
			std::cout << "picked tree input set {" << cir.to_wstr(treeins, ", ") << "}";
			return true;
		}
	}

	return false;

}

bool encrypt::_pick_tree_inputs(idset& treeins, circuit& cir, id output, int ant_width, bool pick_internal) {

	idset faninset, faninpis, nonkey_inputs, nonkey_tfanins;
	cir.get_trans_fanin(output, faninset, faninpis);
	for (auto fanin : faninset) {
		if (!cir.isKey(fanin)) {
			nonkey_tfanins.insert(fanin);
		}
		if (cir.isInput(fanin)) {
			nonkey_inputs.insert(fanin);
		}
	}

	if (!pick_internal) {
		if (nonkey_inputs.size() >= ant_width) {
			treeins = utl::rand_from_set(nonkey_inputs, ant_width);
			return true;
		}
	}
	else {
		if (nonkey_tfanins.size() >= ant_width) {
			treeins = utl::rand_from_set(nonkey_tfanins, ant_width);
			return true;
		}
	}

	return false;

}

}
