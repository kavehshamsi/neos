/*
 * sat.h
 *
 *  Created on: Jul 9, 2018
 *      Author: kaveh
 */

#ifndef SRC_SAT_SAT_H_
#define SRC_SAT_SAT_H_

#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_set>
#include <set>

//#define SAT_SOLVER_MINISAT
//#define SAT_SOLVER_CRYPTOMINISAT
#define SAT_SOLVER_GLUCOSE

#if defined(SAT_SOLVER_CRYPTOMINISAT)
#include "cryptominisat/build/include/cryptominisat5/cryptominisat.h"
#elif defined(SAT_SOLVER_MINISAT)
#include "minisat/simp/SimpSolver.h"
#elif defined(SAT_SOLVER_GLUCOSE)
#include "glucose/simp/SimpSolver.h"
#endif

#ifdef SAT_SOLVER_MINISAT

namespace sat {
// wrapper class for different SAT-solvers

typedef Minisat::Lit slit;
typedef std::vector<slit> slitvec;
typedef std::unordered_set<slit> slitset;
typedef std::set<slit> slitoset;
using Minisat::lbool;

namespace SolverNs = Minisat;

#define sl_True  0
#define sl_False 1
#define sl_Undef 2

}

// specialized hash function for slits
namespace std {
  template <> struct hash<sat::slit>
  {
	std::size_t operator()(const sat::slit& s) const {
		return std::hash<int>{}(s.x);
	}
  };
}

namespace sat {

class sat_solver {
private:
	slit _true_lit = Minisat::toLit(-1);

	bool use_simp = false;

public:
	Minisat::SimpSolver S;

	sat_solver(bool use_simp = false);
	sat_solver& operator=(const sat_solver& Sr);

	void clear(int new_use_simp = -1);
	slit create_new_var(bool setfrozen = false);
	void add_clause(slit var);
	void add_clause(slit var1, slit var2);
	void add_clause(slit var1, slit var2, slit var3);
	void add_clause(const std::vector<slit>& clause);
	inline slitvec get_unsat_assumps();
	inline slitset get_unsat_assumps_set();
	int solve();
	int solve(slit assump);
	int solve(slit assump1, slit assump2);
	void simplify();
	void set_simp(bool use_simp);
	void setFrozen(slit p, bool val = true);
	void setDecVar(slit p, bool val = true);
	int solve(const std::vector<slit>& assumps);
	int solve(const std::vector<slit>& assumps, slit assump1);
	int solve_limited(const std::vector<slit>& assumps, int64_t propBudget);
	inline slit true_lit();
	inline slit false_lit();
	bool get_value(slit) const;
	void reseed_solver();
	void write_dimacs(std::string filename, const std::vector<slit>& assumps);
	void print_stats();
	uint64_t num_vars() { return S.nVars(); }
	uint64_t num_clauses() { return S.nClauses(); }

protected:
	static void to_mvec(const std::vector<slit>& slitvec, Minisat::vec<slit>& mvec);

};


inline slit operator|(slit c, bool b) {
	slit p = c;
	p.x = (b == 0 ? (p.x & ~1):(p.x | 1) );
	return p;
}

inline bool litsgn(slit c) {
	return Minisat::sign(c);
}

inline slitvec operator~(const slitvec& c) {
	slitvec ret;
	for (auto s : c)
		ret.push_back(~s);
	return ret;
}

inline std::vector<slit> sat_solver::get_unsat_assumps() {
	std::vector<slit> retvec;
	for (int i = 0; i < S.conflict.size(); i++) {
		retvec.push_back(S.conflict[i]);
	}
	return retvec;
}

inline slitset sat_solver::get_unsat_assumps_set() {
	slitset retset;
	for (int i = 0; i < S.conflict.size(); i++) {
		retset.insert(S.conflict[i]);
	}
	return retset;
}

inline slit sat_solver::true_lit() {
	if (_true_lit.x == -1) {
		_true_lit = create_new_var();
		setFrozen(_true_lit);
		add_clause(_true_lit);
	}
	return _true_lit;
}
inline slit sat_solver::false_lit() {
	return ~true_lit();
}

#elif defined(SAT_SOLVER_CRYPTOMINISAT)

typedef CMSat::Lit slit;
using CMSat::lbool;

#define sl_True  0
#define sl_False 1
#define sl_Undef 2

class sat_solver {
public:

	CMSat::SATSolver S;
	uint64_t nvars = 0;
	uint64_t nclauses = 0;

	slit create_new_var();
	void add_clause(slit var);
	void add_clause(slit var1, slit var2);
	void add_clause(slit var1, slit var2, slit var3);
	void add_clause(const std::vector<slit>& clause);
	int solve();
	int solve(slit assump);
	int solve(slit assump1, slit assump2);
	int solve(const std::vector<slit>& assumps);
	void simplify();
	int solve_limited(const std::vector<slit>& assumps, int propBudget);
	bool get_value(slit) const;
	void reseed_solver();
	void write_dimacs(std::string filename, const std::vector<slit>& assumps);

	uint64_t num_vars() { return S.nVars(); }
	uint64_t num_clauses() { return nclauses; }

	void print_stats();
};

#elif defined(SAT_SOLVER_GLUCOSE)

namespace sat {
// glucose has a sweet copy constructor for cloning
// minisat doesn't clone. Cryptominisat I feel clones the wrong way
// TODO:lingeling is another cloning option and is a great solver. add support

typedef Glucose::Lit slit;
typedef std::vector<slit> slitvec;
typedef std::unordered_set<slit> slitset;
typedef std::set<slit> slitoset;
using Glucose::lbool;

namespace SolverNs = Glucose;

#define sl_True  0
#define sl_False 1
#define sl_Undef 2

} // namespace sat

// specialized hash function for slits
namespace std {
  template <> struct hash<sat::slit>
  {
	std::size_t operator()(const sat::slit& s) const {
		return std::hash<int>{}(s.x);
	}
  };
}

namespace sat {


class sat_solver {
private:
	slit _true_lit = Glucose::toLit(-1);

	bool use_simp = false;

public:
	Glucose::SimpSolver S;

	sat_solver(bool use_simp = false);
	sat_solver& operator=(const sat_solver& Sr);

	void clear(int new_use_simp = -1);
	slit create_new_var(bool setfrozen = false);
	void add_clause(slit var);
	void add_clause(slit var1, slit var2);
	void add_clause(slit var1, slit var2, slit var3);
	void add_clause(const std::vector<slit>& clause);
	inline slitvec get_unsat_assumps();
	inline slitset get_unsat_assumps_set();
	int solve();
	int solve(slit assump);
	int solve(slit assump1, slit assump2);
	void simplify();
	void set_simp(bool use_simp);
	void setFrozen(slit p, bool val = true);
	void setDecVar(slit p, bool val = true);
	int solve(const std::vector<slit>& assumps);
	int solve(const std::vector<slit>& assumps, slit assump1);
	int solve_limited(const std::vector<slit>& assumps, int64_t propBudget);
	int solve_limited_prog(const std::vector<slit> assumps, int64_t startpropBudget,
			int64_t maxpropBudget, float growth, bool verbose = false);
	inline slit true_lit();
	inline slit false_lit();
	bool get_value(slit) const;
	void reseed_solver();
	void write_dimacs(std::string filename, const std::vector<slit>& assumps);
	void print_stats();
	uint64_t num_vars() { return S.nVars(); }
	uint64_t num_clauses() { return S.nClauses(); }

	static int var(slit p);
	static bool sign(slit p);

protected:
	static void to_mvec(const std::vector<slit>& slitvec, Glucose::vec<slit>& mvec);

};


inline slit operator|(slit c, bool b) {
	slit p = c;
	p.x = (b == 0 ? (p.x & ~1):(p.x | 1) );
	return p;
}

inline bool litsgn(slit c) {
	return Glucose::sign(c);
}

inline slitvec operator~(const slitvec& c) {
	slitvec ret;
	for (auto s : c)
		ret.push_back(~s);
	return ret;
}

inline std::vector<slit> sat_solver::get_unsat_assumps() {
	std::vector<slit> retvec;
	for (int i = 0; i < S.conflict.size(); i++) {
		retvec.push_back(S.conflict[i]);
	}
	return retvec;
}

inline slitset sat_solver::get_unsat_assumps_set() {
	slitset retset;
	for (int i = 0; i < S.conflict.size(); i++) {
		retset.insert(S.conflict[i]);
	}
	return retset;
}

inline slit sat_solver::true_lit() {
	if (_true_lit.x == -1) {
		_true_lit = create_new_var();
		setFrozen(_true_lit);
		add_clause(_true_lit);
	}
	return _true_lit;
}
inline slit sat_solver::false_lit() {
	return ~true_lit();
}

#endif

} // namespace sat


#endif /* SRC_SAT_SAT_H_ */
