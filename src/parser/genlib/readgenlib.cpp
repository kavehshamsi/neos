

#include "parser/genlib/readgenlib.h"
#include "genlib_parser.tab.hh"
#include "base/circuit.h"
#include "parser/parser.h"

namespace prs {
namespace genlib {

readgenlib::readgenlib()
  : trace_parsing(false), trace_scanning(false) {}

int readgenlib::parse_file(const std::string &f) {
	trace_parsing = 0;
	file = f;
	location.initialize(&file);
	FILE* fp = fopen(f.c_str(), "r");
	if (!fp) {
		std::cout << "could not open file " << f << "\n";
		return 1;
	}
	scan_begin(fp);
	gl::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	if (res != 0) {
		std::cout << "failed to parse file " << f << "\n";
	}
	return res;
}

int readgenlib::parse_string(const std::string &f) {
	trace_parsing = 0;
	file = "";
	FILE* fp = fmemopen((void*)f.c_str(), f.size(), "r");
	auto tm = utl::_start_wall_timer();
	scan_begin(fp);
	gl::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	std::cout << "parse time: " << utl::_stop_wall_timer(tm) << "\n";
	return res;
}

void readgenlib::build_library(ckt::cell_library& mgr) {

	using boost::get;
	using namespace prs::genlib::ast;
	using namespace ckt;

	class genlib_builder : boost::static_visitor<> {
	public:
		cell_library *mgr = nullptr;

		void operator()(const ast::genlib& x) {
			for (auto& y : x.nls) {
				boost::apply_visitor(*this, y);
			}
		}

		void operator()(const ast::gate_def& x) {
			// std::cout << "trying to parse " << x.cell_fun << "\n";
			if (x.cell_fun == "CONST0" || x.cell_fun == "CONST1") {
				return;
			}
			circuit cir;
			prs::expr::parse_expr_to_cir(x.cell_fun, cir);
			id oid = *(cir.outputs().begin());
			cir.setwirename(oid, x.cell_out);
			// cir.write_bench();
			vector<string> port_names;
			vector<cell_library::port_direction> port_dirs;
			for (auto xid : cir.inputs()) {
				port_names.push_back(cir.wname(xid));
				port_dirs.push_back(cell_library::port_direction::IN);
			}
			port_names.push_back(x.cell_out);
			port_dirs.push_back(cell_library::port_direction::OUT);

			id cellid = mgr->add_cell(x.cell_name, port_names, port_dirs);
			mgr->add_cell_attr(cellid, "fun", cir);
			mgr->add_cell_attr(cellid, "area", x.cell_area);
			/*mgr->cells.at(cellid).cell_attr_vec.push_back(cir);
			mgr->cells.at(cellid).cell_attr_name2ind["fun"]
					 = mgr->cells.at(cellid).cell_attr_vec.size() - 1;*/
			//mgr->add_cell(x.cell_name, );
		}

		void operator()(const ast::latch_def& x) {
			neos_abort("under construction");
		}

	} visitor;

	visitor.mgr = &mgr;
	visitor(bast);

}

void readgenlib::print_ast() {
/*
	using boost::get;
	using namespace genlib_ast;

	class genlib_ast_printer : boost::static_visitor<> {
		int level = 0;
	public:

		string indent() {
			string ret;
			for (uint i = 0; i < level; i++)
				ret += "  ";
			return ret;
		}

		void operator()(const genlib_ast::node_assignment& x) {
			std::cout << indent() << x.lhs << " = " << x.rhs << ";\n";
		}

		void operator()(const genlib_ast::named_node& x) {
			std::cout << indent() << x.node_name << "(" << utl::to_delstr(x.node_params, ", ") << ")";
			if (!x.children.empty()) {
				level++;
				std::cout << " { \n";
				for (auto& y : x.children) {
					boost::apply_visitor(*this, y);
				}
				level--;
				std::cout << indent() <<  "}\n";
			}
			else std::cout << ";\n";
		}

		void operator()(const genlib_ast::node_property& x) {
			std::cout << indent() << x.name << " : " << x.prop << ";\n";
		}

		void operator()(const genlib_ast::liberty& x) {
			for (auto& y : x.nls) {
				boost::apply_visitor(*this, y);
			}
		}
	} visitor;


	std::cout << bast.nls.size() << "\n";
	visitor(bast);
*/
}

int parse_genlib_file(const std::string& filename, ckt::cell_library& cell_lib) {
	using namespace ckt;
	readgenlib rgl;
	int stat = rgl.parse_file(filename);
	if (stat != 0)
		return stat;
	rgl.build_library(cell_lib);
	return stat;
}

} // namespace prs
} // namespace genlib
