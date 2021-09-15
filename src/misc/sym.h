/*
 * sym.h
 *
 *  Created on: Jan 11, 2021
 *      Author: kaveh
 *      Description: Symbolic engine for circuit-based SAT-based algorithms
 */

#ifndef SRC_SAT_SYM_H_
#define SRC_SAT_SYM_H_

#include "base/circuit.h"
#include "sat/c_sat.h"

namespace sym {

using namespace ckt;
using namespace csat;
using namespace sat;

class sym_mgr_t;
class sym_op_t;

class sym_var_t {
public:
	enum sym_var_type_t {
		BOOL,
		BOOLVEC
	};

	sym_mgr_t* mgr = nullptr;
	id varid;
	bool frozen : 1;

	string name; // basename
	idvec fanins;
	idvec fanouts;

	virtual sym_var_type_t vtype() = 0;
	virtual vector<string> enumerate_names() = 0;

	sym_mgr_t& getmgr() const { return *mgr; }

	void to_circuit(circuit& rcir, Hmap<id, idvec> v2wmap) const;
	void write_graph() const;

protected:
	void op_to_circuit(const sym_op_t& op, circuit& rcir,
			const Hmap<id, idvec>& v2wmap) const;
};

class sym_op_t {
public:

	enum attch_t {
		ATTCH_CIR,
		ATTCH_AIG
	};

	enum op_t {
		NOT = 0,

		AND = 1,
		OR = 2,
		XOR = 3,
		ITE = 4,
		ADD = 5,
		MUL = 6,

		CIR = 7,
		AIG = 8,

		EQ = 9,
		GR = 10,
		GEQ = 11,

		CONCAT = 12,
		SPLIT = 13
	} op : 4;

	id oid;
	string name;
	idvec fanins;
	idvec fanouts;

	Omap<attch_t, std::any> attchmap;

	static string op_enum2string(op_t ope) {
		switch (ope) {
		case NOT: return "NOT"; break;
		case AND: return "AND"; break;
		case OR: return "OR"; break;
		case XOR: return "XOR"; break;
		case ITE: return "ITE"; break;
		case ADD: return "ADD"; break;
		case MUL: return "MUL"; break;
		case CIR: return "CIR"; break;
		case AIG: return "AIG"; break;
		default: {
			neos_error("literaly not possible");
		}
		}
	}
};

class sym_var_bool_t : public sym_var_t {
public:
	slit satvar;
	sym_var_type_t vtype() { return sym_var_t::BOOL; }
	vector<string> enumerate_names() { return {name}; }
	virtual ~sym_var_bool_t() {}
};

sym_var_bool_t operator&(const sym_var_bool_t& a, const sym_var_bool_t& b);
sym_var_bool_t operator|(const sym_var_bool_t& a, const sym_var_bool_t& b);
sym_var_bool_t operator^(const sym_var_bool_t& a, const sym_var_bool_t& b);
sym_var_bool_t operator+(const sym_var_bool_t& a, const sym_var_bool_t& b);
sym_var_bool_t operator~(const sym_var_bool_t& a);

class sym_var_boolvec_t : public sym_var_t {
public:
	int num_bits = -1;
	sym_var_type_t vtype() { return sym_var_t::BOOLVEC; }
	vector<string> enumerate_names();
	virtual ~sym_var_boolvec_t() {}
};

sym_var_boolvec_t operator&(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_boolvec_t operator|(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_boolvec_t operator^(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_boolvec_t operator+(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_boolvec_t operator~(const sym_var_boolvec_t& a);

sym_var_bool_t operator==(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_bool_t operator!=(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_bool_t operator>=(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_bool_t operator<=(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_bool_t operator>(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);
sym_var_bool_t operator<(const sym_var_boolvec_t& a, const sym_var_boolvec_t& b);


id bool_apply_op(const vector<sym_var_bool_t>& ins, sym_var_bool_t& y, sym_op_t::op_t ope);
id boolvec_apply_op(const vector<sym_var_bool_t>& ins, sym_var_bool_t& y, sym_op_t::op_t ope);
id boolvec_apply_op(const vector<sym_var_boolvec_t>& ins, sym_var_bool_t& y, sym_op_t::op_t ope);

sym_var_boolvec_t& apply_cir(const sym_var_boolvec_t& x, const sym_var_boolvec_t& k, const circuit& cir);

class sym_mgr_t {
public:
	idmap<std::shared_ptr<sym_var_t>> sym_vars;
	idmap<sym_op_t> sym_ops;
	Hmap<string, id> name2op;
	Hmap<string, id> name2var;

public:
	sym_mgr_t() {}
	sym_var_bool_t& new_bool_variable(const string& nm = "", bool frozen = true);
	sym_var_boolvec_t& new_boolvec_variable(uint sz, const string& basenm = "", bool frozen = true);
	sym_op_t& new_operation(sym_op_t::op_t ope, const idvec& fanins,
			const idvec& fanouts, const string& opnm = "");

	static void get_gate_inputs(const sym_op_t& op, const Hmap<id, idvec>& v2wmap, vector<idvec>& gateins);
	static void buf_to_other_fanouts(circuit& rcir, const sym_op_t& op, const Hmap<id, idvec>& v2wmap);

	id find_var(const string& nm);
	id find_op(const string& nm);

	string new_var_name() const;
	string new_op_name() const;

	void set_varname(id varid, const string& nm);
	void set_opname(id opid, const string& nm);

	void remove_op(id opid);
	void remove_var(id opid);

	void flatten_cir_op(sym_op_t& op);

	template<typename T = sym_var_t>
	T& getvar(id vid) {
		try {
			auto ptr = std::dynamic_pointer_cast<T>(sym_vars.at(vid));
			if (!ptr)
				throw std::runtime_error( "variable with id " + std::to_string(vid) +
						" is not of type " + typeid(T).name() );
			return *ptr;
		}
		catch (std::out_of_range&) {
			neos_error("variable with " << vid << " not found");
		}
	}

	sym_op_t& getop(id oid) {
		return sym_ops.at(oid);
	}
};



} // namespace sym


#endif /* SRC_SAT_SYM_H_ */

