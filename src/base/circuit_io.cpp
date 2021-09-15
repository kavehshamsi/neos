
#include "circuit.h"
#include "parser/parser.h"
#include <filesystem>
//////////////// The circuit class ////////////////////

namespace ckt {

using namespace utl;

circuit::circuit(const string& filename, char format) {
	if (format == 'b')
		read_bench(filename);
	else if (format == 'v')
		read_verilog(filename);
}

void circuit::print_wireset(idset wset) const {
	int i = 1;
	for (auto& x : wset) {
		std::cout << wname(x);
		if ( ((i++) % 10) == 0)
			std::cout << "\n";
		else
			std::cout << " ";
	}
	std::cout << "\n";
}

void circuit::print_wire(id wid) const {
	const auto& w = getcwire(wid);
	std::cout << wname(wid) << " ins: ";
	for (auto wid : w.fanins) {
		std::cout << wname(wid) << ", ";
	}
	std::cout << ";  out: ";
	for (auto wid : w.fanouts) {
		std::cout << wname(wid) << ", ";
	}
	std::cout << "\n";
}

// gate-level verilog
bool circuit::read_verilog(std::istream& infin) {
	return prs::verilog::parse_verilog(infin, *this);
}

bool circuit::read_verilog(const std::string& filename) {
	if (!utl::_file_exists(filename)) {
		neos_error("file " << filename << " cannot be opened");
	}
	top_name = std::filesystem::path(filename).stem();
	return prs::verilog::parse_verilog_file(filename, *this);
}


bool circuit::read_bench(std::istream& inist) {
	return prs::bench::parse_bench(inist, *this);
}

bool circuit::read_bench(const std::string& filename) {
	if (!utl::_file_exists(filename)) {
		neos_error("file " << filename << " cannot be opened");
	}
	top_name = std::filesystem::path(filename).stem();
	return prs::bench::parse_bench_file(filename, *this);
}

void circuit::write_verilog(std::ostream& fout) const {

	id cnt = 0;
	std::string outstring = "// Verilog File \n";

	auto verwname = [&](id wid) -> string {
		if (is_const0(wid))
			return "1'b0";
		else if (is_const1(wid))
			return "1'b1";
		return wname(wid);
	};

	outstring += "module ";
	outstring += (top_name.empty() ? "cir":top_name) + " (";
	foreach_wire((*this), w, wid) {
		if ( w.isInput() || w.isOutput() || w.isKey() ) {
			//std::cout << wname(wid) << " " << cnt << "\n";
			cnt++;
			if ( cnt % 10 == 9 )
				outstring += "\n";
			if ( cnt != nInputs() + nOutputs() + nKeys() )
				outstring += wname(wid) + " , ";
			else
				outstring += wname(wid);
		}
	}
	outstring += " ) ;\n\n";
	cnt = 0;
	if (nInputs() > 0) {
		outstring += std::string("input ");
		foreach_wire((*this), w, wid) {
			if ( w.isKey() || w.isInput()) {
				cnt++;
				if ( cnt % 10 == 9 )
					outstring += "\n";
				if ( cnt != nInputs() + nKeys() )
					outstring += wname(wid) + " , ";
				else
					outstring += wname(wid);
			}
		}
		outstring += " ;\n\n";
	}
	cnt = 0;
	if (nOutputs() > 0) {
		outstring += std::string("output ");
		foreach_wire((*this), w, wid) {
			if ( w.isOutput() ) {
				cnt++;
				if ( cnt % 10 == 0 )
					outstring += "\n";
				if ( cnt != nOutputs() )
					outstring += wname(wid) + " , ";
				else
					outstring += wname(wid);
			}
		}
		outstring += " ;\n\n";
	}

	cnt = 0;
	if ( nInters() > 0 ) {
		outstring += std::string("wire ");
		foreach_wire((*this), w, wid) {
			if ( w.isInter() && !isConst(wid) ) {
				cnt++;
				if ( cnt % 10 == 9 )
					outstring += "\n";
				if ( cnt != nInters() - has_const0() - has_const1() )
					outstring += wname(wid) + " , ";
				else
					outstring += wname(wid);
			}
		}
		outstring += " ;\n\n";
	}

	foreach_gate((*this), g, gid) {
		if (g.gfun() != fnct::INST) {

			if (g.gfun() == fnct::MUX) {
				outstring += "assign "
						+ verwname(gfanout(gid)) + " = "
						+ verwname(gfanin(gid)[0]) + " ? "
						+ verwname(gfanin(gid)[1]) + " : "
						+ verwname(gfanin(gid)[2]) + ";\n";
			}

			else { // normal primtive cells
				outstring += funct::enum_to_string(gfunction(gid)) + " "
						+ gname(gid) + "("
						+ verwname(gfanout(gid)) + " , ";
				for ( uint64_t j = 0; j < gfanin(gid).size() ; j++) {
						outstring += verwname(nodes.at(gid).fanins[j]);
					if (j != nodes.at(gid).fanins.size()-1) {
						outstring += " , ";
					}
					else {
						outstring += " ) ;\n";
					}
				}
			}
		}
	} // end of gate printing for loop

	if (nInst() != 0) {
		const cell_library& cell_mgr = cget_cell_library();
		foreach_instance(*this, g, gid) {
			const auto& cli = cell_info_of_inst(gid);
			cell_mgr.print_cell(cli.cellid);
			const auto& inst_cell = cell_mgr.cells.at(cli.cellid);
			outstring += inst_cell.cell_name
					+ std::string(" ")
					+ ndname(gid) + " ( ";
			for ( uint i = 0; i < g.fanins.size(); i++ ) {
				id port_index = cli.fanin_port_inds.at(i);
				outstring += "." + inst_cell.port_names.at(port_index) + "( "
						+ verwname(g.fanins[i]);
				if (i != g.fanins.size() - 1) 
					outstring += " ), ";
				else 
					outstring += " ) ";
			}
			//std::cout << " g.fanout: " << g.fanouts.size() << "\n";
			if (!g.fanouts.empty())
			    outstring += ", ";
			for ( uint i = 0; i < g.fanouts.size(); i++ ) {
				id port_index = cli.fanout_port_inds.at(i);
				outstring += "." + inst_cell.port_names.at(port_index) + "("
						+ verwname(g.fanouts[i]);
				if (i != g.fanouts.size() - 1) 
					outstring += " ), ";
				else
					outstring += " ) ";
			}
			outstring += " ) ;\n";
		}
	}

	outstring += "endmodule\n";

	fout << outstring;
}

void circuit::write_verilog(const std::string& filename) const {
	std::ofstream fout(filename, std::ofstream::out);
	write_verilog(fout);
	fout.close();
}

void circuit::write_bench(const std::string& filename) const {
	std::ofstream fout(filename, std::ofstream::out);
	write_bench(fout);
	fout.close();
}

void circuit::write_bench(std::ostream& fout) const {

	//fout << "# BENCH FILE \n";
	fout << "# circuit:" + top_name + "\n";

	// print names in string-sorted manner
	for (auto& xid : inputs())
			fout << std::string("INPUT(")
			+ wname(xid) + std::string(")\n");

	for (auto kid : keys())
		fout << std::string("INPUT(")
		+ wname(kid) + std::string(")\n");

	for (auto oid : outputs())
		fout << std::string("OUTPUT(")
			+ wname(oid) + std::string(")\n");

	foreach_latch((*this), dffid, dffout, dffin) {
		fout << wname(dffout) + " = dff( " + wname(dffin) + " )\n";
	}

	foreach_gate((*this), g, gid) {
		if(gfunction(gid) != fnct::UNDEF && gfunction(gid) != fnct::DFF ){
			fout << wname(g.fo0()) + " = "
					+ funct::enum_to_string(g.gfun()) + "( ";
			for ( uint64_t j = 0; j < g.fanins.size() ; j++ ){
				fout << wname(g.fanins[j]);

				if ( j != g.fanins.size() - 1 ){
					fout << ", ";
				}
				else{
					fout << " )";
				}
			}
			fout << "\n";
		}
	}

}

void circuit::rename_wires() {

	id num_ins = 1, num_inters = 1, num_outs = 1, num_louts = 1;
	foreach_wire((*this), w, wid) {
		if (_is_in(wfanin0(wid), dffs))
			setwirename(wid, "s" + std::to_string(num_louts++));
		else if (w.isInput())
			setwirename(wid, "in" + std::to_string(num_ins++));
		else if (w.isOutput())
			setwirename(wid, "o" + std::to_string(num_outs++));
		else if (isConst(wid))
			continue;
		else if (w.isInter())
			setwirename(wid, "w" + std::to_string(num_inters++));
	}
}

void circuit::rename_internal_wires() {
	foreach_wire((*this), w, wid) {
		if (isInter(wid) && !isConst(wid)) {
			setwirename(wid);
		}
	}
}

} // namespace ckt
