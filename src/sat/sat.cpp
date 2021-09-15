/*
 * sat.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: kaveh
 */


#include <utl/utility.h>
#include "sat/sat.h"

namespace sat {

/*
 * MiniSAT sat solver interface
 *
 */
#if defined(SAT_SOLVER_MINISAT)

sat_solver::sat_solver(bool simp) {
	use_simp = simp;

	S.use_elim = S.use_simplification = use_simp;

	S.budgetOff();
}

void sat_solver::set_simp(bool simp) {
	use_simp = simp;
	S.use_simplification = use_simp;
}

void sat_solver::clear(int new_use_simp) {
	auto old_use_simp = use_simp;
	S.~Solver();
	new (&S) Minisat::SimpSolver();

	use_simp = (new_use_simp != -1) ? new_use_simp:old_use_simp;

	_true_lit = Minisat::toLit(-1);
	S.use_elim = S.use_simplification = use_simp;
}

sat_solver& sat_solver::operator=(const sat_solver& st) {
	//S.~SimpSolver();
	new (&S) Minisat::SimpSolver(st.S);
	S.use_elim = S.use_simplification = use_simp = st.use_simp;

	_true_lit = st._true_lit;

	return *this;
}

slit sat_solver::create_new_var(bool setfrozen) {
	slit p = Minisat::mkLit(S.newVar());
	if (setfrozen) setFrozen(p);
	return p;
}

void sat_solver::add_clause(slit clause) {
	S.addClause(clause);
}

void sat_solver::add_clause(slit var1, slit var2) {
	S.addClause(var1, var2);
}

void sat_solver::add_clause(slit var1, slit var2, slit var3) {
	S.addClause(var1, var2, var3);
}

void sat_solver::add_clause(const std::vector<slit>& clause) {
	Minisat::vec<slit> mclause;
	for (auto sl : clause)
		mclause.push(sl);

	S.addClause(mclause);
}

int sat_solver::solve() {
	return S.solve();
}

int sat_solver::solve_limited(const std::vector<slit>& assumps, int64_t propBudget) {

	using namespace Minisat;

	S.budgetOff();
	if (propBudget != -1) {
		S.setPropBudget(propBudget);
	}

	vec<slit> massumps;
	to_mvec(assumps, massumps);

	Minisat::lbool retbool = S.solveLimited(massumps);
	int status = toInt(retbool);

	S.budgetOff();

	return status;
}


int sat_solver::solve(slit assump1, slit assump2) {
	return S.solve(assump1, assump2);
}

int sat_solver::solve(slit assump) {
	return S.solve(assump);
}

int sat_solver::solve(const std::vector<slit>& assumps) {
	Minisat::vec<slit> massumps;

	for (auto sl : assumps) {
		massumps.push(sl);
	}

	return S.solve(massumps);
}


int sat_solver::solve(const std::vector<slit>& assumps, slit assump1) {
	Minisat::vec<slit> massumps;

	for (auto sl : assumps) {
		massumps.push(sl);
	}
	massumps.push(assump1);

	return S.solve(massumps);
}


void sat_solver::simplify() {
	assert(S.use_elim);
	S.simplify();
}

void sat_solver::setFrozen(slit p, bool frozen) {
	S.setFrozen(Minisat::var(p), frozen);
}

void sat_solver::setDecVar(slit p, bool decvar) {
	S.setDecisionVar(Minisat::var(p), decvar);
}

bool sat_solver::get_value(slit p) const {
	return Minisat::toInt(S.modelValue(p)) == 0;
}

void sat_solver::reseed_solver() {

	auto d = static_cast<uint64_t>(
			std::chrono::high_resolution_clock::now().time_since_epoch().count());
	uint64_t d_d = (0x0ffff) & d;
	S.random_seed = (double)d_d;
	//S.rnd_pol = true;
	S.rnd_init_act = true;
}

void sat_solver::write_dimacs(std::string filename, const std::vector<slit>& assumps) {

	FILE* fp = fopen(filename.c_str(), "w");
	S.toDimacs(filename.c_str());
	fclose(fp);
}

void sat_solver::to_mvec(const std::vector<slit>& slitvec, Minisat::vec<slit>& mvec) {
	for (auto sl : slitvec)
		mvec.push(sl);
}

void sat_solver::print_stats() {
	std::cout << "variables: " << S.nVars() << " clauses: " << S.nClauses() << " assignments: " << S.nAssigns() << "  propagations: " << S.propagations << "\n";
}

/*
 * Glucose SAT solver interface
 *
 *
 */
#elif defined(SAT_SOLVER_GLUCOSE)

sat_solver::sat_solver(bool simp) {
	use_simp = simp;

	S.use_elim = S.use_simplification = use_simp;

	S.budgetOff();
	S.setIncrementalMode();
}

void sat_solver::set_simp(bool simp) {
	use_simp = simp;
	S.use_simplification = use_simp;
}

void sat_solver::clear(int new_use_simp) {
	auto old_use_simp = use_simp;
	S.~Solver();
	new (&S) Glucose::SimpSolver();

	use_simp = (new_use_simp != -1) ? new_use_simp:old_use_simp;

	_true_lit = Glucose::toLit(-1);
	S.setIncrementalMode();
	S.use_elim = S.use_simplification = use_simp;
}

sat_solver& sat_solver::operator=(const sat_solver& st) {
	//S.~SimpSolver();
	new (&S) Glucose::SimpSolver(st.S);
	S.use_elim = S.use_simplification = use_simp = st.use_simp;

	_true_lit = st._true_lit;
	S.setIncrementalMode();
	return *this;
}

slit sat_solver::create_new_var(bool setfrozen) {
	slit p = Glucose::mkLit(S.newVar());
	if (setfrozen) setFrozen(p);
	return p;
}

void sat_solver::add_clause(slit clause) {
	S.addClause(clause);
}

void sat_solver::add_clause(slit var1, slit var2) {
	S.addClause(var1, var2);
}

void sat_solver::add_clause(slit var1, slit var2, slit var3) {
	S.addClause(var1, var2, var3);
}

void sat_solver::add_clause(const std::vector<slit>& clause) {
	Glucose::vec<slit> mclause;
	for (auto sl : clause)
		mclause.push(sl);

	S.addClause(mclause);
}

int sat_solver::solve() {
	return S.solve();
}

int sat_solver::solve_limited_prog(const std::vector<slit> assumps,
		int64_t startpropBudget, int64_t maxpropBudget, float growth, bool verbose) {

	assert(growth > 1);

	int status = 0;
	while (true) {
		if (verbose) {
			std::cout << "solving with proplimit: " << startpropBudget;
		}
		status = solve_limited(assumps, startpropBudget);
		if (verbose) {
			std::cout << " -> " << status << "\n";
		}
		if (status == sl_Undef) {
			startpropBudget *= growth;
			if (maxpropBudget != -1 && startpropBudget > maxpropBudget) {
				if (verbose)
					std::cout << "exceeding proplimit of " << maxpropBudget << "\n";
				return sl_Undef;
			}
		}
		else {
			return status;
		}
	}

}


int sat_solver::solve_limited(const std::vector<slit>& assumps, int64_t propBudget) {

	using namespace Glucose;

	S.budgetOff();
	if (propBudget != -1) {
		S.setPropBudget(propBudget);
	}

/*
	auto tm = utl::_start_wall_timer();


	int64_t props = S.propagations;
	int64_t conflicts = S.conflicts;
	int64_t restarts = S.curr_restarts;
*/

	vec<slit> massumps;
	to_mvec(assumps, massumps);

	Glucose::lbool retbool = S.solveLimited(massumps);
	int status = toInt(retbool);

/*
	std::cout << "SAT stats: " << (S.propagations - props) << "  "
			<< (S.conflicts - conflicts) << "  " << (S.curr_restarts - restarts) << "  -> " << utl::_stop_wall_timer(tm) << "\n";
*/

	S.budgetOff();

	return status;
}


int sat_solver::solve(slit assump1, slit assump2) {
	return S.solve(assump1, assump2);
}

int sat_solver::solve(slit assump) {
	return S.solve(assump);
}

int sat_solver::solve(const std::vector<slit>& assumps) {
	Glucose::vec<slit> massumps;

	for (auto sl : assumps) {
		massumps.push(sl);
	}

	return S.solve(massumps);
}


int sat_solver::solve(const std::vector<slit>& assumps, slit assump1) {
	Glucose::vec<slit> massumps;

	for (auto sl : assumps) {
		massumps.push(sl);
	}
	massumps.push(assump1);

	return S.solve(massumps);
}


void sat_solver::simplify() {
	assert(S.use_elim);
	S.simplify();
}

void sat_solver::setFrozen(slit p, bool frozen) {
	S.setFrozen(Glucose::var(p), frozen);
}

void sat_solver::setDecVar(slit p, bool decvar) {
	S.setDecisionVar(Glucose::var(p), decvar);
}

bool sat_solver::get_value(slit p) const {
	return Glucose::toInt(S.modelValue(p)) == 0;
}

void sat_solver::reseed_solver() {

	auto d = static_cast<uint64_t>(
			std::chrono::high_resolution_clock::now().time_since_epoch().count());
	uint64_t d_d = (0x0ffff) & d;
	S.random_seed = (double)d_d;
	//S.rnd_pol = true;
	S.rnd_init_act = true;
}

void sat_solver::write_dimacs(std::string filename, const std::vector<slit>& assumps) {

	FILE* fp = fopen(filename.c_str(), "w");
	S.toDimacs(filename.c_str());
	fclose(fp);
}

void sat_solver::to_mvec(const std::vector<slit>& slitvec, Glucose::vec<slit>& mvec) {
	for (auto sl : slitvec)
		mvec.push(sl);
}

int sat_solver::var(slit p) {
	return Glucose::var(p);
}

bool sat_solver::sign(slit p) {
	return Glucose::sign(p);
}

void sat_solver::print_stats() {
	std::cout << "variables: " << S.nVars() << " clauses: " << S.nClauses() << " assignments: " << S.nAssigns() << "  propagations: " << S.propagations << "\n";
}
#endif

} // namespace sat

