/*
 * graph.h
 *
 *  Created on: Dec 31, 2016
 *      Author: kaveh
 */

#ifndef GRAPH_H_
#define GRAPH_H_

// standard libs
#include <iomanip>


// boost
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/tiernan_all_cycles.hpp>

#include "base/circuit.h"
#include "utility.h"

/*
 * a class for converting circuit to boost graphs
 * for visulization and other applications
 */
namespace cktgph {

using namespace ckt;
using namespace utl;
using namespace boost;

typedef directed_graph<> Graph;
typedef std::pair<int, int> Edge;
typedef Graph::edge_iterator edge_iterator;
typedef Graph::vertex_iterator vertex_iterator;
typedef Graph::vertex_descriptor vdes;
typedef Graph::edge_descriptor edes;
template<typename K, typename T>
using Map = std::map<K, T>;

class cktgraph {

public:
	Graph gp;

	const circuit& cir;

	// mappings for nodes
	Map<id, vdes> id2vdesmap;
	Map<vdes, id> vdes2idmap;

public:
	cktgraph(const circuit& ckt) : cir(ckt) { initialize(ckt); };

	void initialize(const circuit& ckt);

	vdes create_vertex(Graph& grph, id nid);
	void add_edge_wid(id u, id v);

	void countCycles();

	void printGraph();
	void writeVizFile(const std::string& filename);
	void writeVizFile_wcolorEdges(const std::string& filename,
			const std::set<idpair>& colored_edges);

	vdes wire_to_vdes(id wr);



	/*
	 * for terrible complexity cycle enumeration
	 */

	// cycle count visitor structs
	uint64_t num_cycles;

	void increment_cycles() {
		num_cycles++;
	}


	struct cycle_printer
	{
	private:

		cktgraph *cgp;

	public:
		cycle_printer() : cgp(NULL) {}
		cycle_printer(cktgraph &cgp) :
			cgp(&cgp) {}


		template <typename Path>
		void cycle(const Path& p, const Graph& g)
		{
			// Get the property map containing the vertex indices
			// so we can print them.
			typedef typename property_map<Graph, vertex_index_t>::const_type IndexMap;
			IndexMap indices = get(vertex_index, g);

			// Iterate over path printing each vertex that forms the cycle.
			cgp->increment_cycles();
			typename Path::const_iterator i, end = p.end();
			for(i = p.begin(); i != end; ++i) {
			   std::cout << get(indices, *i) << " ";
			}
			std::cout << std::endl;
		}
	};

	cycle_printer vis;

};

/*
* property writer classes for coloring the graphviz file
*/
class node_color_writer {

private:
	const cktgraph& owner;
public:
	Hmap<id, std::string> gate_labels;
	Hmap<id, std::string> wire_labels;

	bool first_round;
	// constructor
	node_color_writer(const cktgraph& owner);

	// functor that does the coloring
	template <class Node>
	void operator()(std::ostream& out, const Node& v) {
		out << "[shape=circle]";
		auto it = owner.vdes2idmap.find(v);
		assert(it != owner.vdes2idmap.end());
		if (owner.cir.isGate(it->second) || owner.cir.isInst(it->second)) {
			// node is a gate
			id gid = owner.vdes2idmap.at(v);
			out << gate_labels.at(owner.vdes2idmap.at(v));
		}
		else {
			id wid = it->second;
			out << wire_labels.at(wid);
		}
	}
};

/*
* edge color writier
*/
class edge_color_writer {

private:
	const cktgraph& owner;
	const std::set<edes>& colored_edges;
	const circuit& ckt;
public:
	// constructor
	edge_color_writer(const cktgraph& owner, const std::set<edes>& colored_edges):
		owner(owner), colored_edges(colored_edges), ckt(owner.cir) {}

	// functor that does the coloring
	template <class Edgetmp>
	void operator()(std::ostream& out, const Edgetmp& e) const {

		out << "[concentrate=true]";
		if (_is_in(e, colored_edges) ) {
			out << "[color=blue]";
			out << "[penwidth=3]";
		}
		return;
	}
};


}


#endif /* SRC_GRAPH_H_ */
