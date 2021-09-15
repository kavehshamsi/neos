/*
 * oless.cpp
 *
 *  Created on: Nov 24, 2019
 *      Author: kaveh
 */

#include <boost/functional/hash.hpp>
#include <dec/dec_oless_simil.h>
#include <utl/ext_cmd.h>
#include <utl/utility.h>
#include <bitset>
#include <algorithm>

namespace dec_ns {

using namespace utl;

void dec_oless_simil::run_0() {

	using namespace aig_ns;

	aig_t enc_aig(*enc_cir);

	ext::abc_simplify(enc_aig, true);
	double baseline_metric = get_metric(enc_aig);
	std::cout << "baseline metric " << baseline_metric << " \n";

	id2boolHmap cmap;
	aig_t cand_aig;
	for (auto kid : enc_aig.keys()) {
		// for 0
		cmap[kid] = 0;
		cand_aig = enc_aig;
		cand_aig.propagate_constants(cmap);
		ext::abc_simplify(cand_aig, true);
		double metric_for0 = get_metric(cand_aig);

		// for 1
		cand_aig = enc_aig;
		cmap[kid] = 1;
		auto cand_aig = enc_aig;
		cand_aig.propagate_constants(cmap);
		ext::abc_simplify(cand_aig, true);
		double metric_for1 = get_metric(cand_aig);

		std::cout << "metric for " << enc_aig.ndname(kid) << " -> 0:" << metric_for0 << "  1:" << metric_for1 << "\n";
	}
}

double dec_oless_simil::get_metric(const aig_t& ntk) {
	if (!rwr_mgr) {
		rwr_mgr = std::make_shared<rewrite_manager_t>();
	}

	Hset<truth_t> npnclasses;
	rwr_mgr->aig_npnclasses(ntk, npnclasses);

	return npnclasses.size();
}

using cir_cut_t = circuit::cir_cut_t;


void topsort_cut(const circuit& cir, const cir_cut_t& cut, idvec& ts_cut_gates) {
	const auto& tsort_gvec = cir.get_topsort_gates();
	for (int32_t i = tsort_gvec.size() - 1; i >= 0; i--) {
		if ( _is_in(tsort_gvec[i], cut.gates) ) {
			ts_cut_gates.push_back(tsort_gvec[i]);
		}
	}
}

void renum_cut(const circuit& cir, id2idHmap wid2num, const cir_cut_t& cut, const idvec& ts_cut_gids) {
	uint ind = 0;
	for (auto gid : ts_cut_gids) {
		wid2num[cir.gfanout(gid)] = ind++;
	}
	// all inputs have the same ind for Permutation-equality
	for (auto wid : cut.inputs) {
		wid2num[wid] = ind;
	}
}

typedef size_t cut_sig_t;

void get_cut_keys(const circuit& cir, const cir_cut_t& cut, oidset& cut_keys) {
	for (auto gid : cut.gates) {
		for (auto gin : cir.gfanin(gid)) {
			if (cir.isKey(gin))
				cut_keys.insert(gin);
		}
	}
}

void print_cut(const circuit& cir, const cir_cut_t& cut) {
	idvec ts_cut_gids;
	topsort_cut(cir, cut, ts_cut_gids);

	id2idHmap wid2num;
	renum_cut(cir, wid2num, cut, ts_cut_gids);

	for (auto gid : ts_cut_gids) {
		const auto& g = cir.getcgate(gid);
		std::cout << wid2num.at(g.fo0()) << " = " <<
				funct::enum_to_string(g.gfun()) << " (";
		for (auto gin : g.fanins) {
			std::cout << wid2num.at(gin) << ", ";
		}
		std::cout << ")\n";
	}
}

struct full_cut_t {
	id root;
	circuit cc;

	bimap<id, id> top2cut_wmap;
	bimap<id, id> top2cut_gmap;

	idvec tsorted_topgids;

	full_cut_t(const circuit& cir, const cir_cut_t& cut) {

		assert(!cut.gates.empty());

		root = top2cut_wmap.add_pair(cut.root, cc.add_wire(wtype::OUT));

		for (auto wid : cut.inputs) {
			wtype wt = cir.isKey(wid) ? wtype::KEY:wtype::IN;
			top2cut_wmap.add_pair( wid, cc.add_wire(wt) );
		}

		for (auto gid : cut.gates) {
			const auto& g = cir.getcgate(gid);

			for (auto gin : g.fanins) {
				if (!top2cut_wmap.is_in_firsts(gin)) {
					top2cut_wmap.add_pair(gin, cc.add_wire(wtype::INTER));
				}
			}
			if (!top2cut_wmap.is_in_firsts(g.fo0())) {
				top2cut_wmap.add_pair(g.fo0(), cc.add_wire(wtype::INTER));
			}

			top2cut_gmap.add_pair(g.nid, cc.add_gate(cir.gname(g), g, top2cut_wmap));
		}

		topsort_cut();

	}

	void topsort_cut() {
		tsorted_topgids = cc.get_topsort_gates();
		std::reverse(tsorted_topgids.begin(), tsorted_topgids.end());
	}

	void print_cut(const circuit& cir, const cir_cut_t& cut) {
		std::cout << "\n\nins: {";
		for (auto xid : cut.inputs) {
			std::cout <<  cir.wname(xid) << " ";// wid2nom.at(xid) << " ";
		}
		std::cout << "}\n";
		for (auto& gid : cut.gates) {
			const auto& g = cir.getcgate(gid);
			//std::cout << wid2nom.at(g.fanout) << " = " << funct::enum_to_string(g.function) << " (";
			std::cout << g.fo0() << " = " << funct::enum_to_string(g.gfun()) << " (";
			for (auto fanin : g.fanins) {
				std::cout << cir.wname(fanin) << " ";
			}
			std::cout << ")\n";
		}
	}


	void print_cut() {
		cc.write_bench();
	}

	cut_sig_t compute_shapehash() {

		size_t seed = 0;
		id2idHmap wid2num;
		uint ind = 1;
		for (auto gid : tsorted_topgids) {
			const auto& g = cc.getcgate(gid);
			wid2num[g.fo0()] = ind++;
		}
		for (auto wid : cc.allins()) {
			wid2num[wid] = 0;
		}

		for (auto& gid : tsorted_topgids) {
			const auto& g = cc.getcgate(gid);
			boost::hash_combine(seed, g.gfun());
			oidset gfanins;
			for (auto gin : g.fanins) {
				gfanins.insert(wid2num.at(gin));
			}
			for (auto gin : gfanins) {
				boost::hash_combine(seed, gin);
			}
		}

		return seed;
	}

	void simplify_cut(const circuit& cir, id2boolHmap& constmap) {
		id2boolHmap cutconstmap;
		for (auto& pr : constmap) {
			cutconstmap[top2cut_wmap.at(pr.first)] = pr.second;
		}
		cc.propagate_constants(cutconstmap);
	}

};


// cut based
void dec_oless_simil::run_1(uint k, uint num_cut, uint cut_key_size) {

	using namespace aig_ns;

	Hmap<id, double> key_scores;
	for (auto kid : enc_cir->keys()) {
		key_scores[kid] = 0.5;
	}

	std::cout << "k: " << k << " num_cut: " << num_cut << "\n";

	Hmap<cut_sig_t, idset> cs2widset;
	Hmap<id, vector<full_cut_t> > cutfmap;
	{
		Hmap<id, vector<cir_cut_t> > cutmap;
		enc_cir->enumerate_cuts(cutmap, k, num_cut, false);

		std::cout << "cut enumeration done\n";
		for (auto& p : cutmap) {
			for (auto& cv : p.second) {
				if (!cv.gates.empty() && cv.inputs.size() == k)
					cutfmap[p.first].push_back(full_cut_t(*enc_cir, cv));
				else
					cutfmap[p.first];
			}
		}
		std::cout << "built all cuts\n";
	}

	assert(cut_key_size < 10);

	Hmap<cut_sig_t, vector<full_cut_t> > cs2cutset;

	// record all cut signatures
	foreach_wire(*enc_cir, w, wid) {
		if (w.isInter() || w.isOutput()) {
			for (auto& cut : cutfmap.at(wid)) {

				// print_cut(enc_cir, cut);

				if (cut.cc.nKeys() <= cut_key_size) { // explore
					//cut.print_cut();
					cut_sig_t cs = cut.compute_shapehash();
					cs2widset[cs].insert(wid);
					//cs2cutset[cs].push_back(cut);
				}
			}
		}
	}
	std::cout << "num wires : " << enc_cir->nWires() << " -> shapehashes: " << cs2widset.size() << "\n";

	uint64_t max_cutclass_size = 0;
	for (auto& pr : cs2widset) {
		max_cutclass_size = MAX(max_cutclass_size, pr.second.size());
	}

/*
	for (auto& pr : cs2cutset) {
		printf("\n\nPer shapehash: %llx\n", pr.first);
		for (auto& c : pr.second) {
			c.print_cut();
		}
	}
*/


	//exit(1);
	// test cut signatures for different subkey assignments
	foreach_wire(*enc_cir, w, wid) {
		if (w.isInter() || w.isOutput()) {
			// std::cout << " at wire " << enc_cir.wname(wid) << " : ";
			for (auto& cut : cutfmap.at(wid)) {
				if (cut.cc.nKeys() != 0 && cut.cc.nKeys() <= cut_key_size && cut.cc.nGates() > 2) { // explore
					//std::cout << "c:\n";

					int64_t best_cut_score = 0;
					uint best_key = 0;
					for (uint d = 0; d < pow(2, cut.cc.nKeys()); d++) {
						auto cut_copy = cut;
						std::bitset<16> bs(d);

						id2boolHmap keymap;
						// std::cout << "setting subkey :";
						uint i = 0;
						for (auto ckid : cut.cc.keys()) {
							id kid = cut.top2cut_wmap.ta(ckid);
							keymap[kid] = bs[i];
							//std::cout << bs[i];
							i++;
						}

						cut_copy.simplify_cut(*enc_cir, keymap);

						cut_sig_t cs = cut_copy.compute_shapehash();

						uint cut_score = 0;
						//std::cout << " -> ";
						if (_is_in(cs, cs2widset)) {
							cut_score = cs2widset.at(cs).size();
						}
						if (cut_score > best_cut_score) {
							std::cout << "\n\nwinning cut: (" << cut_score << ")\n";
							cut_copy.print_cut();
							best_cut_score = cut_score;
							best_key = d;
						}
						//std::cout << cut_score << "\n";
					}


					std::bitset<16> bk(best_key);
					// std::cout << "best_key: " << bk << "\n";
					uint i = 0;
					for (auto ckid : cut.cc.keys()) {
						id kid = cut.top2cut_wmap.ta(ckid);
						float key_delta = (bk[i] - 0.5) * ((double)best_cut_score / max_cutclass_size);
						if (key_delta != 0)
							std::cout << "key delta " << key_delta << "\n";
						float new_key_score = key_scores.at(kid) + key_delta;
						key_scores[kid] = utl::clip(new_key_score, 0, 1);
						i++;
					}
				}
			}
			//std::cout << "\n";
		}
	}

	std::cout << "key: ";

	double correct_keys = 0;
	boolvec lkey;
	for (auto kid : enc_cir->keys()) {
		std::cout << "  " << key_scores.at(kid);
		bool kv = key_scores.at(kid) > 0.5;
		lkey.push_back(kv);
		if (kv == 0)
			correct_keys++;
	}
	std::cout << "\n";


	std::cout << "key=" << to_str(lkey) << "  (" << (correct_keys/enc_cir->nKeys() * 100) << "%)\n";
	check_key(lkey);

	boolvec randkey = utl::random_boolvec(lkey.size());
	std::cout << "vs random key=" << to_str(randkey) << "\n";
	check_key(randkey);
}

} // namespace dec


