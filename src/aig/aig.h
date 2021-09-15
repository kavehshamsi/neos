/*
 * aig.h
 *
 *  Created on: Apr 6, 2018
 *      Author: kaveh
 *      Description: A c++ STL based AIG package
 */

#ifndef SRC_AIG_AIG_H_
#define SRC_AIG_AIG_H_

#define NEOS_DEBUG_LEVEL 0

#include "base/base.h"
#include "base/circuit.h"
#include "boost/functional/hash.hpp"

// macros
#define foreach_nodeid(ckt, nid) \
	RANGE_INIT_FOR(auto _macro_gp : (ckt).getnodeMap(), (id nid = _macro_gp.first), ((void)nid))

#define foreach_node(ckt, nd, nid) \
	RANGE_INIT_FOR(auto _macro_gp : (ckt).getnodeMap(), \
		(id nid = _macro_gp.first)(const auto& nd = _macro_gp.second), ((void)nid)((void)nd))

// macros
#define foreach_and(ckt, ad, aid) \
	RANGE_INIT_FOR(auto _macro_gp : (ckt).getnodeMap(), \
			(const aigNode& ad = _macro_gp.second)(id aid = _macro_gp.first), ((void)aid)((void)ad))\
		if (_macro_gp.second.isAnd())

#define foreach_andid(ckt, nid) \
	RANGE_INIT_FOR(auto _macro_gp : (ckt).getnodeMap(), (id nid = _macro_gp.first), ((void)nid))\
		if (_macro_gp.second.isAnd())

#define foreach_node_ordered(ckt, nd, nid) \
	RANGE_INIT_FOR(id nid : (ckt).top_order(), (const aigNode& nd = (ckt).getcNode(nid)), ((void)nid)((void)nd))\

#define foreach_and_ordered(ckt, aid) \
	for (auto aid : (ckt).top_order())\
		if ((ckt).isAnd(aid))

#define foreach_inaid(ntk, xid) for (auto xid : (ntk).inputs())
#define foreach_outaid(ntk, yid) for (auto yid : (ntk).outputs())

namespace aig_ns {

enum ndtype_t {
	NTYPE_IN = 0,
	NTYPE_OUT = 1,
	NTYPE_AND = 2,
	NTYPE_CONST0 = 3,
	NTYPE_LATCH = 4,
	NTYPE_EMPTY = 5
};

using namespace ckt;
using namespace utl;

/*
 * AIG literal. is an id shifted up by one to create room for
 * a single complement-bit.
 */

class alit {
public:
	id x = -1;
public:
	alit() {}
	alit(id i, bool sign = false) : x((i << 1) | (id)sign) {}
	inline alit& operator^=(bool sign) { x ^= (id)sign; return *this; }
	inline bool sgn() const { return x & 1; }
	inline void setsgn(bool sign) { x |= (id)sign; }
	inline void setid(id i) { x = (i << 1) | (id)(x & 1); }
	inline id lid() const { return x >> 1; }
	inline id getx() const { return x; }
	inline alit& operator=(id p) { x = (p << 1); return *this; }

	friend std::ostream& operator<<(std::ostream& out, const alit& al);
};

// specialized hash function for aigNodes
struct alit_Hash {

	std::size_t operator()(const alit& af) const {
		return std::hash<id>{}(af.getx());
	}
};

inline bool operator==(alit a, alit b) { return a.x == b.x; }
inline bool operator!=(alit a, alit b) { return a.x != b.x; }
inline bool operator==(alit a, id p) { return a.lid() == p; }
inline bool operator==(id p, alit a) { return a.lid() == p; }
inline bool operator!=(alit a, id p) { return a.lid() != p; }
inline bool operator!=(id p, alit a) { return a.lid() != p; }
inline alit operator^(alit a, bool b) { alit r = a; r ^= b; return r; }
inline alit operator|(alit a, bool b) { alit r = a; r.setsgn(b); return r; }
inline alit operator^(bool b, alit a) { alit r = a; r ^= b; return r; }
inline alit operator~(alit a) { alit b; b.x = a.x ^ 1; return b; }

typedef std::unordered_map<id, alit> id2alitmap;
typedef std::unordered_set<alit, alit_Hash> alitset;
typedef std::vector<alit> alitvec;
typedef std::queue<alit> alitque;

// the aigNode : size =
class aigNode {
public:
	id nid;
	alit fanin0;
	alit fanin1;
	bool markA : 1, markB : 1, markC : 2;
	ndtype_t ndtype : 3;

	idset fanouts; // node fanout idset

	int16_t depth;

	aigNode();
	aigNode(id nid, id fanin0, bool cmask0, id fanin1, bool cmask1, ndtype_t ntype);
	aigNode(id nid, alit fanin0, alit fanin1, ndtype_t ntype);

	id getid() const { return nid; }

	bool isInput() const { return ndtype == NTYPE_IN; }
	bool isOutput() const { return ndtype == NTYPE_OUT; }
	bool isAnd() const { return ndtype == NTYPE_AND; }
	bool isLatch() const { return ndtype == NTYPE_LATCH; }
	bool isConst0() const { return ndtype == NTYPE_CONST0; }
	const idset& nfanouts() const;
	id nfanout0() const;

	inline id fi0() const { return fanin0.lid(); }
	inline id fi1() const { return fanin1.lid(); }
	inline bool cm0() const { return fanin0.sgn(); }
	inline bool cm1() const { return fanin1.sgn(); }

	void addfanout(id aid) { fanouts.insert(aid); }
	void removefanout(id aid) { fanouts.erase(aid); }

	ndtype_t ntype() const { return ndtype; }

};

// fanin key structure used for lookup
class aigFanin {
public:
	alit fl; // lowest lid
	alit fr;

	// will automatically sort upon creation
	aigFanin(alit afanin0, alit afanin1) :
		fl(afanin0), fr(afanin1) {
		// sort if necessary
		if (fl.lid() > fr.lid()) utl::swap(fl, fr);
	}

	// will automatically sort upon creation
	aigFanin(const aigNode& nd) :
		fl(nd.fanin0), fr(nd.fanin1) {
		// sort if necessary
		if (fl.lid() > fr.lid()) utl::swap(fl, fr);
	}

	bool operator==( const aigFanin& af2 ) const {
		return 	   fl == af2.fl
				&& fr == af2.fr;
	}

	inline id fi0() const { return fl.lid(); }
	inline id fi1() const { return fr.lid(); }

	friend std::ostream& operator<<(std::ostream& os, const aigFanin& fanin2) {
		os << "2-fanin: " << fanin2.fl << " " << fanin2.fr;
		return os;
	}

};


// specialized hash function for aigNodes
struct aigFanin_Hash {

	std::size_t operator()(const aigFanin& af) const {
		using boost::hash_combine;
		std::size_t seed = 0;

		// mix fanin ids and fanin complement flags
		hash_combine(seed, af.fl.x);
		hash_combine(seed, af.fr.x);

		return seed;
	}
};

typedef std::unordered_map<aigFanin, id, aigFanin_Hash> aigFaninTable;

// fanin key structure used for 3-input structures
class aigFanin3 {
public:
	// three complemented variables and four complement-masks
	// comprise the 3-input signature
	alit fl;


	bool cmr;
	alit frl; // low in pair
	alit frr;

	aigFanin3() {} // uninitalized for speed

	// will automatically sort upon creation
	aigFanin3(alit afl, bool acmr, alit afrl, alit afrr) :
		fl(afl), frl(afrl), frr(afrr), cmr(acmr) {
		// sort if necessary
		if (frl.lid() > frr.lid()) utl::swap(frl, frr);
	}
	bool operator==( const aigFanin3& af2 ) const {
		return 	   fl == af2.fl
				&& frl == af2.frl
				&& frr == af2.frr
				&& cmr == af2.cmr;
	}

	friend std::ostream& operator<<(std::ostream& os, const aigFanin3& fanin3) {
		os << "3-fanin: " << fanin3.fl << " " << fanin3.cmr
			<< " " << fanin3.frl << " " << fanin3.frr;
		return os;
	}

};


// specialized hash function for aigNodes
struct aigFanin3_Hash {

	std::size_t operator()(const aigFanin3& af) const {
		using boost::hash_combine;
		std::size_t seed = 0;

		// mix fanin ids and fanin complement flags
		hash_combine(seed, af.fl.x);
		hash_combine(seed, af.frl.x);
		hash_combine(seed, af.frr.x);
		hash_combine(seed, af.cmr);

		return seed;
	}
};
typedef std::unordered_map<aigFanin3, id, aigFanin3_Hash> aigFanin3Table;


// fanin key structure used for lookup
class aigFanin4 {
public:
	// three complemented variables and four complement-masks
	// comprise the 3-input signature
	alit fll; // low in pair, low among pairs
	alit flr;
	alit frl; // low in pair, high among pairs
	alit frr;
	bool cml, cmr;

	aigFanin4() {} // uninitalized for speed

	// will automatically sort upon creation
	aigFanin4(alit afanin0, alit afanin1, alit afanin2, alit afanin3,
			bool acml, bool acmr) :
		fll(afanin0), flr(afanin1), frl(afanin2), frr(afanin3),
		cml(acml), cmr(acmr) {
		// sort if necessary
		if (fll.lid() > flr.lid()) utl::swap(fll, flr);
		if (frl.lid() > frr.lid()) utl::swap(frl, frr);

		if (fll.lid() > frl.lid()) {
			utl::swap(fll, frl);
			utl::swap(flr, frr);
			utl::swap(cml, cmr);
		}
	}

	bool operator==( const aigFanin4& af2 ) const {
		return 	   fll == af2.fll
				&& flr == af2.flr
				&& frl == af2.frl
				&& frr == af2.frr
				&& cml == af2.cml
				&& cmr == af2.cmr;
	}

	friend std::ostream& operator<<(std::ostream& os, const aigFanin4& fanin4) {
		os << "4-fanin: " << fanin4.fll << " " << fanin4.flr
								<< " " << fanin4.cml << " " << fanin4.frl
								<< " " <<  fanin4.frr << " " << fanin4.cmr;
		return os;
	}
};

// specialized hash function for aigNodes
struct aigFanin4_Hash {

	std::size_t operator()(const aigFanin4& af) const {
		using boost::hash_combine;
		std::size_t seed = 0;

		// mix fanin ids and fanin complement flags
		hash_combine(seed, af.fll.x);
		hash_combine(seed, af.flr.x);
		hash_combine(seed, af.frl.x);
		hash_combine(seed, af.frr.x);
		hash_combine(seed, af.cml);
		hash_combine(seed, af.cmr);

		return seed;
	}
};
typedef std::unordered_map<aigFanin4, id, aigFanin4_Hash> aigFanin4Table;


namespace dummy_defaults {
static bool _dummy_bool_defualt = false; // I hate doing this. C++'s fault
}

class aig_t {
private:
	static const int verbose = 0;

	idmap<aigNode> nodeMap; // id to node object mapping

	// the fanin table does not include latches and outputs
	aigFaninTable faninTable; // node table (aigFanin -> nodeId)

	// only used in case of 2-level strashing.
	static const bool _two_level_hashing = false;
	aigFanin3Table fanin3Table;
	aigFanin4Table fanin4Table;

	oidset input_set; // inputs including keys
	oidset key_set; // KIs
	oidset output_set; // POs
	oidset latch_set; // latch set

	id aConst0 = -1;

	uint64_t number_of_nodes = 0;

	bimap<id, string> nodeNames;

	bool order_needs_update = true;
	idvec top_order_vec;

// some public types
public:

	string top_name;

public:

	aig_t() {} // empty aig
	aig_t(const circuit& ckt);
	aig_t(const circuit& ckt, id2alitmap& wid2almap); // construct from circuit
	void init_from_ckt(const circuit& ckt, id2alitmap& wid2almap);

	aig_t(const string& filename, char format = 'b');

	const oidset& inputs() const { return input_set; }
	const oidset& keys() const { return key_set; }
	const oidset& outputs() const { return output_set; }
	const oidset& latches() const { return latch_set; }
	oidset nonkey_inputs() const;
	oidset combins() const;

	uint64_t nInputs() const { return input_set.size(); }
	uint64_t nOutputs() const { return output_set.size(); }
	uint64_t nNodes() const { return number_of_nodes; }
	uint64_t nAnds() const { return number_of_nodes - nInputs() - nOutputs() - nConst(); }
	uint64_t nConst() const { return (has_const0() ? 1:0); }
	uint64_t nLatches() const { return latch_set.size(); }

	static id lookup_fanintable(const aigNode& nd, const aigFaninTable& fanin_table);
	static id lookup_fanintable(const aigFanin& afanin, const aigFaninTable& fanin_table);

	alit add_and(const aigNode& nd, string nm = "",
			bool& strashed = dummy_defaults::_dummy_bool_defualt);
	alit add_and(const aigNode& nd, const id2alitmap& mapping, const string& nm = "",
			bool& strashed = dummy_defaults::_dummy_bool_defualt);
	alit add_and(alit fanin0, alit fanin1, const string& nm = "",
			bool& strashed = dummy_defaults::_dummy_bool_defualt);
	alit add_and(id fanin0, bool faninc0, id fanin1, bool faninc1, const string& nm = "",
			bool& strashed = dummy_defaults::_dummy_bool_defualt);

	// special function for just checking if AND is trivial or exists
	alit lookup_and(alit fanin0, alit fanin1, bool& strashed = dummy_defaults::_dummy_bool_defualt) const;

	// two level hashing. (TODO: doesnt work)
	id two_level_strash_and(alit f0, alit f1, const string& nm, bool& strashed);

	bool has_const0() const { return aConst0 != -1; };
	alit get_const0();
	alit get_cconst0() const;
	id get_const0id();
	id get_cconst0id() const;
	template<typename M, typename T = bool>
	void add_const_to_map(M& id2tmap, T c0 = false) const {
		if (has_const0()) id2tmap[get_cconst0id()] = c0;
	}
	template<typename T = bool>
	void add_const_to_vec(std::vector<T>& v1, T c0 = false) const {
		if (has_const0()) v1.push_back(c0);
	}
	bool node_exists(alit al) const;
	bool node_exists(id aid) const;
	inline aigNode& getNode(id aid);
	inline const aigNode& getcNode(id aid) const;
	aigFanin getFaninObj(id aid);

	id add_new_key();
	void add_to_keys(id aid);
	id add_key(const string& inName);
	id add_key(id kid, const string& inName);
	id add_input(const string& inName);
	id add_input(id nid, const string& inName);
	id add_output(const string& outName);
	id add_output(const string& outname, alit faninlit);
	id add_output(const string& outName, id fanin, bool mask);
	id add_output(id nid, const string& outname, id fanin, bool mask);

	void add_edge(alit ul, id vid, int fanin_index = -1);

	id add_latch(const string& latchname = "");
	id add_latch(id fanin, bool cmask, const idset& fanout, const string& latchname = "");
	id join_outputs(fnct fn, const string& outname = "");
	id join_outputs(const idvec& outputs, fnct fn, const string& outname = "");
	alit add_complex_gate(const circuit& ckt, id gid, const id2alitmap& wid2idmap,
			const string& nodename = "");
	alit add_complex_gate(fnct gfun, const std::vector<alit>& faninlits,
					const string& nodename = "");
	alit add_complex_gate(fnct gfun, const idvec& fanins, const boolvec& cmasks,
				const string& nodename = "");

	void aig_test();
	void to_circuit(circuit& ckt) const;
	circuit to_circuit() const;

	bool error_check() const;

	id get_max_id() const { return nodeMap.max_id(); }

	bool isInput(id aid) const { return getcNode(aid).ndtype == NTYPE_IN; }
	bool isKey(id aid) const { return _is_in(aid, key_set); }
	bool isOutput(id aid) const { return getcNode(aid).ndtype == NTYPE_OUT; }
	bool isConst0(id aid) const { return aid == aConst0; }
	bool isLatch(id aid) const { return getcNode(aid).ndtype == NTYPE_LATCH; }
	bool isLatch_input(id aid, id* latch = NULL) const;
	bool isLatch_output(id aid, id* latch = NULL) const;
	bool isAnd(id aid) const { return ntype(aid) == NTYPE_AND; }
	ndtype_t ntype(id aid) const { return getcNode(aid).ndtype; }

	const string& ndname(id aid) const;
	string ndname(alit al) const;
	void set_node_name(id aid, const string& nm);
	string get_new_name(id hint = -1, ndtype_t nt = NTYPE_AND) const;
	void freeup_node_name(const string& nm);
	id find_node(const string& nm) const;
	id find_wcheck(const string& nm) const;
	string latch_to_string(const aigNode& nd) const;
	string to_str(id nid) const;
	string to_str(const aigNode& nd) const;
	string and_to_str(const aigNode& nd) const;
	void print_node(id aid) const;
	void print_stats() const;

	id nfanin0(id aid) const { return getcNode(aid).fanin0.lid(); }
	id nfanin1(id aid) const { return getcNode(aid).fanin1.lid(); }

	// marks
	inline void clear_markA() { for( auto p : nodeMap) p.second.markA = 0; }
	inline void clear_markB() { for( auto p : nodeMap) p.second.markB = 0; }
	inline void clear_markC() { for( auto p : nodeMap) p.second.markC = 0; }

	void set_fanins(aigNode& nd, alit fanin01);

	id nfanout0(id aid) const;
	const idset& nfanouts(id aid) const;

	id ncmask0(id aid) const { return getcNode(aid).fanin0.sgn(); }
	id ncmask1(id aid) const { return getcNode(aid).fanin1.sgn(); }

	uint16_t nlevel(id aid) const { return getcNode(aid).depth; }

	const idmap<aigNode>& getnodeMap() const { return nodeMap; }
	void resolve_mask(const circuit& ckt, id rootid, bool& mask, id& aigwid);

	// AIG modification
	void remove_node(id aid);
	void update_node_fanin(id nid, id oldfanin, id newfanin, bool invert = false);
	void update_node_fanin(aigNode& nd, id oldfanin, id newfanin, bool mask);
	void move_all_fanouts(id oldnode, id newfanin, bool invert);
	bool check_constness(const aigNode& nd, alit& retnd);
	bool check_constness(alit f0, alit f1, alit& retnd);
	void merge_nodes(id nd1, id nd2, bool invert = false, bool check_cycle = false);
	void strash_fanout(id aid);
	void propagate_constants(const id2boolHmap& cstMap);
	void remove_node_recursive(id aid);
	void remove_node_recursive(const aigNode& nd);
	int remove_dead_logic();
	int trim_aig_to(const oidset& outs);

	// Composition: connects inputs according to name
	void add_aig(const aig_t& aig2, const string& postfix = "");
	void add_aig(const aig_t& aig2, id2alitmap& conMap, const string& postfix = "");

	// DAG functions
	const idvec& top_order() const;
	idvec top_andorder() const;
	void clear_orders() const;
	idset trans_fanout(id aid) const;
	idset trans_fanin(id aid) const;
	idset trans_fanin(id aid, idset& pis) const;
	idvec topsort_trans_fanin(id aid);

	void simulate_comb(id2boolmap& simMap) const;
	void simulate_comb(const id2boolmap& inputmap, id2boolmap& simMap) const;
	void simulate_comb_word(const id2boolmap& inputs, id2boolmap& simMap) const;
	bool simulate_node(id aid, const id2boolmap& simMap) const;

	void sig_prob(uint num_patterns);

	void print_ports() const;
	void write_bench(string filename = "") const;
	void write_verilog(string filename = "") const;
	void write_aig(const string& filename) const;
	void write_aig(std::ostream& out = std::cout) const;

	void read_aig(const string& filename);

	void static convert_to_and2(circuit& cir);

protected:
	id create_new_and(alit f0, alit f1, const string& nm);
	void remove_strashes(id aid);
	void remove_surrounding_strashes(id aid);
};

inline aigNode& aig_t::getNode(id aid) {
	try { return nodeMap.at(aid); }
	catch (std::out_of_range& e) {
		throw std::out_of_range( fmt("node %d not in aig nodemap: %s", aid % e.what()) );
	}
}
inline const aigNode& aig_t::getcNode(id aid) const {
	try { return nodeMap.at(aid); }
	catch (std::out_of_range& e) {
		throw std::out_of_range( fmt("node %d not in aig nodemap: %s", aid % e.what()) );
	}
}

// global helpers
inline aigNode get_mapped_aignode (const aigNode& nd, const id2alitmap& alitmp) {
	return aigNode(alitmp.at(nd.nid).lid(),
			nd.cm0() ^ alitmp.at(nd.fi0()),
			nd.cm1() ^ alitmp.at(nd.fi1()),
			nd.ndtype);
}

} // namespace aig


#endif /* SRC_AIG_AIG_H_ */
