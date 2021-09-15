

#include "parser/liberty/readliberty.h"
#include "liberty_parser.tab.hh"
#include "base/circuit.h"
#include "parser/expr/readexpr.h"

namespace prs {
namespace liberty {

readliberty::readliberty()
  : trace_parsing(false), trace_scanning(false) {}

int readliberty::parse_file(const std::string &f) {
	trace_parsing = 0;
	file = f;
	location.initialize(&file);
	FILE* fp = fopen(f.c_str(), "r");
	scan_begin(fp);
	ll::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	return res;
}

int readliberty::parse_string(const std::string &f) {
	trace_parsing = 0;
	file = "";
	FILE* fp = fmemopen((void*)f.c_str(), f.size(), "r");
	auto tm = utl::_start_wall_timer();
	scan_begin(fp);
	ll::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	std::cout << "parse time: " << utl::_stop_wall_timer(tm) << "\n";
	return res;
}

void readliberty::build_library(ckt::cell_library& mgr) {
	using boost::get;
	using namespace liberty_ast;
	using namespace ckt;

	class liberty_builder : boost::static_visitor<> {
	public:
		cell_library *mgr = nullptr;

		void operator()(const liberty_ast::node_assignment& x) {

		}

		void operator()(const liberty_ast::node_property& x) {

		}

		void operator()(const liberty_ast::liberty& x) {
			for (auto& y : x.nls) {
				boost::apply_visitor(*this, y);
			}
		}

		void operator()(const liberty_ast::named_node& x) {
			string cell_name;
			if (x.node_name == "cell") {
				if (x.node_params.size() != 1) {
					neos_error("library syntax error: cell name should be singular at "
							<< x.node_params[0]);
				}
				cell_name = x.node_params[0];


				vector<string> pin_names;
				vector<cell_library::port_direction> pin_dirs;
				Hmap<string, std::any> attributes;
				for (auto& y : x.children) {
					if (const named_node* p = boost::get<named_node>(&y)) {
						if (p->node_name == "pin") {
							if (p->node_params.size() != 1) {
								neos_error("library syntax error: pin name should be singular at "
										<< p->node_params[0]);
							}
							pin_names.push_back(p->node_params[0]);
							for (auto& z : p->children) {
								if (const node_property* pz = boost::get<node_property>(&z)) {
									if (pz->name == "direction") {
										cell_library::port_direction pdir;
										if (pz->prop == "output") {
											pdir = cell_library::port_direction::OUT;
										}
										else if (pz->prop == "input") {
											pdir = cell_library::port_direction::IN;
										}
										else if (pz->prop == "inout") {
											pdir = cell_library::port_direction::INOUT;
										}
										pin_dirs.push_back(pdir);
										std::cout << "pdir : " << pz->prop << "\n";
									}
									else if (pz->name == "function") {
										std::cout << "function :" << pz->prop << "\n";
										circuit funcir;
										prs::expr::parse_expr_to_cir(pz->prop, funcir);
										attributes["fun"] = funcir;
										//fm::formula  frm = parse_function(pz->prop);
										assert(false);
									}
								}
							}
						}
					}
				}

				std::cout << "cell : " << cell_name << " pin: "
						<< utl::to_delstr(pin_names, ", ") << "\n";
				id cellid = mgr->add_cell(cell_name, pin_names, pin_dirs);
				for (auto& p : attributes) {
					mgr->add_cell_attr(cellid, p.first, p.second);
				}
			}
			else {
				for (auto& y : x.children) {
					boost::apply_visitor(*this, y);
				}
			}
		}


	} visitor;

	visitor.mgr = &mgr;
	visitor(bast);
}

void readliberty::print_ast() {
	using boost::get;
	using namespace liberty_ast;

	class liberty_ast_printer : boost::static_visitor<> {
		int level = 0;
	public:

		string indent() {
			string ret;
			for (uint i = 0; i < level; i++)
				ret += "  ";
			return ret;
		}

		void operator()(const liberty_ast::node_assignment& x) {
			std::cout << indent() << x.lhs << " = " << x.rhs << ";\n";
		}

		void operator()(const liberty_ast::named_node& x) {
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

		void operator()(const liberty_ast::node_property& x) {
			std::cout << indent() << x.name << " : " << x.prop << ";\n";
		}

		void operator()(const liberty_ast::liberty& x) {
			for (auto& y : x.nls) {
				boost::apply_visitor(*this, y);
			}
		}
	} visitor;


	std::cout << bast.nls.size() << "\n";
	visitor(bast);
}


int parse_liberty_file(const std::string& filename, ckt::cell_library& cell_lib) {
	using namespace ckt;
	readliberty rgl;
	int stat = rgl.parse_file(filename);
	if (stat != 0)
		return stat;
	rgl.build_library(cell_lib);
	return stat;
}

} // namespace liberty
} // namespace prs

