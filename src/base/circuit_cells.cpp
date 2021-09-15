/*
 * circuit_cells.cpp
 *
 *  Created on: Jan 31, 2020
 *      Author: kaveh
 */

#include "circuit.h"
#include "parser/liberty/readliberty.h"

namespace ckt {

const string CELL_LIB_ATTCH_NAME = "cl";

id circuit::add_instance(const string& inst_name, const string& cell_name,
		const vector<string>& net_names, const vector<string>& port_names) {
	idvec net_ids;
	std::cout << "adding instance " << inst_name << " " << cell_name << "\n nets: "
			<< utl::to_delstr(net_names, ", ") << "\n"
			<< utl::to_delstr(port_names, ", ") << "\n";

	assert(!net_names.empty());
	for (uint i = 0; i < net_names.size(); i++) {
		//std::cout << "at net " << net_names.at(i) << "\n";
		if (net_names[i] == "vdd")
			net_ids.push_back(get_const1());
		else if (net_names[i] == "gnd")
			net_ids.push_back(get_const0());
		else
			net_ids.push_back(find_wcheck(net_names[i]));
	}
	return add_instance(inst_name, cell_name, net_ids, port_names);
}

void circuit::remove_instance(id iid, bool update_wires) {

	assert( inst_exists(iid) );

	if (update_wires) {
		const auto& g = getcgate(iid);
		for (auto fo : g.fo()) {
			if ( wire_exists(fo) ) {
				getwire(fo).removefanin(iid);
			}
		}
		for (auto fi : g.fi()) {
			if ( wire_exists(fi) )
				getwire(fi).removefanout(iid);
		}
	}

	nodes.erase(iid, (int8_t)nltype::INST);
	node_names.remove_byfirst(iid);

	clear_topsort();
}

const cell_library::cell_obj& circuit::cget_cell(id cellid) const {
	return cget_cell_library().cells.at(cellid);
}

const cell_library::cell_obj& circuit::cget_cell_of_inst(id iid, id& cellid) const {
	const auto& cl = cell_info_of_inst(iid);
	cellid = cl.cellid;
	return cget_cell(cl.cellid);
}

const string& circuit::inst_pin_name(id iid, id ind) const {
	id x;
	return cget_cell_of_inst(iid, x).port_names[ind];
}


id circuit::add_instance(const string& inst_name, const string& cell_name,
		const vector<id>& net_ids, const vector< string >& port_names) {

	assert(!net_ids.empty());
	vector<string> port_names_c = port_names;
	if (port_names.empty()) { // implicitly defined port names
		for (uint i = 0; i < net_ids.size(); i++) {
			port_names_c.push_back("P" + std::to_string(i));
		}
	}

	assert(net_ids.size() == port_names_c.size());

	cell_library& cell_lib = get_cell_library();

	id cellid = cell_lib.add_cell(cell_name, port_names_c);

	return add_instance(inst_name, cellid, net_ids);
}

id circuit::add_instance(const string& inst_name, id cellid, const idvec& net_ids) {

	cell_library& cell_lib = get_cell_library();

	const auto& cl = cell_lib.getccell(cellid);

	id instid = nodes.create_new_entry(nlnode(nltype::INST), (int8_t)nltype::INST);
	auto& nd = nodes.at(instid);
	nd.nid = instid;
	node_names.add_pair(instid, inst_name);

	cell_nlattachment atch;
	atch.cellid = cellid;

	idvec fanins;
	idvec fanouts;
	for (uint i = 0; i < net_ids.size(); i++) {
		id netid = net_ids.at(i);
		if (cl.port_dirs.at(i) == cell_library::port_direction::OUT) {
			atch.fanout_port_inds.push_back(i);
			nd.fanouts.push_back(netid);
			getwire(netid).addfanin(instid);
			//std::cout << "fanout " << ndname(netid) << " to index " << cl.port_names.at(i) << "\n";
		}
		else {
			atch.fanin_port_inds.push_back(i);
			nd.fanins.push_back(netid);
			getwire(netid).addfanout(instid);
			//std::cout << "fanin " << ndname(netid) << " to index " << cl.port_names.at(i) << "\n";
		}
	}

	nd.attch.add_attachment("cell", atch);
	nd.ncat0 = nltype::INST;
	nd.ncat1 = (int8_t)fnct::INST;

	return instid;
}

id cell_library::add_cell(const string& cell_name, const vector<string>& port_names,
			const vector<port_direction>& pin_dirs) {

	auto pin_dirs_copy = pin_dirs;
	if (pin_dirs.empty()) {
		pin_dirs_copy = vector<port_direction>(port_names.size(), port_direction::INOUT);
	}

	//std::cout << "adding cell " << cell_name << "\n";

	id cellid = -1;
	if (_is_in(cell_name, cellname2id_map)) {
		cellid = cellname2id_map.at(cell_name);
		const auto& c = cells.at(cellid);
		for (uint i = 0; i < port_names.size(); i++) {
			if ( _is_not_in(port_names[i], c.portname2ind) ) {
				neos_error("netlsit error: cell " << cell_name
						<< " does not have port " << port_names[i] );
			}
		}
	}
	else {
		cellid = cells.create_new_entry();
		auto& cl = cells[cellid];
		cl.port_dirs = pin_dirs_copy;
		cl.cell_name = cell_name;
		for (uint i = 0; i < port_names.size(); i++) {
			cl.portname2ind[port_names[i]] = i;
			cl.port_names.push_back(port_names[i]);
		}
		cellname2id_map[cell_name] = cellid;
	}

	return cellid;
}

cell_library::cell_obj& cell_library::getcell(id cellid) {
	try {
		return cells.at(cellid);
	}
	catch (std::out_of_range&) {
		neos_abort("cell with id " << cellid << " not in library");
	}
}

const cell_library::cell_obj& cell_library::getccell(id cellid) const {
	try {
		return cells.at(cellid);
	}
	catch (std::out_of_range&) {
		neos_abort("cell with id " << cellid << " not in library");
	}
}

int cell_library::add_cell_attr(id cellid, const string& attr_nm, const std::any& attr) {
	return getcell(cellid).attch.add_attachment(attr_nm, attr);
}

void cell_library::print_lib() const {
	for (auto p : cells) {
		print_cell(p.first);
	}
}

void cell_library::print_cell(id cellid) const {
	const auto& cl = getccell(cellid);

	std::cout << cl.cell_name << "  ports:{";
	for (uint i = 0; i < cl.port_names.size(); i++) {
		std::cout << cl.port_names[i];
		std::cout << ":" << (int)cl.port_dirs[i];
		if (i != cl.port_names.size() - 1)
			std::cout << ", ";
		else
			std::cout << "}\n";
	}

	id fun_aind = cl.attch.get_attachment_id("funcir");
	if ( fun_aind != -1 ) {
		const auto& cir = cl.attch.get_cattatchment<circuit>(fun_aind);
		std::cout << "cell function circuit: ";
		cir.write_bench();
		std::cout << "\n";
		std::cout << "area: " << get_cell_attr<float>(cellid, "area") << "\n";
	}
}

void circuit::set_cell_library(cell_library& cl) {
	attch.add_attachment(CELL_LIB_ATTCH_NAME, std::make_shared<cell_library>(cl));
	has_cell_lib = true;
}

const cell_library& circuit::cget_cell_library() const {
	assert(has_cell_library());
	static id cell_lib_ind = -1;
	if (cell_lib_ind == -1) {
		cell_lib_ind = attch.get_attachment_id(CELL_LIB_ATTCH_NAME);
	}
	return *attch.get_cattatchment<std::shared_ptr<cell_library>>(cell_lib_ind);
}

cell_library& circuit::get_cell_library() {
	if (has_cell_library())
		return const_cast<cell_library&>(cget_cell_library());

	attch.add_attachment(CELL_LIB_ATTCH_NAME, std::make_shared<cell_library>()); // add new cell lib
	has_cell_lib = true;

	return get_cell_library();
}

const circuit::cell_nlattachment& circuit::cell_info_of_inst(id gid) const {
	static id cell_attch_id = -1;
	if (cell_attch_id == -1) {
		cell_attch_id = getcgate(gid).attch.get_attachment_id("cell");
	}
	return getcgate(gid).attch.get_cattatchment<cell_nlattachment>(cell_attch_id);
}

circuit::cell_nlattachment& circuit::cell_info_of_inst(id gid) {
	static id cell_attch_id = -1;
	if (cell_attch_id == -1) {
		cell_attch_id = getcgate(gid).attch.get_attachment_id("cell");
	}
	return getgate(gid).attch.get_attatchment<cell_nlattachment>(cell_attch_id);
}

void circuit::resolve_cell_directions(const vector<string>& out_port_names) {

	cell_library& cell_lib = get_cell_library();

	cell_lib.resolve_cell_direction_byname(out_port_names);

	foreach_instance(*this, g, gid) {
		auto& acth = cell_info_of_inst(gid);
		const auto& cl = cell_lib.cells.at(acth.cellid);
		idvec fanins, fanin_indices;
		idvec fanouts, fanout_indices;
		std::cout << "at gid " << gid << " " << ndname(gid) << "\n";
		for (uint i = 0; i < g.fanins.size(); i++) {
			id port_index = acth.fanin_port_inds.at(i);
			std::cout << wname(g.fanins[i]) << " dir -> " << cl.port_names[port_index] << "\n";
			if (cl.port_dirs[port_index] == cell_library::port_direction::IN) {
				fanins.push_back(g.fanins[i]);
				fanin_indices.push_back(port_index);
			}
			else if (cl.port_dirs[port_index] == cell_library::port_direction::OUT) {
				fanouts.push_back(g.fanins[i]);
				fanout_indices.push_back(port_index);
			}
			else {
				neos_error("unresolved port direction remains on port "
						<< cl.port_names[port_index] << " cell " << cl.cell_name
						<< " for instance " << gname(gid));
			}
		}
		for (auto fanin : g.fanins) getwire(fanin).removefanout(gid);
		for (auto fanout : g.fanouts) getwire(fanout).removefanin(gid);

		auto& gnc = getgate(gid);
		gnc.fanins = fanins;
		gnc.fanouts = fanouts;
		acth.fanin_port_inds = fanin_indices;
		acth.fanout_port_inds = fanout_indices;

		for (auto fanin : gnc.fanins) getwire(fanin).addfanout(gid);
		for (auto fanout : gnc.fanouts) getwire(fanout).addfanin(gid);
	}
	
	write_verilog();
}

void circuit::translate_cells_to_primitive_byname() {
	idset instances;
	foreach_instance(*this, g, gid) {
		instances.insert(gid);
	}

	for (auto gid : instances) {
		nlnode g = getnode(gid);
		id cellid = -1;
		const auto& clattch = cell_info_of_inst(gid);
		const auto& cell = cget_cell_of_inst(gid, cellid);
		string cellname = cell.cell_name;
		if (cellname.substr(0, 3) == "AND") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::AND);
		}
		else if (cellname.substr(0, 2) == "OR") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::OR);
		}
		else if (cellname.substr(0, 3) == "NOT" || cellname.substr(0, 3) == "INV") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::NOT);
		}
		else if (cellname.substr(0, 3) == "BUF") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::BUF);
		}
		else if (cellname.substr(0, 3) == "NOR") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::NOR);
		}
		else if (cellname.substr(0, 4) == "NAND") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::NAND);
		}
		else if (cellname.substr(0, 3) == "XOR") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::XOR);
		}
		else if (cellname.substr(0, 4) == "XNOR") {
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::XNOR);
		}
		else if (cellname.substr(0, 3) == "MUX") {
			idvec ofanins(3, -1);
			for (uint i = 0; i < clattch.fanin_port_inds.size(); i++) {
				uint ind = clattch.fanin_port_inds[i];
				if (cell.port_names[ind] == "A") {
					ofanins[1] = g.fanins[i];
				}
				else if (cell.port_names[ind] == "B") {
					ofanins[2] = g.fanins[i];
				}
				else if (cell.port_names[ind] == "S") {
					ofanins[0] = g.fanins[i];
				}
			}
			g.fanins = ofanins;
			remove_instance(gid);
			add_gate(g.fanins, g.fo0(), fnct::MUX);
		}
	}

}

void cell_library::resolve_cell_direction_byname(const vector<string>& out_names) {
	for (auto& p : cells) {
		auto& cl = p.second;
		std::cout << cl.port_dirs.size() << " " << cl.port_names.size() << "\n";
		cl.port_dirs = vector<cell_library::port_direction>(cl.port_dirs.size(), port_direction::IN);
		for (const auto& pnm : out_names) {
			if ( _is_in(pnm, cl.portname2ind) ) {
				std::cout << "setting port " << pnm << " as output on cell " << cl.cell_name << "\n";
				id port_index = cl.portname2ind.at(pnm);
				cl.port_dirs[port_index] = port_direction::OUT;
			}
		}
	}
}

void cell_library::read_liberty(const string& filename) {
	prs::liberty::readliberty lib;
	lib.parse_file(filename);
	lib.build_library(*this);
}

}
