/*
 * map.h
 *
 *  Created on: Apr 8, 2020
 *      Author: kaveh
 */

#ifndef SRC_AIG_MAP_H_
#define SRC_AIG_MAP_H_

#include "aig/aig.h"
#include "aig/cut.h"
#include "base/cell_lib.h"
#include "parser/liberty/readliberty.h"
#include <boost/dynamic_bitset.hpp>

#include <boost/variant.hpp>
#include <any>

#include "utl/npn.h"

namespace aig_ns {

using namespace ckt;
using namespace aig_ns;

using boost::variant;
using namespace utl_npn;

class tech_map_t {
public:
	cell_library* _clib = nullptr;
	aig_t* ntk = nullptr;

	uint max_num_ins = 0;
	vector<uint> input_inds;
	float total_cost = 0;

	enum opt_strategy { AREA, DELAY } opt_st;

	struct cell_match_t {
		id cellid;
		id perm_index;
		id neg_index;
		id cutindex;
	};

	struct map_data_t {
		id nid;
		bool is_root = true;
		double cost = 0;
		id cellid;
		id best_match = -1;
		vector<cell_match_t> matches;
	};

	typedef boost::dynamic_bitset<> dbitset;

	struct cellperm {
		id perm_index;
	};

	struct cmap_obj {
		// possible implementations
		Hmap<id, cellperm> impls;
	};

	// The mapping map that maps the mapping to the map
	idmap<map_data_t> mMap;

	// num-in to all perms
	vector<vector<vector<uint8_t>>> all_perms;

	// npn structs
	vector<dyn_npn_enumerator> npn_enums;

	// simulation inputs (truth-table of singular variables)
	vector<vector<dbitset>> invar_tts;

	// vector maps num-ins to hash-table, hash-table maps truth-table to perm
	vector< Hmap< dbitset, cmap_obj > > cMaps;
	std::multimap<float, id> bufs;
	std::multimap<float, id> invs;

	// map node to cutvec
	idmap<cutvec_t> cutMap;

	// cost and mapping for each cut
	struct cut_mapped_t {
		id cellid;
		id perm_index;
		double area;
	};
	idmap<vector<cut_mapped_t>> cutCostMap;

	tech_map_t(aig_t& ntk, cell_library& _clib) : ntk(&ntk), _clib(&_clib) {}

	void init_map();
	void compute_lib_funcs();
	void map_to_lib(int num_iterations);
	void simulate_cut(const cut_t& cut, idmap<dbitset>& simmap);
	void match_cuts();
	void find_cover();
	void build_perms_and_negs();
	void build_invar_tts();
	void print_map();
	void to_cir(circuit& cir);
};

} // namespace aig_ns

#endif /* SRC_AIG_MAP_H_ */
