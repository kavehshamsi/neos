

#include "circuit.h"

namespace ckt {
// TODO: this whole file needs a rewrite
// modifies ckt
void funct::convert_stdcell2generic(circuit& ckt) {

	foreach_instance(ckt, gobj, gid) {
		auto& g = ckt.getnode(gid);
		if (g.gfun() == fnct::INST) {
			string cellname = get_cell_name(g.nid);
			if (cellname.substr(0, 3) == "AND") {
				g.setgfun(fnct::AND);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "OR") {
				g.setgfun(fnct::OR);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "NOT") {
				g.setgfun(fnct::NOT);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "BUF") {
				g.setgfun(fnct::BUF);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "NOR") {
				g.setgfun(fnct::NOR);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "NAND") {
				g.setgfun(fnct::NAND);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "XOR") {
				g.setgfun(fnct::XOR);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "XNOR") {
				g.setgfun(fnct::XNOR);
				g.ncat0 = nltype::GATE;
			}
			else if (cellname.substr(0, 3) == "MUX") {
				g.setgfun(fnct::XNOR);
				g.ncat0 = nltype::GATE;
			}

		}
	}
}

void funct::simplify_gate(circuit& ckt, id gid,
		id2boolmap& valmap, id known_wire) {
	gate& g = ckt.getgate(gid);
	idvec fanins = ckt.gfanin(gid); // copy original fanin
	switch (g.gfun()) {
	case fnct::XNOR :
	case fnct::XOR : {
		assert(known_wire != -1);
		ckt.remove_wire(known_wire);
		g.setgfun(
				((valmap.at(known_wire) == 0)
						== (g.gfun() == fnct::XOR) )
						? fnct::NOT : fnct::BUF);
		utl::erase_byval(g.fanins, known_wire);
		valmap.erase(known_wire);
		break;
	}
	case fnct::MUX : {
		assert (g.fanins[0] == known_wire);
		ckt.remove_wire(known_wire);
		g.setgfun( fnct::BUF );
		utl::erase_byval(g.fanins,
				((valmap.at(known_wire) == 0)) ? g.fanins[1] : g.fanins[2]);
		valmap.erase(known_wire);
		break;
	}
	default: {
		std::cout << "simplify for gate type "
				<< enum_to_string(g.gfun()) << " not supported\n";
		exit(EXIT_FAILURE);
	}
	}
}


bool funct::addstdcell(const string& cellname, id gid, std::map<string, id>& pin2Net) {

	gid2instname[gid] = cellname;
	gid2pinmap[gid] = bimap<string, id>(pin2Net);
	instcount++;

	return true;
}

void funct::updateWire(id old_wr, id new_wr, id gid) {
	auto& pinmap = gid2pinmap.at(gid);
	for (auto& x : pinmap.getmap()) {
		if (x.second == old_wr) {
			pinmap.add_pair(x.first, new_wr);
		}
	}
}


string funct::inst_pinname(id gid, id wid) const {
	return gid2pinmap.at(gid).ta(wid);
}

string funct::get_cell_name(id gid) const {
	return gid2instname.at(gid);
}

bool funct::read_cell_list(const char* stdlib) {

	std::ifstream cellfile(stdlib);
	id cellid = 0;
	string cellname;

	while (cellfile >> cellname) {
		gid2instname[cellid] = cellname;
		cellid++;
	}

	return true;
}

string funct::enum_to_string(fnct fnID) {
	if (fnID == fnct::AND) {
		return "and";
	}
	else if (fnID == fnct::OR) {
		return "or";
	}
	else if (fnID == fnct::BUF) {
		return "buf";
	}
	else if (fnID == fnct::XOR) {
		return "xor";
	}
	else if (fnID == fnct::XNOR) {
		return "xnor";
	}
	else if (fnID == fnct::NAND) {
		return "nand";
	}
	else if (fnID == fnct::NOR) {
		return "nor";
	}
	else if (fnID == fnct::NOT) {
		return "not";
	}
	else if (fnID == fnct::TBUF) {
		return "tribuf";
	}
	else if (fnID == fnct::TMUX) {
		return "trimux";
	}
	else if (fnID == fnct::MUX) {
		return "mux";
	}
	else if (fnID == fnct::DFF) {
		return "dff";
	}
	assert(false);
	return "UNDEF";
}

fnct funct::string_to_enum (const string& str) {
	static Hmap<string, fnct> fun_map = {
			{"and", fnct::AND},
			{"AND", fnct::AND},
			{"nand", fnct::NAND},
			{"NAND", fnct::NAND},
			{"or", fnct::OR},
			{"OR", fnct::OR},
			{"nor", fnct::NOR},
			{"NOR", fnct::NOR},
			{"xor", fnct::XOR},
			{"XOR", fnct::XOR},
			{"xnor", fnct::XNOR},
			{"XNOR", fnct::XNOR},
			{"buf", fnct::BUF},
			{"buff", fnct::BUF},
			{"BUF", fnct::BUF},
			{"BUFF", fnct::BUF},
			{"not", fnct::NOT},
			{"NOT", fnct::NOT},
			{"mux", fnct::MUX},
			{"MUX", fnct::MUX},
			{"trimux", fnct::TMUX},
			{"TRIMUX", fnct::TMUX},
			{"dff", fnct::DFF},
			{"DFF", fnct::DFF}
	};

	auto it = fun_map.find(str);
	if (it != fun_map.end())
		return it->second;
	else
		neos_error(str << " is not a primitive gate");

/*	if (str == "and" || str == "AND") {
		return fnct::AND;
	} else if (str == "xor" || str == "XOR") {
		return fnct::XOR;
	} else if (str == "xnor" || str == "XNOR") {
		return fnct::XNOR;
	} else if (str == "not" || str == "NOT") {
		return fnct::NOT;
	} else if ( str == "nand" || str == "NAND") {
		return fnct::NAND;
	} else if (str == "nor" || str == "NOR") {
		return fnct::NOR;
	} else if (str == "or" || str == "OR") {
		return fnct::OR;
	} else if (str == "mux" || str == "MUX") {
		return fnct::MUX;
	} else if (str == "buf" || str == "BUF" || str == "BUFF") {
		return fnct::BUF;
	} else if (str == "tribuf" || str == "TRIBUF") {
		return fnct::TBUF;
	} else if (str == "trimux" || str == "TRIMUX") {
		return fnct::TMUX;
	} else if (str == "dff" || str == "DFF") {
		return fnct::DFF;
	} else {
		neos_error(str << " is not a primitive gate");
	}*/
}

}
