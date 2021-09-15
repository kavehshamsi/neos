/*
 * circuit.hpp
 *
 *  Created on: Feb 7, 2016
 *      Author: kaveh
 */
#ifndef CIRCUIT_H_
#define CIRCUIT_H_

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
//#include <list>
#include <stack>
#include <stdint.h>
#include <stdlib.h>
#include <queue>
#include <any>

#include "utl/utility.h"
#include "utl/bimap.h"
#include "utl/idmap.h"
#include "base/cell_lib.h"
#include "base/base.h"

#include "base/attchm.h"

// I hate forward declarations
namespace prs {
namespace bench {
class bench_ast;
}
namespace verilog {
class verilog_ast;
}
}

namespace ckt {

const string ELEMENT_DIR = "bench/util_ckts/";
const string TEST_DIR = "bench/testing/";
const string BENCH_DIR = "bench/";

#define MAX_CHARS_PER_LINE  900
#define MAX_TOKENS_PER_LINE  400
#define BIT0 0x01

#define OPT_NONE 0b0000
#define OPT_KEY 0b0001
#define OPT_INP 0b0010
#define OPT_OUT 0b0100
#define OPT_INT 0b1000
//const char* const DELIMITER = " ";

/*
 *  usefull macros for iterating over circuits
 *  the whole thing is probably a bad idea but helps simplify syntax
 */
#include <boost/preprocessor.hpp>

#define EMIT_DEC_(R,D,DEC) \
    for(DEC; !_macro_k; )


#define FOR(DECS, COND, INC) \
    if(bool _macro_k = false) ; else \
      BOOST_PP_SEQ_FOR_EACH(EMIT_DEC_, DECS, DECS) \
        for(_macro_k = true; COND; INC)

// range loop with a few initialized variables inside the scope
#define RANGE_INIT_FOR(RANGE, INITS, INCS) \
    if(bool _macro_e = true)\
	  for ( RANGE )\
	  	  if(bool _macro_k = false) ; else \
	  	  if (_macro_e == false) break;\
	  	  else BOOST_PP_SEQ_FOR_EACH(EMIT_DEC_, INITS, INITS) \
			  for(_macro_k = true, _macro_e = false; _macro_k && !_macro_e; _macro_e = true, BOOST_PP_SEQ_ENUM(INCS))

#define foreach_gate(ckt, g, gid) \
	RANGE_INIT_FOR(const auto& _macro_p : (ckt).getcgates(),\
	(id gid = _macro_p.first)( const auto& g = _macro_p.second ), ((void)gid)((void)g))

#define foreach_instance(ckt, g, gid) \
	RANGE_INIT_FOR(const auto& _macro_p : (ckt).getcinstances(),\
	(id gid = _macro_p.first)( const auto& g = _macro_p.second ), ((void)gid)((void)g))


#define foreach_gate_ordered(ckt, g, gid) \
	RANGE_INIT_FOR(auto gid : (ckt).get_topsort_gates(), (const auto& g = (ckt).getcgate(gid)), ((void)gid)((void)g))

#define foreach_wire(ckt, w, wid) \
	RANGE_INIT_FOR(const auto& _macro_p : (ckt).getcwires(),\
	(id wid = _macro_p.first)( const auto& w = _macro_p.second ), ((void)wid)((void)w))

#define foreach_wire_ordered(ckt, w, wid) \
	RANGE_INIT_FOR(auto wid : (ckt).get_topsort_wires(), (const auto& w = (ckt).getcgate(wid)), ((void)wid)((void)w))

#define foreach_keyid(ckt, kid) for (auto kid : (ckt).keys())
#define foreach_inid(ckt, xid) for (auto xid : (ckt).inputs())
#define foreach_outid(ckt, yid) for (auto yid : (ckt).outputs())

#define foreach_latch(ckt, dffid, dffout, dffin) \
	RANGE_INIT_FOR(auto dffid : (ckt).latches(), (id dffout = (ckt).gfanout(dffid))(id dffin = (ckt).gfanin0(dffid)), ((void)dffid)((void)dffout)((void)dffin) )

#define foreach_vecindex(vec, i) for(size_t i = 0; i < vec.size(); i++)


//#define CAMOU
#define ASSIGN // for using assign in verilog instead of the mux gate

using utl::bimap;
using utl::idmap;

// static reference for default reference arguments. I hate this
static idset def_idset;
static idvec def_idvec;

enum class wtype {
	REMOVED = 0,
	IN = 1,
	OUT = 2,
	INTER = 3,
	KEY = 4
};

enum class nltype {
	WIRE = 0,
	GATE = 1,
	INST = 2
};

class nlnode { // netlist node unified object
public:
	id nid;
	bool markA : 1, markB : 1;
	unsigned int markC : 2;
	idvec fanouts;
	idvec fanins;
	nltype ncat0; // wire gate instance
	int8_t ncat1; // input/key/.. or function type
	id level = -1;
	attchm_mgr_t attch;

public:
	// constructors
	nlnode() : nid(-1), markA(0), markB(0), markC(2), ncat0(nltype::GATE), ncat1(0) {}
	nlnode(nltype nt) : nid(-1), markA(0), markB(0), markC(2), ncat0(nt), ncat1(0) {}
	nlnode(id nid, nltype nt, const idvec& fanins, id fanout, int8_t ncat1) :
		nid(nid), markA(0), markB(0), markC(2),
		fanouts({fanout}), fanins(fanins), ncat0(nt), ncat1(ncat1) {}
	nlnode(id nid, nltype nt, const idvec& fanins, const idvec& fanouts, int8_t ncat1) :
		nid(nid), markA(0), markB(0), markC(2),
		fanouts(fanouts), fanins(fanins), ncat0(nt), ncat1(ncat1) {}

	// wire interface functions
	id Id(void) const { return nid; }

	const idvec& fi() const { return fanins; }
	const idvec& fo() const { return fanouts; }
	id fi0() const { return fanins[0]; }
	id fo0() const { return fanouts[0]; }
	id& fo0() { return fanouts[0]; }

	nltype ndtype(void) const { return ncat0; }
	wtype wrtype(void) const { return (wtype)ncat1; }
	idvec wfanin(void) const { return fanins; }
	idvec wfanout(void) const{ return fanouts; }
	void setwtype(wtype wt) { ncat1 = (int8_t)wt; }

	void addfanin(id x) { fanins.push_back(x); }
	void addfanout(id x) { fanouts.push_back(x); }
	template<typename cont>
	void addfanouts(const cont& C) {
		for (auto x : C)
			addfanout(x);
	}
	void setfanout(id x) { fanouts = {x}; }
	void removefanin(id x) { utl::erase_byval(fanins, x); }
	void removefanout(id x) { utl::erase_byval(fanouts, x); }
	void removeNthfanin(id x) { utl::erase_byindex(fanins, x); }
	void removeNthfanout(id x) { utl::erase_byindex(fanouts, x); }

	bool isGate() const { return ncat0 == nltype::GATE; }
	bool isWire() const { return ncat0 == nltype::WIRE; }
	bool isInst() const { return ncat0 == nltype::INST; }

	bool isInput() const { return ncat1 == (int8_t)wtype::IN; }
	bool isOutput() const { return ncat1 == (int8_t)wtype::OUT; }
	bool isInter() const { return ncat1 == (int8_t)wtype::INTER; }
	bool isKey() const { return ncat1 == (int8_t)wtype::KEY; }
	bool isGood() const { return ncat1 != 0; }

	void setlevel(id lvl) const { const_cast<nlnode*>(this)->level = lvl; }

	// gate interface functions
	id getid() const { return nid; }
	const vector<id>& gfanin(void) const { return fanins; }
	id gfanin0() const { return fanins[0]; }
	id gfanout() const { return fanouts[0]; }
	fnct gfunction() const { return (fnct)ncat1; }
	fnct gfun() const { return (fnct)ncat1; }

	void setgfun(fnct fn) { ncat1 = (int8_t)fn; }
	bool isLatch() const { return ncat1 == (int8_t)fnct::DFF; }
	void offset_gate(id offset) {
		nid += offset;
		for (auto&& x : fanins)
			x += offset;
		fanouts[0] += offset;
	}
};

typedef nlnode gate;
typedef nlnode wire;


// I hate forward declarations
class circuit;


/**
 * provides mappings for string to function and vice versa
 * implementation can be found in funct.cpp
 * Also supports instances and submodules for hierarchical netlists
 * TODO: Parse Liberty and link
 * TODO: This whole class needs to go go go
 */
class funct {

private:

public:

	// map for available instances
	std::map<id, string> gid2instname;
	std::map<id, bimap<string, id> > gid2pinmap;
	id instcount;

	funct() : instcount(0) {}

	bool addstdcell(const string& cellname, id gateid,
			Omap<string, id>& pin2Net);
	bool read_cell_list(const char* stdlib);

	void updateWire(id old_wr, id new_wr, id x);

	string get_cell_name(id gateid) const;
	string inst_pinname(id gid, id wid) const;

	static string enum_to_string(fnct fnid);
	static fnct string_to_enum (const string& str);

	static void simplify_gate(circuit& ckt, id gid,
			id2boolmap& valmap, id known_wire = -1);

	// modifies ckt
	void convert_stdcell2generic(circuit& ckt);
};

// class for turnery simulations
class tern_t {
public:
	enum tern_val : int8_t {ZERO = 0, ONE = 1, X = 2};
private:
	tern_val val;
public:
	tern_t() : val(X) {}
	tern_t(tern_val val) : val(val) {}
	tern_t(int v) : val((tern_val)v) {}

	tern_t operator^(tern_t p) const {
		if (val == X || p.val == X)
			return tern_t(X);
		return tern_t(val ^ p.val);
	}

	tern_t operator&(tern_t p) const {
		if (val == ZERO || p.val == ZERO)
			return tern_t(ZERO);
		else if (val == ONE && p.val == ONE)
			return tern_t(ONE);
		return tern_t(X);
	}

	tern_t operator|(tern_t p) const {
		if (val == ONE || p.val == ONE)
			return tern_t(ONE);
		else if (val == ZERO && p.val == ZERO)
			return tern_t(ZERO);
		return tern_t(X);
	}

	static tern_t ite(tern_t s, tern_t i0, tern_t i1) {
		if (s.val != X) {
			return (s.val == ZERO) ? i0 : i1;
		}
		else {
			if (i0.val == ZERO && i1.val == ZERO)
				return tern_t(ZERO);
			else if (i0.val == ONE && i1.val == ONE)
				return tern_t(ONE);
			return tern_t(X);
		}
	}

	static tern_t tribuf(tern_t s, tern_t x) {
		return s.val == ONE ? x:tern_t(X);
	}

	bool operator==(tern_t t) const {
		return val == t.val;
	}

	bool operator!=(tern_t t) const {
		return val != t.val;
	}

	tern_t operator~() const {
		if (val == X)
			return tern_t(X);
		else
			return tern_t(val ^ 1);
	}

	int toInt() const {
		return (int)val;
	}

	friend std::ostream& operator<<(std::ostream& out, tern_t t);
};

// keeping tack of words
struct word_t {
	string base_name;
	idvec wids;
	int left = -1, right = -1;
};


typedef idmap<tern_t> id2ternmap;

/*
 * circuit class holds gate-level netlists
 */
class circuit {
	friend class nlnode;
	friend class cell_library;
	friend class prs::bench::bench_ast;
protected:

	// hash maps used for wires and gates
	idmap<nlnode, 3> nodes;

	// bimaps used for names <-> id
	typedef bimap<id, string, std::unordered_map> name_map_t;
	name_map_t node_names;

	idvec top_sort_g; // gate order
	idvec top_sort_w; // wire order

	// constant node ids
	id vddid = -1;
	id gndid = -1;

	// sets for PI, PO, and key inputs
	oidset prim_input_set;
	oidset prim_output_set;
	oidset key_input_set;

	// vector of latch-outputs
	oidset dffs;

	// circuit attachments. Can be (std::any)thing

	attchm_mgr_t attch;

	// words
	Omap<string, word_t> wordmap;

	bool topsort_needs_update : 1 = true;
	bool has_cell_lib : 1 = false;

public:
	// circuit name
	string top_name;

	// attachment to nlnodes for instance of cells
	struct cell_nlattachment {
		id cellid;
		idvec fanin_port_inds;
		idvec fanout_port_inds;
		attchm_mgr_t attch;
	};

	struct cir_cut_t {
		id root;
		oidset inputs;
		oidset gates;
	};

	circuit () {}

	circuit(const string& filename, char format = 'b');

	// getter methods for accessing private data
	uint nInst(void) const { return nodes.size((int8_t)nltype::INST); }
	uint nGates(void) const { return nodes.size((int8_t)nltype::GATE); }
	uint nOutputs(void) const { return prim_output_set.size(); }
	uint nInputs(void) const { return prim_input_set.size(); }
	uint nAllins(void) const { return nInputs() + nKeys(); }
	uint nInters(void) const { return nWires()
			- (outputs().size() + inputs().size() + keys().size()); }
	uint nWires(void) const { return nodes.size((int8_t)nltype::WIRE); }
	uint nKeys(void) const { return key_input_set.size(); }
	uint nLatches(void) const { return dffs.size(); }

	const oidset& inputs() const { return prim_input_set; }
	const oidset& outputs() const { return prim_output_set; }
	const oidset& keys() const { return key_input_set; }
	const oidset& latches() const { return dffs; }

	inline oidset dff_outs() const;
	inline oidset dff_ins() const;
	inline oidset allports() const;
	inline oidset allins() const;
	inline oidset combins() const;
	inline oidset combouts() const;
	inline idvec gates_and_instances() const;

	const idmap<gate, 3>& getcgates() const { return nodes.view((int8_t)nltype::GATE); }
	const idmap<wire, 3>& getcwires() const { return nodes.view((int8_t)nltype::WIRE); }
	const idmap<gate, 3>& getcinstances() const { return nodes.view((int8_t)nltype::INST); }

	inline const wire& getcwire(id wid) const;
	inline const gate& getcgate(id gid) const;

	// getting gates for modification
	inline wire& getwire(id wid);
	inline gate& getgate(id gid);
	inline nlnode& getnode(id nid) { return getwire(nid); }

	fnct gfunction(id gid) const { return (fnct)getcgate(gid).ncat1; }

	id gfanout(id gid) const { return getcgate(gid).fanouts[0]; }
	const idvec& gfanin(id gid) const { return getcgate(gid).fanins; }
	id gfanin0(id gid) const;

	const idvec& wfanins(id wid) const { return getcwire(wid).fanins; }
	id wfanin0(id wid) const;

	const idvec& wfanout(id wid) const { return getcwire(wid).fanouts; } ;
	id wfanout0(id wid) const;

	// latch related
	bool isLatch_input(id wid) const;
	bool isLatch_output(id wid) const { return _is_in(wfanin0(wid), dffs); }
	bool isLatch_port(id wid) const { return isLatch_input(wid) || isLatch_output(wid); }
	bool isLatch(id gid) const { return gfunction(gid) == fnct::DFF; }

	// more is methods
	bool wire_exists(id wid) const { return nodes.entry_exists(wid) && getcwire(wid).isWire(); }
	bool gate_exists(id gid) const { return nodes.entry_exists(gid) && getcgate(gid).isGate(); }
	bool inst_exists(id iid) const { return nodes.entry_exists(iid) && getcgate(iid).isInst(); }
	bool node_exists(id nid) const { return nodes.entry_exists(nid); }
	bool isInput(id wid) const { return getcwire(wid).isInput(); }
	bool isOutput(id wid) const { return getcwire(wid).isOutput(); }
	bool isKey(id wid) const { return getcwire(wid).isKey(); }
	bool isInter(id wid) const { return getcwire(wid).isInter(); }
	bool isGate(id gid) const { return getcgate(gid).isGate(); }
	bool isInst(id gid) const { return getcgate(gid).isInst(); }

	wtype getwtype(id wid) const { return getcwire(wid).wrtype(); }

	inline bool is_keygate(id gid) const;

	// constant nodes
	id get_const0();
	id get_const1();
	id get_cconst0() const;
	id get_cconst1() const;
	id is_const0(id wid) const;
	id is_const1(id wid) const;
	bool has_const1() const { return (vddid != -1); }
	bool has_const0() const { return (gndid != -1); }
	int isConst(id wid) const { return (wid == vddid) ? 2 : ((wid == gndid) ? 1 : 0); }
	bool isConst(id wid, bool cv) const { return cv ? wid == vddid : wid == gndid; }
	static void add_constants(idset& inset, const circuit& ckt);


	wtype wrtype(id x) const { return getcwire(x).wrtype(); }
	const string& ndname(id nid) const;
	const string& ndname(const nlnode& nd) const;
	const string& wname(id x) const;
	const string& wname(const wire& w) const;
	const string& gname(id g) const;
	const string& gname(const gate& g) const;

	string get_new_wname(id hint = -1) const;
	string get_new_gname(id hint = -1) const;
	string get_new_iname(id hint = -1) const;

	// random selects
	id get_random_gateid() const;
	id get_random_wireid(wtype wt = wtype::REMOVED) const;

	const string& getname(const nlnode& n) const { return node_names.at(n.nid); }

	template<typename C>
	string to_wstr(const C& cont, const string& delim = " ") const;
	template<typename C>
	string to_gstr(const C& cont, const string& delim = " ") const;

	// setter functions for writing to private data
	void setwiretype(id x, wtype wt);
	void setgatefun(id gid, fnct fun);
	void setwirename(id x, const string& str, bool force = true);
	void setwirename(id x); // assign next available name

	// tricky function to change id of node. Pushes old id onto another node
	void setnodeid(id oldid, id newid);
	void setnodeid_toabsentid(id oldid, id newid);

	// error checking
	void error_check_assert() const;
	bool error_check() const;

	// read-in and write methods
	bool read_verilog(std::istream& infin);
	bool read_verilog(const std::string& filename);
	void write_verilog(std::ostream& fout = std::cout) const;
	void write_verilog(const string& filename) const;

	bool read_bench(std::istream& infin);
	bool read_bench(const string& filename);
	void write_bench(std::ostream& fout = std::cout) const;
	void write_bench(const string& filename) const;


	bool reparse();
	void rename_wires();
	void rename_internal_wires();

	id get_max_wid() const { return nodes.max_id(); };
	id get_max_gid() const { return nodes.max_id(); };

	// element addition
	id add_wire(wtype wt, id wid = -1);
	id add_wire(const string& str, wtype wt, id wid = -1, bool const_check = true);
	id add_wire_necessary(const string& str, wtype wt);
	id add_wire(const string& str, const wire& wr);

	void register_wiretype(id wid, wtype wt);
	void unregister_wiretype(id wid, wtype wt);

	// word functions
	void add_word(const string& str, int low, int high, wtype wt);
	void add_word_necessary(const string& base, int low, int high, wtype wt);
	bool get_word(const string& base, word_t& wrd) const;

	// adding primitive gates
	id add_gate(const idvec& fanins, id fanout, fnct fun);
	id add_gate(const string& str, const gate& g);
	id add_gate(const string& str, const idvec& fanins,
			id fanout, fnct fun);
	idvec add_gate_word(const vector<idvec>& fanin_words,
				const idvec& fanout_word, fnct functionality);
	idvec add_gate_word(const vector<idvec>& fanin_words, fnct fun);

	// add gate and translate ports through map
	template<typename M>
	id add_gate(const gate& g, const M& wiremap);
	template<typename M>
	id add_gate(const string& str, const gate& g, const M& wiremap);
	template<typename M>
	id add_gate(const idvec& fanins, id fanout, fnct fun, const M& wiremap);
	template<typename M>
	id add_gate(const string& gnm, const idvec& fanins, id fanout, fnct fun, const M& wiremap);

	// cell-type nodes and libraries
	id add_instance(const string& inst_name, const string& inst_type, const vector<string>& net_names,
			const vector<string>& port_names = utl::empty<vector<string>>());
	id add_instance(const string& inst_name, const string& inst_type, const vector<id>& net_ids,
			const vector< string >& port_names = utl::empty<vector<string>>());
	id add_instance(const string& inst_name, id cellid, const idvec& net_ids);
	const cell_library::cell_obj& cget_cell(id cellid) const;
	const cell_library::cell_obj& cget_cell_of_inst(id iid, id& cellid) const;
	const string& inst_pin_name(id iid, id ind) const;
	void set_cell_library(cell_library& clib);
	const cell_library& cget_cell_library() const;
	bool has_cell_library() const { return has_cell_lib; }
	cell_library& get_cell_library();
	void resolve_cell_directions(const vector<string>& out_port_names); // any pin name in argument will be considered output
	const cell_nlattachment& cell_info_of_inst(id gid) const;
	cell_nlattachment& cell_info_of_inst(id gid);
	void translate_cells_to_primitive_byname();


	// critical addition and removal methods
	void add_circuit(const circuit& cir, id2idmap& added2new, const string& postfix = "");
	void add_circuit(char option, const circuit& cir, const string& postfix);
	void add_circuit(char option, const circuit& cir, id2idmap& ckt2newmap, const string& postfix);
	void add_circuit_byname(const circuit& cir2, const string& namePostfix = "");
	void add_circuit_byname(char option, const circuit& cir, const string& postfix);
	void add_circuit_byname(const circuit& cir2, const string& namePostfix,	id2idmap& cir2toNew_wmap);
	void remove_circuit(idvec gates_to_remove);
	void remove_kcut(const idset& kcut, const idset& fanins);
	void remove_wire(id w, bool update_gates = true);
	void remove_node(id nid, bool update_surroundings = true);
	void remove_gate(id gid, bool update_wires = true);
	void remove_instance(id gid, bool update_wires = true);
	void swap_wirenames(id w1, id w2);

	// opens wire at root of the wire
	idpair open_wire(id wr, const string& str = "");

	// opens wire at branch of the wire that is input of gate
	idpair open_wire_at_branch(id wr, id gid, const string& str = "");
	void merge_wires(id to_remain, id to_remove);
	void move_fanouts_to(id from, id to);
	void merge_equiv_nodes(id to_remain, id to_remove);
	void internal_merge_wires(id wr1, id wr2);
	void addWire_to_gateFanin(id gid, id wr);
	void remove_gatefanin(id gid, id wr);
	void update_gateFanin(id gid, id wr_old, id wr_new);
	id join_outputs(fnct fun, const string& owname);
	id join_outputs(const idvec& oids, fnct fun, const string& owname = "");

	// important DAG algorithms
	void get_gate_mffc(id rootgid, idset& mffc_gates) const;
	void enumerate_cuts(Hmap<id, vector<cir_cut_t> >& cuts, int k, int N, bool isolated = false) const ;
	void enumerate_cuts(id root, vector<cir_cut_t>& cuts, int k, int N, bool isolated = false) const ;
	bool find_kcut(id fout, uint num_ins, idset& kcut, idset& fanins, bool isolated) const;
	bool get_kcut(id root, uint N, idset& kcut_gset,
			idset& fanins, bool isolated, circuit* kcut_ckt = nullptr) const;
	bool get_kcut(id root, uint N, idset& kcut_gset,
			idset& fanins, idset& kcut_wset, bool isolated, circuit* kcut_ckt = nullptr) const;
	void get_kcut_topsrot(const idset& kcut_gset, const idset& fanins, idvec& sorted_gates);
	void get_kcut_truth_table(const idset& kcut_gset, const idset& fanins, boolvec& truth_table);
	void gateSet2Circuit(id id, circuit& cir,
			idset& kcut, idset& fanins);
	void get_layers(vector<idvec>& glayers, vector<idvec>& wlayers);
	void get_layers(vector<idvec>& glayers, vector<idvec>& wlayers, const idvec& start_wires);

	void get_trans_fanin (id wr, idset& faninwires,
			idset& faninpis = def_idset, idvec& faninvec = def_idvec) const;
	circuit get_fanin_cone(id wid) const;
	void get_trans_fanoutset (id wr, idset& fanoutset) const;
	void get_trans_fanoutset (const idset& source_set, idset& fanoutset) const;
	bool find_sccs(vector<idset>& all_sccs) const;
	idset get_dominator_gates() const;
	idvec find_wire_path(id swid, id dwid) const;
	id2idHmap find_feedback_arc_set() const;
	id2idHmap find_feedback_arc_set2() const;
	// access internal topsort vecs
	void clear_topsort() const;
	const idvec& get_topsort_gates() const;
	const idvec& get_topsort_wires() const;
	static void make_combinational(circuit& ckt);


	// through gate and through wire methods
	const idvec& gfanoutGates(id gateid) const;
	idset gfaninGates(id gateid) const;
	idset wfaninWires(id wr) const;
	idset wfanoutWires(id wr) const;

	bool wire_fanout_in_cone(idset& target, id wr, id sourceGate) const;

	id add_key(void);

	inline id find_wire(const string& str) const;
	inline id find_wcheck(const string& str) const;
	inline id find_gate(const string& str) const;
	inline id find_gcheck(const string& str) const;

	//circuit simulation
	template<typename M>
	static inline bool simulate_gate(const gate& gt, const M& id2valMap);
	bool simulate_gate(id gid, const id2boolmap& id2valMap) const;
	void simulate_comb(id2boolmap& simMap) const;
	void simulate_comb(const id2boolHmap& inputmap, id2boolmap& simMap) const;
	void simulate_inst(const gate& g, id2boolmap& simMap) const;
	/*static void simulate_comb(const id2boolHmap& inputmap,
			const id2boolHmap& keymap, id2boolmap& simMap);*/
	void simulate_comb_tern(id2ternmap& simMap) const;
	void simulate_seq(vector<id2boolmap>& simmaps) const;
	void simulate_seq(const vector<id2boolmap>& inputmaps, vector<id2boolmap>& simmaps) const;
	template<typename M, typename T = bool>
	void add_const_to_map(M& id2tmap, T c0 = false, T c1 = true) const {
		if (has_const0()) id2tmap[get_cconst0()] = c0;
		if (has_const1()) id2tmap[get_cconst1()] = c1;
	}
	template<typename T = bool>
	void add_const_to_vec(vector<T>& v1, T c0 = false, T c1 = true) const {
		if (has_const0()) v1.push_back(c0);
		if (has_const1()) v1.push_back(c1);
	}
	void fill_inputmap_random(id2boolmap& simmap, bool inputs = true, bool keys = true, bool latches = true);
	tern_t simulate_gate_tern(id gid, id2ternmap& valmap) const;
	tern_t simulate_gate_tern(const gate& g, id2ternmap& valmap) const;
	static tern_t simulate_gate_tern(const vector<tern_t>& invals, fnct fun);
	static void random_simulate(const circuit& cir, uint32_t num_patterns);

	//circuit simplification
	void remove_bufs();
	void remove_nots();
	void remove_dead_logic(bool remove_input = true);
	void trim_circuit_to(const idset& outs, bool remove_input = true);
	void simplify_wkey(const boolvec& key);
	void propagate_constants();
	void propagate_constants(id2boolHmap wire2val_map);
	void tie_to_constant(const id2boolHmap& consts_map);
	void tie_to_constant(id wid, bool v);
	void rewrite_gates();
	static void simplify_gate(const gate& g, gate& ng, const id2boolHmap& consts_map, int& output_val, bool& dead_gate);
	void convert2generic();
	void limit_fanin_size(int max_sz = 3);

	//for handling sle generated netlists (why do I have this??)
	void fix_output(void);

	void print_stats() const;


	/*
	 *  method for clearing all data from circuit
	 */
	void clear();

	void print_wireset(idset wrset) const;
	void print_ports() const;
	void print_wire(id wid) const;
	void print_gate(id gid) const;

protected:

	void _resolve_wire_array(string strtoken, wtype wt);

	idset _strongconnect(id v, id& index, id2idmap& gate2index,
			id2idmap& lowlink, idset& onstack,
			idvec& S, vector<idset>& all_sccs) const;

	void topsort(idvec& gate_ordering, idvec& wire_ordering) const;
	void topsort_wires(idvec& wire_ordering) const;
	void topsort_gates(idvec& gate_ordering) const;

};

// implementation of some inline methods
inline const wire& circuit::getcwire(id wid) const {
	return ((circuit*)this)->getwire(wid);
}

inline const gate& circuit::getcgate(id gid) const {
	return ((circuit*)this)->getgate(gid);
}


inline wire& circuit::getwire(id wid) {
	try {
		return nodes.at(wid);
	}
	catch(std::out_of_range& e) {
		neos_abort("wire id " << wid << " not found -> " << e.what());
	}
}

inline gate& circuit::getgate(id gid) {
	try {
		return nodes.at(gid);
	}
	catch (std::out_of_range& e) {
		neos_abort("gate id " << gid << " not found -> " << e.what());
	}
}

template<typename T>
inline string circuit::to_wstr(const T& cont, const string& delim) const {
	string rstr;
	int i = 0;
	for (auto wid : cont) {
		if (i++ != cont.size() - 1)
			rstr += wname(wid) + delim;
		else
			rstr += wname(wid);
	}
	return rstr;
}

template<typename T>
inline string circuit::to_gstr(const T& cont, const string& delim) const {
	string rstr;
	int i = 0;
	for (auto gid : cont) {
		if (i++ != cont.size() - 1)
			rstr += wname(gid) + delim;
		else
			rstr += wname(gid);
	}
	return rstr;
}

template<typename M>
inline id circuit::add_gate(const gate& g, const M& wiremap) {
	return add_gate("", g, wiremap);
}

template<typename M>
inline id circuit::add_gate(const string& str, const gate& g, const M& wiremap) {
	return add_gate(str, g.fanins, g.fo0(), g.gfun(), wiremap);
}

template<typename M>
inline id circuit::add_gate(const idvec& fanins, id fanout, fnct fun, const M& wiremap) {
	return add_gate("", fanins, fanout, fun, wiremap);
}

template<typename M>
inline id circuit::add_gate(const string& gnm, const idvec& fanins, id fanout, fnct fun, const M& wiremap) {

	idvec mp_fanins;
	id mp_fanout;
	try {
	mp_fanout = wiremap.at(fanout);
	for ( auto x : fanins ) {
		mp_fanins.push_back(wiremap.at(x));
	}
	}
	catch(std::out_of_range&) {
		neos_error( "not found net in wiremap \n");
	}

	return add_gate(gnm, mp_fanins, mp_fanout, fun);
}


template<typename M>
inline bool circuit::simulate_gate(const gate& gt, const M& id2valMap) {

	// meta programming magic
	static_assert(std::is_same<typename M::mapped_type, bool>::value);

	std::vector<bool> invals;

	for (auto gfan : gt.fanins) {
		try {
			invals.push_back(id2valMap.at(gfan));
		}
		catch(std::out_of_range&) {
			neos_abort("gate simulation: input missing from value-map for gate "
					<< gt.nid << " and input " << gfan << "\n");
		}
	}

	bool outVal;

	switch (gt.gfun()) {
	case fnct::AND: {
		outVal = invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal &= invals[i];
		break;
	}
	case fnct::NAND: {
		outVal = invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal &= invals[i];
		outVal = !outVal;
		break;
	}
	case fnct::OR: {
		outVal = invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal |= invals[i];
		break;
	}
	case fnct::NOR: {
		outVal = invals[0];
		for (size_t i = 1; i < invals.size(); i++)
			outVal |= invals[i];
		outVal = !outVal;
		break;
	}
	case fnct::XOR: {
		outVal = invals[0] ^ invals[1];
		break;
	}
	case fnct::XNOR: {
		outVal = invals[0] ^ invals[1];
		outVal = !outVal;
		break;
	}
	case fnct::MUX: {
		outVal = (!invals[0] & invals[1]) | (invals[0] & invals[2]);
		break;
	}
	case fnct::TMUX: {
		int num_ones = 0;
		for (uint i = 0; i < gt.fanins.size()/2; i++) {
			if (invals[i] == 1) {
				outVal = invals[i + gt.fanins.size()/2];
				num_ones++;
			}
		}
		assert(num_ones != 0);
		assert(num_ones == 1);
		break;
	}
	case fnct::NOT: {
		outVal = !invals[0];
		break;
	}
	case fnct::BUF: {
		outVal = invals[0];
		break;
	}
	default:
		neos_error("cannot simulate " << funct::enum_to_string(gt.gfun()) << " gate");
	}


	/*std::cout << outVal << "  =  " << funct::enum_to_string(gt.gfun()) << " (";
	for (auto val : invals) {
		std::cout << val << ", ";
	}
	std::cout << ");\n"; */



	return outVal;
}


inline id circuit::find_wire(const string& str) const {
	auto it = node_names.getImap().find(str);
	if (it != node_names.getImap().end())
		return it->second;
	else
		return -1;
}

inline id circuit::find_wcheck(const string& str) const {
	auto it = node_names.getImap().find(str);
	if (it != node_names.getImap().end())
		return it->second;
	else {
		std::cout << "wire name " << str << " not found" << "\n";
		assert(0);
		return -1;
	}
}

inline id circuit::find_gate(const string& str) const {
	auto it = node_names.getImap().find(str);
	if (it != node_names.getImap().end())
		return it->second;
	else {
		return -1;
	}
}


inline id circuit::find_gcheck(const string& str) const {
	auto it = node_names.getImap().find(str);
	if (it != node_names.getImap().end())
		return it->second;
	else {
		std::cout << "gate " << str << " not found" << "\n";
		assert(0);
		return -1;
	}
}

inline const std::string& circuit::ndname(id nid) const {
	auto it = node_names.getmap().find(nid);
	if (it != node_names.getmap().end())
		return it->second;

	return const_cast<name_map_t&>(node_names).add_pair(nid, get_new_wname(nid));
}

inline const std::string& circuit::wname(const wire& w) const {
	return wname(w.nid);
}

inline const std::string& circuit::gname(const gate& g) const {
	return gname(g.nid);
}

inline const std::string& circuit::wname(id wid) const {
	if (!wire_exists(wid))
		neos_abort("wire with id " << wid << " does not exist\n");

	auto it = node_names.getmap().find(wid);
	if (it != node_names.getmap().end())
		return it->second;

	return const_cast<name_map_t&>(node_names).add_pair(wid, get_new_wname(wid));
}

inline const std::string& circuit::gname(id gid) const {
	if (!gate_exists(gid))
		neos_abort("gate with id " << gid << " does not exist\n");

	auto it = node_names.getmap().find(gid);
	if (it != node_names.getmap().end())
		return it->second;

	return const_cast<name_map_t&>(node_names).add_pair(gid, get_new_gname(gid));
}

inline id circuit::gfanin0(id gid) const {
	return getcgate(gid).fanins[0];
}

inline id circuit::wfanin0(id wid) const {
	if (getcwire(wid).fanins.empty())
		return -1;
	return getcwire(wid).fanins[0];
}

inline oidset circuit::dff_outs() const {
	oidset ret;
	foreach_latch(*this, dffid, dffout, dffin) {
		ret.insert(dffout);
	}
	return ret;
}

inline oidset circuit::dff_ins() const {
	oidset ret;
	foreach_latch(*this, dffid, dffout, dffin) {
		ret.insert(dffin);
	}
	return ret;
}


inline oidset circuit::allports() const {
	oidset retset = allins();
	utl::add_all(retset, outputs());
	return retset;
}

inline oidset circuit::allins() const {
	oidset retset = inputs();
	utl::add_all(retset, keys());
	return retset;
}

inline oidset circuit::combins() const {
	oidset retset = allins();
	utl::add_all(retset, dff_outs());
	return retset;
}

inline oidset circuit::combouts() const {
	oidset retset = outputs();
	utl::add_all(retset, dff_ins());
	return retset;
}

inline idvec circuit::gates_and_instances() const {
	idvec retvec;
	foreach_gate(*this, g, gid) {
		retvec.push_back(gid);
	}
	foreach_instance(*this, g, gid) {
		retvec.push_back(gid);
	}
	return retvec;
}



inline id circuit::wfanout0(id wid) const {
	if (getcwire(wid).fanouts.empty())
		return -1;
	return getcwire(wid).fanouts[0];
}

inline bool circuit::isLatch_input(id wid) const {
	for (auto fanout : wfanout(wid))
		if (_is_in(fanout, dffs))
			return true;
	return false;
}

inline bool circuit::is_keygate(id gid) const {
	for (auto gin : gfanin(gid)) {
		if (isKey(gin)) {
			return true;
		}
	}
	return false;
}

inline id circuit::get_const1() {
	if (vddid == -1)
		vddid = add_wire("vdd", wtype::INTER, -1, false);
	return vddid;
}

inline id circuit::get_const0() {
	if (gndid == -1)
		gndid = add_wire("gnd", wtype::INTER, -1, false);
	return gndid;
}

inline id circuit::get_cconst1() const {
	assert(vddid != -1);
	return vddid;
}

inline id circuit::get_cconst0() const {
	assert(gndid != -1);
	return gndid;
}

inline id circuit::is_const0(id wid) const {
	return (wid == gndid);
}
inline id circuit::is_const1(id wid) const {
	return (wid == vddid);
}

/*
 * a visitor class for circuit graph traversals
 */
// simple usage macros
#define faninwire_visitor(ckt) visitor(ckt, visitor::strategy::BFS,\
		visitor::direction::BACK, visitor::nodes::WIRES)

#define faningate_visitor(ckt) visitor(ckt, visitor::strategy::BFS,\
		visitor::direction::BACK, visitor::nodes::GATES)

#define fanoutwire_visitor(ckt) visitor(ckt, visitor::strategy::BFS,\
		visitor::direction::FWD, visitor::nodes::WIRES)

#define fanoutgate_visitor(ckt) visitor(ckt, visitor::strategy::BFS,\
		visitor::direction::FWD, visitor::nodes::GATES)

#define foreach_visitor_elements(v, start_nd_or_ndset, nid) \
	for (id nid = v.start(start_nd_or_ndset); !v.finished(); nid = v.next())



class visitor {
public:
	enum strategy {DFS, BFS};
	enum nodes {WIRES, GATES};
	enum direction {BACK, FWD};

protected:
	id cur_node = -1;
	idvec stack;
	idque que;
	const circuit& ckt;
	strategy stg;
	direction dir;
	nodes nd;
	idset visited;
	bool finish = false;

public:

	visitor(const circuit& ckt, strategy stg, direction dir, nodes nd) :
		ckt(ckt), stg(stg), dir(dir), nd(nd) {};

	id start(id x);
	template<typename S>
	id start(const S& start_node_set) {
		cur_node = *start_node_set.begin();

		if (stg == BFS) {
			utl::push_all(que, start_node_set);
			utl::add_all(visited, start_node_set);
		}
		else {
			utl::push_all(que, start_node_set);
			utl::add_all(visited, start_node_set);
		}
		return cur_node;
	}
	id next();
	bool finished() { return finish; }

protected:
	void _visit(id node);
	id _pop_node();
	idset _neighbors(id node);

};




}

#endif /* CIRCUIT_H_ */
