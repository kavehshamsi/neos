/*
 * graph.h
 *
 *  Created on: Nov 24, 2017
 *      Author: kaveh
 */

#include <utl/graph.h>

namespace utl {

id graph::create_node() {
	nodevec.push_back(node());
	return nodevec.size()-1;
}

node& graph::getnode(id n) {
	return nodevec[n];
}

bool graph::add_edge(id n1, id n2) {
	if (n1 > (id)nodevec.size() || n2 > (id)nodevec.size())
		return false;
	nodevec[n1].outs.insert(n2);
	return true;
}

bool graph::remove_edge(id n1, id n2) {
	if (n1 > (id)nodevec.size())
		return false;
	else {
		if (nodevec[n1].outs.erase(n2) == 0)
			return false;
		else return true;
	}
}

}
