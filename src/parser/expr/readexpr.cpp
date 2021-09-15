

#include "parser/expr/readexpr.h"
#include "expr_parser.tab.hh"
#include "base/circuit.h"
#include "parser/parser.h"

namespace prs {
namespace expr {

readexpr::readexpr()
  : trace_parsing(false), trace_scanning(false) {}

int readexpr::parse_file(const std::string &f) {
	trace_parsing = 0;
	file = f;
	location.initialize(&file);
	FILE* fp = fopen(f.c_str(), "r");
	scan_begin(fp);
	ex::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	return res;
}

int readexpr::parse_string(const std::string &f) {
	trace_parsing = 0;
	trace_scanning = 0;
	file = "";
	FILE* fp = fmemopen((void*)f.c_str(), f.size(), "r");
	// auto tm = utl::_start_wall_timer();
	scan_begin(fp);
	ex::parser parse(*this);
	parse.set_debug_level(trace_parsing);
	int res = parse();
	scan_end();
	if (res != 0) {
		std::cout << "problem parsing expression string " << f << "\n";
		return res;
	}
	//std::cout << "parse time: " << utl::_stop_wall_timer(tm) << "\n";
	return res;
}

int parse_expr_to_cir(const std::string& expr_str, ckt::circuit& cir) {
	readexpr rex;
	int res = rex.parse_string(expr_str);
	if (res != 0)
		return res;
	rex.to_circuit(cir);
	return res;
}

void readexpr::to_circuit(ckt::circuit& retcir) {

	using boost::get;
	using namespace expr;
	using namespace ckt;

	class expr_ast_to_circuit : boost::static_visitor<> {
		int level = 0;
		circuit& cir;

	public:
		expr_ast_to_circuit(circuit& cir) : cir(cir) {}

		id operator()(const expr::ast::unit_expr& x) {
			// std::cout << "bare expr " << x << "\n";
			return cir.add_wire_necessary(x, wtype::IN);
		}

		id operator()(const expr::ast::unop_expr& x) {
			id o = cir.add_wire(wtype::INTER);
			id a = boost::apply_visitor(*this, x.e);
			cir.add_gate({a}, o, fnct::NOT);
			return o;
		}

		id operator()(const expr::ast::binop_expr& x) {
			id o = cir.add_wire(wtype::INTER);
			id a = boost::apply_visitor(*this, x.lhs);
			id b = boost::apply_visitor(*this, x.rhs);
			if (x.op_str == "*" || x.op_str == "&")
				cir.add_gate({a, b}, o, fnct::AND);
			else if (x.op_str == "+" || x.op_str == "|")
				cir.add_gate({a, b}, o, fnct::OR);
			else {
				neos_abort("unsupported operator " << x.op_str << " in parsing expression");
			}

			return o;
		}
	};

	expr_ast_to_circuit vis(retcir);

	id oid = boost::apply_visitor(vis, bast);
	if (retcir.nWires() == 1) {
		id o = retcir.add_wire(wtype::OUT);
		retcir.add_gate({oid}, o, fnct::BUF);
	}
	else {
		retcir.setwiretype(oid, wtype::OUT);
	}
}

} // namespace expr
} // namespace prs

