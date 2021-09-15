

#include "parser/verilog/readverilog.h"
#include "parser/verilog/verilog_parser.tab.hh"

namespace prs {
namespace verilog {

readverilog::readverilog()
  : trace_parsing(false), trace_scanning(false) {}

int readverilog::parse_file(const std::string &f) {
	file = f;
	location.initialize(&file);
	trace_parsing = 0;
	trace_scanning = 0;
	FILE* fp = fopen(f.c_str(), "r");
	scan_begin(fp);
	vv::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	return res;
}

int readverilog::parse_string(const std::string &f) {
	file = "";
	trace_parsing = 0;
	trace_scanning = 0;
	FILE* fp = fmemopen((void*)f.c_str(), f.size(), "r");
	scan_begin(fp);
	vv::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	return res;
}

using namespace ckt;
using namespace prs::verilog::ast;

class verilog_statement_visitor : boost::static_visitor<> {
public:
	circuit* cir = nullptr;

	std::map<string, id> name_map;
	std::map<string, oidset> words;

	inline void resolve_key_type(wtype& wt, const string& nm) {
		if (nm.substr(0, 8) == "keyinput" && wt != wtype::INTER) {
			wt = wtype::KEY;
		}
	}

	inline void error_wire_not_found(const string& wnm) {
		std::cout << "netlist error: wire " << wnm << " not found\n";
		exit(EXIT_FAILURE);
	}

	id find_wire_wcheck(const string& nm) {
		id wid = cir->find_wire(nm);
		if (wid == -1) {
			if (nm == "vdd")
				return cir->get_const1();
			else if (nm == "gnd")
				return cir->get_const0();
			neos_error("netlist error: can't find wire " << nm << "\n");
		}
		return wid;
	}


	void operator()(const net_decl_stmt& x) {
		wtype wt = wtype::INTER;
		if (x.netdir == "input") {
			wt = wtype::IN;
		}
		else if (x.netdir == "output") {
			wt = wtype::OUT;
		}

		if (x.mvars.r.right == x.mvars.r.left && x.mvars.r.left == -1) {
			for (auto& y : x.mvars.var_names) {
				resolve_key_type(wt, y);
				//std::cout << "adding wire " << y <<" " << cir->nWires() << "\n";
				cir->add_wire_necessary(y, wt);
			}
		}
		else {
			for (auto& y : x.mvars.var_names) {
				int left = x.mvars.r.left;
				int right = x.mvars.r.right;
				auto wwt = wt;
				resolve_key_type(wwt, y);
				// std::cout << fmt("%s %d %d %d\n", y % low % high % (int)wwt);
				cir->add_word_necessary(y, left, right, wwt);
				// cir->print_stats();
			}
		}

	}

	void expand_expr(const constant_expr& e, vector<string>& net_names) {
		if ( const bin_constant *p = boost::get<bin_constant>(&(e)) )  {
			//std::cout << "bin const: " << p->num_bits << " " << p->values << "\n";
			for (int i = 0; i < p->values.size(); i++) {
				net_names.push_back(p->values[i] == '1' ? "vdd":"gnd");
			}
		}
		else if ( const hex_constant *p = boost::get<hex_constant>(&(e)) )  {
			//std::cout << "hex constant\n";
			boolvec values;
			//std::cout << "nb: " << p->num_bits << "\n";
			//std::cout << "nd: " << p->digits << "\n";
			uint num_letters = p->digits.size();
			for (uint i = 0; i < num_letters; i++) {
				int val = utl::hex_to_int(p->digits[i]);
				for (uint j = 0; j < 8; j++) {
					values.push_back( (val>>j) == 1 );
				}
			}
			values.resize(p->num_bits);
			//std::cout << utl::to_delstr(values) << "\n";
            //std::reverse(values.begin(), values.end());
			for (auto b : values) {
				net_names.push_back(b ? "vdd":"gnd");
			}
		}
		else if ( const dec_constant *p = boost::get<dec_constant>(&(e)) )  {
			//std::cout << "dec constant\n";
			boolvec values;
			//std::cout << "nb: " << p->num_bits << "\n";
			//std::cout << "nd: " << p->digits << "\n";
			for (uint i = 0; i < p->digits.size(); i++) {
				int c = p->digits[i] - '0';
				for (uint j = 0; j < 8; j++) {
					values.push_back( (c >> j) == 1 );
				}
			}
			values.resize(p->num_bits);
            //std::reverse(values.begin(), values.end());
			//std::cout << utl::to_delstr(values) << "\n";
			for (auto b : values) {
				net_names.push_back(b ? "vdd":"gnd");
			}
		}
		else if ( const string *p = boost::get<string>(&(e)) )  {
			neos_error("string constants not supported in gate-level verilog\n");
		}
		else if ( const integer *p = boost::get<integer>(&(e)) )  {
			neos_error("integer constants may be ambiguous in gate-level verilog\n");
		}
		else {
			neos_abort("no way");
		}
	}

	void expand_expr(const expr& e, vector<string>& net_names) {
		if ( const ranged_expr *p = boost::get<ranged_expr>(&(e.v)) ) {
			for (int i = p->r.left; ; i += (p->r.left > p->r.right) ? -1:1) {
				net_names.push_back(fmt("\\%s[%d]", p->basename % i));
				if (i == p->r.right)
					break;
			}
		}
		else if ( const indexed_expr *p = boost::get<indexed_expr>(&(e.v)) ) {
			net_names.push_back(fmt("\\%s[%d]", p->basename % p->index));
		}
		else if ( const concat_expr *p = boost::get<concat_expr>(&(e.v)) ) {
			for (auto& x : p->vl) {
				expand_expr(x, net_names);
			}
		}
		else if ( const bare_expr *p = boost::get<bare_expr>(&(e.v)) )  {
			word_t wrd;
			if (cir->get_word(*p, wrd)) {
				for (int i = wrd.left; ; i += (wrd.left > wrd.right) ? -1:1) {
					net_names.push_back(fmt("\\%s[%d]", *p % i));
					if (i == wrd.right)
						break;
				}
			}
			else {
				net_names.push_back(*p);
			}
		}
		else if ( const constant_expr *p = boost::get<constant_expr>(&(e.v)) )  {
			expand_expr(*p, net_names);
		}
		else {
			neos_abort("net variant problem");
		}
	}


	void operator()(const prim_gate_stmt& x) {
		//std::cout << "adding gate\n";
		idvec fanins;

		vector<string> fanin_names;
		for (auto& nt : x.in_nets) {
			expand_expr(nt, fanin_names);
		}

		for (auto& y : fanin_names) {
			id wid = cir->find_wire(y);
			if (wid == -1) {
				if (y == "gnd")
					wid = cir->get_const0();
				else if (y == "vdd")
					wid = cir->get_const1();
				else
					error_wire_not_found(y);
			}
			fanins.push_back(wid);
		}

		vector<string> fanout_names;
		expand_expr(x.out_net, fanout_names);
		if (fanout_names.size() > 1) {
			neos_error("netlist error: primitive gate cannot drive multiple net : "
					<< utl::to_delstr(fanout_names, ", "));
		}

		id fanout = find_wire_wcheck(fanout_names[0]);

		cir->add_gate(fanins, fanout, funct::string_to_enum(x.fun_name));
	}

	void operator()(const assignment_stmt& x) {
		vector<string> rhs_names;
		vector<string> lhs_names;

		expand_expr(x.rhs, rhs_names);
		expand_expr(x.lhs, lhs_names);

		if (rhs_names.size() != lhs_names.size()) {
			std::cout << "netlist_error: assignment right-hand-side does not match lef-hand-side";
			exit(EXIT_FAILURE);
		}

		for (uint i = 0; i < rhs_names.size(); i++) {
			std::cout << "assigning " << rhs_names[i] << " to " << lhs_names[i] << "\n";
			id rhs_wid = find_wire_wcheck(rhs_names[i]);
			id lhs_wid = find_wire_wcheck(lhs_names[i]);

			cir->add_gate({rhs_wid}, lhs_wid, fnct::BUF);
		}
	}

	void operator()(const instance_stmt& x) {
		id instid = -1;
		if ( const conn_pair_list *p = boost::get<conn_pair_list>(&(x.ports)) ) {
			vector<string> net_names_all, port_names_all;
			for (const auto& conn_pair : *p) {
				vector<string> net_names, port_names;
				expand_expr(conn_pair.second, net_names);
				if (net_names.size() > 1) {
					for (uint i = 0; i < net_names.size(); i++) {
						port_names.push_back( fmt("%s_%d_", conn_pair.first % i) );
					}
				}
				else {
					port_names = {conn_pair.first};
				}
				utl::push_all(net_names_all, net_names);
				utl::push_all(port_names_all, port_names);
			}
			for (uint i = 0; i < net_names_all.size(); i++) {
				//std::cout << net_names_all[i] << " -> " << port_names_all[i] << "\n";
			}
			instid = cir->add_instance(x.instname, x.insttype, net_names_all, port_names_all);
		}
		else if ( const expr_list *p = boost::get<expr_list>(&(x.ports)) ) {
			vector<string> net_names_all;
			for (const auto& net : *p) {
				vector<string> net_names;
				expand_expr(net, net_names);
				utl::push_all(net_names_all, net_names);
			}
			instid = cir->add_instance(x.instname, x.insttype, net_names_all);
		}
		assert(instid != -1);

		if (x.parameters.which() != 0) {
			//std::cout << "instance has parameters \n";
			if ( const expr_list *p = boost::get<expr_list>(&(x.parameters)) ) {
				//std::cout << "parameters are expr list\n";
			}
			else if ( const conn_pair_list *p = boost::get<conn_pair_list>(&(x.parameters)) ) {
				for (auto& cpair : *p) {
					cir->cell_info_of_inst(instid).attch.add_attachment(cpair.first, cpair.second);
				}
			}
		}

	}
};

void readverilog::to_circuit(ckt::circuit& cir) {
	using namespace ckt;

	assert(vast.size() == 1);

	std::cout << "looking at module\n";
	std::cout << "statements: " << vast[0].sts.size() << "\n";
	verilog_statement_visitor visitor;
	visitor.cir = &cir;
	for (auto& y : vast[0].sts)
		boost::apply_visitor(visitor, y);

}

} // namespace verilog
} // namespace prs

