/*
 * neosat.cpp
 *
 *  Created on: Jun 24, 2021
 *      Author: kaveh
 */

#include "neosat.h"
#include "utl/utility.h"
#include <iomanip>

namespace neosat {

slit NeoSat::create_new_var() {

	vii v = vardata_map.create_new_entry();
	return slit(v, false);

}

void NeoSat::add_clause(slit l1) {
	add_clause({l1});
}

void NeoSat::add_clause(slit l1, slit l2) {
	add_clause({l1, l2});
}

void NeoSat::add_clause(slit l1, slit l2, slit l3) {
	add_clause({l1, l2, l3});
}

tern_t NeoSat::value(slit lt) {
	return getVarData(lt.ind()).cur_assingment ^ lt.sgn();
}

NeoSat::cii NeoSat::reason(slit lt) {
	return getVarData(lt.ind()).reason;
}

std::string NeoSat::to_string() {
	std::string ret;
	for (auto& p : clause_map) {
		ret += "( ";
		for (auto x : p.second.lits) {
			ret += (x.sgn() ? "~x":"x") + std::to_string(x.ind()) + " ";
		}
		ret += ") \n";
	}
	return ret;
}

std::ostream& operator<<(std::ostream& out, slit a) {
	out << (std::string(a.sgn() ? "~x":"x") + std::to_string(a.ind()));
	return out;
}

std::string NeoSat::Clause::to_string() {
	return "(" + utl::to_delstr(lits, " + ") + ")";
}

void NeoSat::add_clause(const std::vector<slit>& clause) {

	std::cout << "adding clause (" << utl::to_delstr(clause, " + ") << ")\n";

	std::vector<slit> tokeep;
	for (cvi i = 0; i < clause.size(); i++) {
		auto curval = value(clause[i]);
		if (curval == tern_t::ONE) {
			return; // clause is true. no need to add
		}
		else if (curval != tern_t::ZERO) {
			tokeep.push_back(clause[i]);
		}
	}

	if (tokeep.size() == 0) {
		// unsat clause added
		status = STATUS_UNSAT;
		return;
	}

	// otherwise add clause to database
	cii clind = clause_map.create_new_entry();
	for (cvi i = 0; i < tokeep.size(); i++) {
		if (i == 0) {
			getVarData(tokeep[i].ind()).watches.push_back({clind, 0});
		}
		else if (i == clause.size() - 1) {
			getVarData(tokeep[i].ind()).watches.push_back({clind, clause.size() - 1});
		}
		else {
			// to make sure the variable exists
			auto& vd = getVarData(tokeep[i].ind());
		}
	}

	auto& cl = clause_map.at(clind);
	cl.watches = {tokeep[0], tokeep[tokeep.size() - 1]};
	cl.lits = tokeep;
	std::cout << "(" << utl::to_delstr(tokeep, " + ") << ")\n";
	cl.fc = tokeep.size();

}



NeoSat::VarData& NeoSat::getVarData(vii v) {

	try {
		return vardata_map.at(v);
	}
	catch (std::out_of_range&) {
		throw std::out_of_range("variable index "+ std::to_string(v) + " does not exist");
	}

}

NeoSat::WatchList& NeoSat::varWatches(slit lt) {
	return getVarData(lt.ind()).watches;
}

void NeoSat::assignVar(slit a, cii reas) {
	std::cout << "assigning x" << a.ind() << "=" << !a.sgn() << "\n";
	auto& vd = getVarData(a.ind());
	auto& ca = vd.cur_assingment;
	if (ca == tern_t::X) {
		num_assigned_var++;
	}
	vd.reason = reas;
	ca = !a.sgn();
}

void NeoSat::unassignVar(slit a) {
	std::cout << "unassiging " << a.ind() << "\n";
	auto& vd = getVarData(a.ind());
	auto& ca = vd.cur_assingment;
	if (ca != tern_t::X) {
		num_assigned_var--;
	}
	ca = tern_t::X;
	vd.reason = cr_NotAssigned;
}

bool NeoSat::dequeAssignment(slit& p) {
	if (trail.empty()) {
		return false;
	}
	p = trail.back();
	trail.pop_back();
	return true;
}

bool NeoSat::getlastAssignment(slit& p) {
	if (trail.empty()) {
		return false;
	}
	p = trail.back();
	return true;
}

// enque and assign
void NeoSat::enquepAssign(slit p, cvi reason) {
	trail.push_back(~p);
	assignVar(~p, reason);
}

NeoSat::cii NeoSat::propagate() {

	qhead = MAX(0, trail.size() - 1);

	while (true) {

		if (qhead >= trail.size()) {
			std::cout << "all facts at lvl " << decisionLevel() << " propagated \n";
			return cr_NoConf;
		}

		slit agn = trail[qhead++];

		auto& wcl = varWatches(agn);
		for (auto it = wcl.begin(); it != wcl.end(); it++) {

			cii clx = it->first;
			auto& cl = clause_map.at(clx);
			std::cout << "checking clause " << clx << " : " << cl.to_string() << "\n";

			bool zind = (cl.watches[1].ind() == agn.ind());
			slit l0 = cl.watches[zind]; // the currently changing lit
			slit l1 = cl.watches[zind ^ 1]; // unaffected lit
			std::cout << "watches: " << l0 << " " << l1 << "\n";

			if (value(l0) != tern_t::ZERO) {
				std::cout << "clause is sat. skipping\n";
				// clause is true. skip it.
				continue;
			}

			std::cout << "first watch at " << l0 << " is zero. looking for replacement.\n";

			bool found_new = false;
			for (cvi i = 0; i < cl.size(); i++) {
				if (cl[i] != l0 && cl[i] != l1 && value(cl[i]) != tern_t::ZERO) {
					// found new lit to watch
					it = wcl.erase(it);
					slit nl0 = cl[i];
					std::cout << "moving watch to " << i << " " << nl0 << "\n";
					getVarData(nl0.ind()).watches.push_back({clx, i});
					cl.watches[zind] = cl[i];
					found_new = true;
					break;
				}
			}
			if (found_new) {
				continue;
			}

			// no new watch found, clause is unit
			std::cout << "no new watch found. unit clause \n";
			//assert(value(l1) == tern_t::ONE);
			enquepAssign(~l1, clx);

		}
	}

}

vii NeoSat::decisionLevel() {
	return trail_lim.size();
}

vii NeoSat::newDecisionLevel() {
	trail_lim.push_back(trail.size());
	return trail.size();
}

vii NeoSat::numAssigned() {
	return num_assigned_var;
}

void NeoSat::print_state() {
	for (auto& p : vardata_map) {
		std::cout << "x" << p.first;
		slit lt(p.first);
		if (value(lt) != tern_t::X) {
			std::cout << "=" << value(lt);
		}
		std::cout << "  ";
	}
	std::cout << "\n";
}

void NeoSat::print_trail() {

	std::vector<vii> pos(trail.size(), -1);

	for (vii i = 0; i < trail_lim.size(); i++) {
		pos[trail_lim[i]] = i;
	}

	std::cout << "trail:";
	for (vii i = 0; i < trail.size(); i++) {
		std::cout << std::left << std::setw(5) << trail[i];
	}
	std::cout << "\nlims :";
	for (vii i = 0; i < trail.size(); i++) {
		std::cout << std::left << std::setw(5) << (pos[i] == -1 ? "-":std::to_string(i));
	}
	std::cout << "\n";

}

void NeoSat::analyze(cii ret) {

	auto& cl = clause_map.at(ret);
	std::cout << "analyzing conflict at " << ret << ":" << cl.to_string() << "\n";
	print_state();
	slitvec ban_clause;
	for (cii i = 0; i < cl.size(); i++) {
		cii rc = reason(cl[i]);
		std::cout << rc << "->" << cl[i] << "=" << value(cl[i]) << "\n";
		if (rc == cr_DecReason) { // decision variable rason
			std::cout << "  is a decision var. ban its state\n";
			assert(value(cl[i]) != tern_t::X);
			ban_clause.push_back( cl[i] ^ value(cl[i]).toBool() );
		}
		else {
			auto& rcl = clause_map.at(rc);
			std::cout << "  going to reason clause. " << rcl.to_string() << ": ";
			for (cvi j = 0; j < rcl.size(); j++) {
				if (rcl[j].ind() != cl[j].ind() && value(rcl[j]) != tern_t::X) {
					std::cout << " " << (rcl[j] ^ value(rcl[j]).toBool()) << "\n";
					ban_clause.push_back(rcl[j] ^ value(rcl[j]).toBool());
				}
			}
		}
	}
	std::cout << "\n";
	add_clause(ban_clause);

	// backtrack to last decision level
	std::cout << "backtracking\n";
	std::cout << utl::to_delstr(trail_lim, " ") << "\n";
	std::cout << utl::to_delstr(trail, " ") << "\n";

	print_trail();

	for (vii i = trail.size() - 1 ; i != trail_lim.back() - 1; i--) {
		unassignVar(trail[i]);
	}

	trail.resize(trail_lim.back());
	trail_lim.pop_back();

	std::cout << "post backtrack\n";
	print_trail();
	print_state();

}

int NeoSat::solve_() {

	search();

	if (status == STATUS_SAT) {
		std::cout << "model:\n";
		for (auto& p : vardata_map) {
			model[p.first] = value(p.first).toInt();
			std::cout << p.first << ":" << value(p.first) << "\n";
		}
		std::cout << "\n";
	}

}

int NeoSat::search() {

	int prop = 0;
	while (prop++ < 50) {

		std::cout << "decision level is " << decisionLevel() << "\n";
		print_state();

		cii ret = propagate();
		std::cout << "done with propagation. decision level is " << decisionLevel() << "\n";

		if (ret == cr_NoConf) {
			if (numAssigned() == num_vars()) {
				std::cout << "all variables assigned. SAT.\n";
				status = STATUS_SAT;
				goto done_search;
			}
			else { // have to pick a new variable
				std::cout << "picking new decision var\n";
				vii lvl = newDecisionLevel();
				pickvar();
				enquepAssign(slit(curdecvar, 0), cr_DecReason);
			}
		}
		else {
			std::cout << "found conflict\n";
			if (numAssigned() == 0) {
				std::cout << "conflict at level 0. UNSAT.\n";
				status = STATUS_UNSAT;
				goto done_search;
			}

			analyze(ret);
		}

	}

	done_search:
	std::cout << "done with search\n";

	return 0;
}

void NeoSat::pickvar() {
	for (auto& p : vardata_map) {
		if (value(p.first) == tern_t::X) {
			curdecvar = p.first; // polarity is always zero
			break;
		}
	}
}

} // namespace neosat



