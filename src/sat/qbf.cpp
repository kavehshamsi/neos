/*
 * qbf.cpp
 *
 *  Created on: Jun 3, 2021
 *      Author: kaveh
 */

#include "sat/qbf.h"

namespace csat {

void cir_to_qdimacs(const circuit& cir, const vector<idvec>& quantlists, const vector<bool>& isuniv, string& qdstr) {

	assert(cir.nOutputs() == 1);

	string body;
	string preamble;

	int nclause = 0, nvar = 0;

	id2litmap_t varmap;

	sat_solver S;
	S.create_new_var();
	foreach_wire(cir, w, wid) {
		varmap[wid] = S.create_new_var();
		nvar++;
	}

	for (uint i = 0; i < quantlists.size(); i++) {
		body += isuniv[i] ? "a":"e";
		for (auto wid : quantlists[i]) {
			body += fmt(" %1%", sat_solver::var(varmap.at(wid)));
			/*else {
				body += fmt("e %1% 0\n", sat_solver::var(varmap.at(wid)));
			}*/
		}
		body += " 0 \n";
	}



	foreach_gate(cir, g, gid) {
		vector<slitvec> Cls;
		std::vector<slit> ins;
		for (auto gin : g.fanins) {
			ins.push_back(varmap.at(gin));
		}
		slit y = varmap.at(g.fo0());
		get_logic_clause(Cls, ins, y, g.gfun());

		nclause += Cls.size();

		for (auto& cl : Cls) {
			for (auto p : cl) {
				string sgn = sat_solver::sign(p) ? "-":"";
				body += sgn + std::to_string(sat_solver::var(p)) + " ";
			}
			body += "0 \n";
		}
	}

	id oid = *(cir.outputs().begin());
	body += fmt( "%1% 0 \n", std::to_string(sat_solver::var(varmap.at(oid))) );
	nclause++;

	preamble = fmt("p cnf %1% %2% \n", nvar % nclause);

	qdstr = preamble + body;

}

} // namespace csat
