/*
 * graph.h
 *
 *  Created on: Nov 24, 2017
 *      Author: kaveh
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <vector>
#include <set>

namespace utl {

typedef int32_t id;

class node {
public:
	std::set<id> outs;
	std::set<id> ins;

	const std::set<id>& getouts() {return outs;}
	const std::set<id>& getins() {return ins;}
};

class graph {
public:
	std::vector<node> nodevec;

	id create_node();
	bool add_edge(id n1, id n2);
	bool remove_edge(id n1, id n2);
	node& getnode(id nodeid);
};

}

#endif /* GRAPH_H_ */
