/*
 * neosat.h
 *
 *  Created on: Jun 24, 2021
 *      Author: kaveh
 */

#ifndef SRC_SAT_NEOSAT_H_
#define SRC_SAT_NEOSAT_H_

#include <vector>
#include <string>
#include <set>
#include <list>
#include "utl/idmap.h"

namespace neosat {

#define sl_True 1
#define sl_False 0
#define sl_Undef 2

typedef int64_t vii;

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

	bool toBool() const {
		assert(val != X);
		return (bool)val;
	}

	friend std::ostream& operator<<(std::ostream& out, tern_t t);
};

inline std::ostream& operator<<(std::ostream& out, tern_t t) {
	switch(t.val) {
	case tern_t::X:
		out << "x";
		break;
	case tern_t::ONE:
		out << "1";
		break;
	case tern_t::ZERO:
		out << "0";
		break;
	default:
		abort();
		break;
	}
	return out;
}



struct slit {
	vii x = -1;
	slit() {}
	slit(vii i, bool sign = false) : x((i << 1) | (vii)sign) {}
	slit(const slit& b) : x(b.x) {}
	vii ind() const { return x >> 1; }
	bool sgn() const { return x & 1; }
	slit operator~() const { slit r; r.x = x; r.x ^= 1; return r; }
	bool operator==(slit b) { return x == b.x; }
	bool operator!=(slit b) { return x != b.x; }
	friend std::ostream& operator<<(std::ostream& out, slit a);
};

inline slit operator^(slit a, bool b) { slit r; r.x = a.x ^ b; return r; }

const slit lit_Undef = { -2 };  // }- Useful special constants.
const slit lit_Error = { -1 };  // }

typedef std::vector<slit> slitvec;
typedef std::set<slit> slitset;

class NeoSat {

protected:

	typedef int dmetric; // decision metric
	typedef int64_t cii; // clause index type in clause database
	typedef int64_t cvi; // variable index type in clause

	struct Clause {
		slitvec lits;
		cvi fc; // free count
		std::array<slit, 2> watches;

		cii size() { return lits.size(); }
		slit& operator[](cvi i) { return lits[i]; }

		std::string to_string();
	};

	typedef std::vector<Clause> ClauseVec;
	typedef std::pair<cii, cvi> ClauseLitRef;
	typedef ClauseLitRef Watcher;
	typedef std::list<Watcher> WatchList;
	typedef std::vector<slit> AssignTrail;

	const static cii cr_NoConf = -1;

	// reasons for variable assignment
	const static cii cr_NotAssigned = -2; // not assigned yet
	const static cii cr_DecReason = -3; // picked as decision variable

	AssignTrail trail;
	std::vector<vii> trail_lim;
	vii qhead = 0;
	std::map<dmetric, slit> varque;
	vii curdecvar;

	vii num_var = 0;
	cii  num_claus = 0;
	vii dec_level = 0;
	vii num_assigned_var = 0;

	struct VarData {
		bool alive = true;
		bool decision_var = true;
		int assign_count = 0;
		vii declevel = -1;
		cii reason = cr_NotAssigned; // reason for assignment
		tern_t cur_assingment = tern_t::X;
		WatchList watches;
	};

	utl::idmap<VarData, 1, vii> vardata_map;
	utl::idmap<Clause, 1, cii> clause_map;
	utl::idmap<bool, 1, vii> model;

public:
	NeoSat() {}

	void clear(int new_use_simp = -1);
	slit create_new_var();

	void add_clause(slit l1);
	void add_clause(slit l1, slit l2);
	void add_clause(slit l1, slit l2, slit l3);
	void add_clause(const std::vector<slit>& clause);
//	inline slitvec get_unsat_assumps();
//	inline slitset get_unsat_assumps_set();
//	int solve();
//	int solve(slit assump);
//	int solve(slit assump1, slit assump2);
//	void simplify();
//	void set_simp(bool use_simp);
//	void setFrozen(slit p, bool val = true);
//	void setDecVar(slit p, bool val = true);
//	int solve(const std::vector<slit>& assumps);
//	int solve(const std::vector<slit>& assumps, slit assump1);
//	int solve_limited(const std::vector<slit>& assumps, int64_t propBudget);
//	int solve_limited_prog(const std::vector<slit> assumps, int64_t startpropBudget,
//			int64_t maxpropBudget, float growth, bool verbose = false);
//	inline slit true_lit();
//	inline slit false_lit();
//	bool get_value(slit) const;
//	void reseed_solver();
//	void write_dimacs(std::string filename, const std::vector<slit>& assumps);
//	void print_stats();
	vii num_vars() { return vardata_map.size(); }
	cii  num_clauses() { return clause_map.size(); }

	int solve() { return solve_(); }

	std::string to_string();

protected:

	enum status_t {STATUS_FRESH, STATUS_UNSAT, STATUS_SAT, STATUS_INTER} status = STATUS_FRESH;

	VarData& getVarData(vii v);
	WatchList& varWatches(slit lt);
	void assignVar(slit a, cii reas = cr_DecReason);
	void unassignVar(slit a);
	cii propagate();
	void pickvar();
	bool dequeAssignment(slit& assgn);
	bool getlastAssignment(slit& assgn);
	void enquepAssign(slit p, cvi reason);
	vii decisionLevel();
	vii newDecisionLevel();
	void analyze(cii ret);
	vii numAssigned();
	int solve_();
	int search();
	tern_t value(slit lt);
	cii reason(slit lt);
	void print_state();
	void print_trail();

};


} // namespace neosat

#endif /* SRC_SAT_NEOSAT_H_ */
