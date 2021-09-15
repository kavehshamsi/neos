/*
 * aig_read.cpp
 *
 *  Created on: Dec 11, 2020
 *      Author: kaveh
 */

#include "aig/aig.h"

#include <boost/tokenizer.hpp>

namespace aig_ns {


void aig_t::write_bench(std::string filename) const {
	circuit cktofaig;
	to_circuit(cktofaig);
	assert(cktofaig.error_check());
	if (filename == "")
		cktofaig.write_bench();
	else
		cktofaig.write_bench(filename);

}

void aig_t::write_verilog(std::string filename) const {
	circuit cktofaig;
	to_circuit(cktofaig);
	assert(cktofaig.error_check());
	if (filename == "")
		cktofaig.write_verilog();
	else
		cktofaig.write_verilog(filename);

}


// bench-like file for raw aigs
void aig_t::write_aig(const std::string& filename) const {
	auto fout = std::ofstream(filename, std::ofstream::out);
	write_aig(fout);
}

void aig_t::write_aig(std::ostream& outss) const {

	for (auto aid : inputs()) {
		outss << "INPUT(" << ndname(aid) << ")\n";
	}
	for (auto aid : outputs()) {
		outss << "OUTPUT(" << ndname(aid) << ")\n";
	}

	foreach_node((*this), nd, nid) {
		if (nd.isLatch()) {
			outss << latch_to_string(nd) << "\n";
		}
	}

	foreach_node_ordered((*this), nd, nid) {
		if (nd.isAnd() || nd.isOutput()) {
			outss << and_to_str(nd) << "\n";
		}
	}
}

std::ostream& operator<<(std::ostream& out, const alit& al) {
	out << al.lid() << ":" << al.sgn();
	return out;
}

void aig_t::print_node(id aid) const {
	std::cout << to_str(getcNode(aid)) << "\n";
}

void aig_t::print_stats() const {
	std::cout << "   #inputs: " << nInputs();
	std::cout << "   #output: " << nOutputs();
	std::cout << "   #dffs: " << nLatches();
	std::cout << "   #ands: " << nAnds() << "\n";
}

std::string aig_t::latch_to_string(const aigNode& nd) const {
	std::stringstream ss;
	ss << ndname(nd.nid) << " = dff(" << ndname(nd.fanin0) << ")";
	return ss.str();
}

std::string aig_t::to_str(id nid) const {
	return to_str(getcNode(nid));
}

std::string aig_t::and_to_str(const aigNode& nd) const {
	std::stringstream ss;
	ss << ndname(nd.nid) << " = " <<
				"and(" << ndname(nd.fanin0)
				<< ", "	<< ndname(nd.fanin1) << ")";
	return ss.str();
}

std::string aig_t::to_str(const aigNode& nd) const {
	std::stringstream ss;
	switch (nd.ntype()) {
	case ndtype_t::NTYPE_AND:
		ss << ndname(nd.nid) << " = " <<
				"and(" << ndname(nd.fanin0)
				<< ", "	<< ndname(nd.fanin1) << ")";
		break;
	case ndtype_t::NTYPE_LATCH:
		ss << ndname(nd.nid) << " = dff(" << (nd.cm0() ? "~":"") << ndname(nd.fi0()) << ")";
		break;
	case ndtype_t::NTYPE_IN:
		ss << "INPUT(" << ndname(nd.nid) << ")";
		break;
	case ndtype_t::NTYPE_OUT:
		ss << "OUTPUT(" << ndname(nd.nid) << ")";
		break;
	case ndtype_t::NTYPE_CONST0:
		ss << ndname(nd.nid);
		break;
	}

	return ss.str();
}



void aig_t::read_aig(const string& filename) {
	std::ifstream fin(filename, std::ios::in);
	size_t line_num = 0;
	string line;
	Hmap<string, bool> outcms;
	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	while (std::getline(fin, line)) {
	    std::cout << "\n";
		std::cout << line << "\n";
		string outname, in0, in1;
		boost::char_separator<char> sep("=(), ");
		tokenizer tok(line, sep);
	    tokenizer::iterator beg = tok.begin();
	    outname = *beg++;
	    if (outname == "INPUT") {
			std::cout << "adding input " << *beg << "\n";
			add_input(*beg);
	    }
	    else if (outname == "OUTPUT") {
	    	string nm = *beg;
	    	bool cm0 = (nm[0] == '~');
	    	std::cout << "adding output " << nm << "\n";
	    	nm = cm0 ? nm.substr(1, string::npos):nm;
	    	add_output(nm);
	    	outcms[nm] = cm0;
	    }
	    else {
			if (*beg++ != "and") {
				neos_error("read_aig error: expected \"and\" in line " << line_num);
			}
			in0 = *beg++;
			in1 = *beg++;
			if (beg != tok.end()) {
				neos_error("read_aig error: extra text at line " << line_num);
			}

			bool cm0 = (in0[0] == '~');
			bool cm1 = (in1[0] == '~');
			in0 = cm0 ? in0.substr(1, string::npos):in0;
			in1 = cm1 ? in1.substr(1, string::npos):in1;
			std::cout << "adding node " << outname << " = " << cm0 << ":" << in0 << " & " << cm1 << ":" << in1 << "\n";

			id fi0 = find_node(in0);
			id fi1 = find_node(in1);

			id oid = find_node(outname);
			if (oid !=- 1) {
				alit nl = add_and(fi0, cm0, fi1, cm1);
				add_edge(alit(nl.lid(), outcms.at(outname) ^ nl.sgn()), oid);
			}
			else {
				add_and(fi0, cm0, fi1, cm1, outname);
			}
	    }

		line_num++;
	}
}

} // namespace aig_ns

