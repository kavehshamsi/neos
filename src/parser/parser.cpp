/*

 * parser.cpp
 *
 *  Created on: Oct 8, 2017
 *      Author: kaveh
 */



#include "base/circuit.h"
#include "parser/parser.h"
#include <iomanip>
#include "parser/bench/readbench.h"
#include "parser/verilog/readverilog.h"

namespace prs {

// helper error method
inline void _undefined_wire (string wire_name, string gate_name) {
	neos_error("wire " << wire_name
			<< " in gate " << gate_name << " is undefined\n");
}

namespace bench {

bool parse_bench_file(const std::string& filename, ckt::circuit& cir) {
	readbench rdb;
	rdb.parse_file(filename);
	rdb.to_circuit(cir);
	return true;
}

bool parse_bench(std::istream& inist, ckt::circuit& cir) {
	readbench rdb;
	std::string str(std::istreambuf_iterator<char>(inist), {});
	rdb.parse_string(str);
	rdb.to_circuit(cir);
	return false;
}

} // namespace bench


namespace verilog {

bool parse_verilog_file(const std::string& filename, ckt::circuit& cir) {
	readverilog rdv;
	rdv.parse_file(filename);
	rdv.to_circuit(cir);
	return true;
}

bool parse_verilog(std::istream& inist, ckt::circuit& cir) {
	readverilog rdv;
	std::string str(std::istreambuf_iterator<char>(inist), {});
	rdv.parse_string(str);
	rdv.to_circuit(cir);
	return true;
}

} // namespace verilog


} // namespace prs



