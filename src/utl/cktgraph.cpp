/*
 * graph.cpp
 *
 *  Created on: Dec 31, 2016
 *      Author: kaveh
 */


#include <utl/cktgraph.h>

namespace cktgph {

void cktgraph::initialize(const circuit& cir) {

	//const_cast<circuit&>(cir).resolve_cell_directions({"CO_0_", "CO_1_", "CO_2_", "CO_3_", "O_0_", "O_1_", "O_2_", "O_3_", "O"});
	assert(cir.error_check());

	gp = Graph();

	num_cycles = 0;

	vis = cycle_printer(*this);

	idvec gatesinst = cir.gates_and_instances();

	for (auto gid : gatesinst) {
		create_vertex(gp, gid);
	}

	for (auto xid : cir.allins()) {
		create_vertex(gp, xid);
	}

	for (auto yid : cir.outputs()) {
		create_vertex(gp, yid);
	}

	if (cir.has_const0()) {
		create_vertex(gp, cir.get_cconst0());
	}
	if (cir.has_const1()) {
		create_vertex(gp, cir.get_cconst1());
	}

	for (auto gid : gatesinst) {
		const auto& g = cir.getcgate(gid);

		for (auto in : g.fanins) {
			auto& wfanin = cir.wfanins(in);
			if (wfanin.empty()) {
				add_edge_wid(in, gid);
			}
			else {
				for (auto x : wfanin) {
					add_edge_wid(x, gid);
				}
			}
		}

		//std::cout << "at " << cir.ndname(gid) << "\n";

		assert(!g.fanouts.empty());
		assert(cir.node_exists(g.fo0()));

		if ( cir.isOutput(g.fo0()) ) {
			add_edge_wid(gid, g.fo0());
		}
	}
}

vdes cktgraph::create_vertex(Graph& grph, id nid) {
	vdes v = add_vertex(grph);
	id2vdesmap[nid] = v;
	vdes2idmap[v] = nid;
	return v;
}

void cktgraph::add_edge_wid(id uid, id vid) {
	add_edge(id2vdesmap.at(uid), id2vdesmap.at(vid), gp);
}


void cktgraph::countCycles() {

	std::cout << "Iterating over cycles: \n";
	// Instantiate the visitor for printing cycles

    // Use the Tiernan algorithm to visit all cycles, printing them
    // as they are found.
    tiernan_all_cycles(gp, vis);
    std::cout << "cycle count: " << num_cycles;
}

void cktgraph::writeVizFile(const std::string& filename) {


	// write the dot file
	std::ofstream dotfile (filename.c_str ());
	write_graphviz (dotfile, gp, node_color_writer(*this), default_writer());
}

vdes cktgraph::wire_to_vdes(id wr) {
	vdes ret;
	if (cir.wfanins(wr).empty())
		ret = id2vdesmap.at(wr);
	else
		// works for unique gate-fanout circuits
		ret = id2vdesmap.at(*cir.wfanins(wr).begin());
	return ret;
}

/*
void cktgraph::writeVizFile_wNodeStr(const std::string& filename,
		const Hmap<id, std::string>& node_strings) {

	std::set<edes> edge_set;

	for (auto& e : colored_edges) {
		vdes n1 = wire_to_vdes(e.first);
		vdes n2 = gid2vdesmap.at(e.second);
		add_edge(n1, n2, gp);
		assert(edge(n1, n2, gp).second);
		edge_set.insert(edge(n1, n2, gp).first);
	}

	std::ofstream dotfile(filename.c_str());
	write_graphviz (dotfile, gp, node_color_writer(gp, *this),
			edge_color_writer(gp, *this, edge_set));
}
*/

node_color_writer::node_color_writer(const cktgraph& owner) :
	owner(owner), first_round(true) {

	std::string base = "[style=filled][shape=circle]";
	foreach_wire(owner.cir, w, wid) {
		if (owner.cir.isConst(wid)) {
			wire_labels[wid] = base + fmt("[fillcolor=gray][label=\"%s\"]", owner.cir.ndname(wid));
		}
		else if (w.isInput()) {
			wire_labels[wid] = base + fmt("[fillcolor=red][label=\"%s\"]", owner.cir.ndname(wid));
		}
		else if (w.isKey()) {
			wire_labels[wid] = base + fmt("[fillcolor=green][label=\"%s\"]", owner.cir.ndname(wid));
		}
		else if (w.isOutput()) {
			wire_labels[wid] = base + fmt("[fillcolor=blue][label=\"%s\"]", owner.cir.ndname(wid));
		}
		else {
			wire_labels[wid] = base + fmt("[fillcolor=gray][label=\"%s\"]", owner.cir.ndname(wid));
		}
	}

	foreach_instance(owner.cir, g, gid) {
		id cellid = owner.cir.cell_info_of_inst(gid).cellid;
		const auto& cl = owner.cir.cget_cell_library().cells.at(cellid);
		gate_labels[gid] = fmt("[fillcolor=white][labelfontcolor=white][label=\"%s\"][style=filled][shape=box]", cl.cell_name);
	}

	foreach_gate(owner.cir, g, gid) {
		if (g.isLatch()) {
			gate_labels[gid] = "[fillcolor=black][labelfontcolor=white][label=\"DFF\"][style=filled][shape=box]";
		}
		else {
			std::string gfun_str = funct::enum_to_string(owner.cir.gfunction(gid));
			gate_labels[gid] =  "[fillcolor=gray][style=filled][shape=triangle][orientation=180][label=" + gfun_str + "]";
		}

	}

}


void cktgraph::writeVizFile_wcolorEdges(const std::string& filename,
		const std::set<idpair>& colored_edges) {

	std::set<edes> edge_set;

	for (auto& e : colored_edges) {
		vdes n1 = wire_to_vdes(e.first);
		vdes n2 = id2vdesmap.at(e.second);
		add_edge(n1, n2, gp);
		assert(edge(n1, n2, gp).second);
		edge_set.insert(edge(n1, n2, gp).first);
	}

	std::ofstream dotfile(filename.c_str());
	write_graphviz (dotfile, gp, node_color_writer(*this),
			edge_color_writer(*this, edge_set));
}

void cktgraph::printGraph() {
	std::cout << "\ncircuit-graph edges:\n";
//	std::pair<vertex_iterator, vertex_iterator> vi = vertices(gp);
//	for (vertex_iterator v_it = vi.first; v_it != vi.second; ++v_it) {
//		std::cout << *v_it << ": " << ckt.getwire_name(vid2idmap[*v_it]) << "\n";
//	}
}

} // namespace cktgraph
