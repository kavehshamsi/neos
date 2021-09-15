

#include <utl/utility.h>
#include "parser/bench/readbench.h"
#include "bench_parser.tab.hh"

namespace prs {
namespace bench {

readbench::readbench()
  : trace_parsing(false), trace_scanning(false) {}

int readbench::parse_file(const std::string &filename) {
	//auto tm_parse = utl::_start_wall_timer();
	file = filename;
	location.initialize(&file);
	FILE* fp = fopen(filename.c_str(), "r");
	scan_begin(fp);
	bb::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();

	//std::cout << "parse time: " << utl::_stop_wall_timer(tm_parse) << "\n";
	return res;
}

int readbench::parse_string(const std::string &f) {
	file = "";
	FILE* fp = fmemopen((void*)f.c_str(), f.size(), "r");
	scan_begin(fp);
	bb::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	return res;
}

using namespace prs::bench::ast;

void readbench::to_circuit(ckt::circuit& cir) {
	using namespace ckt;

	class bench_ast_visitor : boost::static_visitor<> {
	public:
		circuit* cir = nullptr;

		std::map<string, id> name_map;
		std::map<string, oidset> words;

		inline void resolve_type(wtype& wt, const string& nm) {
			if (nm.substr(0, 8) == "keyinput" && wt != wtype::INTER) {
				wt = wtype::KEY;
			}
		}

		id get_wire(const string& wn) {
			// take care of constants
			if (wn == "gnd")
				return cir->get_const0();
			if (wn == "vdd")
				return cir->get_const1();

			id wid = cir->add_wire_necessary(wn, wtype::INTER);
			return wid;
		}

		void operator()(const input_decl& x) {
			wtype wt = wtype::IN;
			resolve_type(wt, x.in_name);
			// std::cout << "adding wire " << x.in_name << "\n";
			cir->add_wire(x.in_name, wt);
		}

		void operator()(const output_decl& x) {
			cir->add_wire(x.out_name, wtype::OUT);
		}

		void operator()(const gate_decl& gda) {
			idvec fanins;

			for (auto& y : gda.in_names) {
				fanins.push_back(get_wire(y));
			}
			id fanout = get_wire(gda.out_name);

			cir->add_gate(fanins, fanout, funct::string_to_enum(gda.fun_name));
		}

		void operator()(const assign_decl& asd) {
			idvec fanins = { get_wire(asd.rhs) };
			id fanout = get_wire(asd.lhs);
			cir->add_gate(fanins, fanout, fnct::BUF);
		}

	} visitor;
	visitor.cir = &cir;

	for (auto& x : bast) {
		boost::apply_visitor(visitor, x);
	}

}

} // namespace bench
} // namespace prs

