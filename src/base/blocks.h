/*
 * blocks.h

 *
 *  Created on: May 18, 2016
 *      Author: kaveh
 */

#ifndef BLOCKS_H_
#define BLOCKS_H_

#include "base/circuit.h"
#include <math.h>


namespace ckt_block {

using namespace ckt;
using namespace utl;

typedef std::map<string,id> inter_map;
typedef std::map<id,id> con_map;

// switch box inherited class
class swbox: public circuit {
  public:
	idvec sel_vec;
	idvec in_vec;
	idvec out_vec;
};

// mux inherited class
class mux_t: public circuit {
  public:
	idvec sel_vec;
	idvec in_vec;
	id out;
};

// broken-up and-tree inherited class
class tree: public circuit {
public:
	idvec in_vec;
	idvec key_vec;
	id out;
};


tree get_tree(uint width, fnct gatetype);
tree and_tree(uint width);
tree eq_comparator(int width, idvec& ains, idvec& bins, id& out);
tree key_comparator(uint width, bool apply_mapping = false);
tree fixed_comparator(id width, const boolvec& key);
tree anti_sat(id width);
tree lut(uint numinputs);
tree ra_lut(int num_rows, int num_columns, int num_on_rows = -1);
tree rca_lut(int num_rows, int num_columns, int num_on_rows = -1, int num_on_columns = -1);
tree rca_lut(id& out, vector<idvec>& row_keys, vector<idvec>& cola_keys, int num_rows, int num_columns, int num_on_rows = -1, int num_on_columns = -1);

circuit encXOR();
circuit encXNOR();
circuit Inverter();
circuit encAND();
circuit encANDN();
circuit encOR();

circuit half_adder();
circuit half_adder(id& a, id& b, id& s, id& co);
circuit full_adder();
circuit full_adder(id& a, id& b, id& ci, id& s, id& co);
circuit half_bit_comparator();
circuit half_bit_comparator(id& a, id& b, id& agb);
circuit full_bit_comparator();
circuit full_bit_comparator(id& a, id& b, id& agb, id& bga, id& aeb);

// if no num_ins defined interesting things occur
mux_t nmux(int num_ins, int num_select = -1, bool iskey_controlled = true);
mux_t key_tmux(int num_ins, bool iskey_controlled = true);
mux_t key_nmux_w2mux(int num_ins, bool iskey_controlled = true);
swbox n_switchbox(uint N);

circuit bit_counter(uint N);
circuit bit_counter(uint N, idvec& ins, idvec& outs);
circuit vector_adder(uint N);
circuit vector_adder(uint N, idvec& avec, idvec& bvec, idvec& svec);
circuit vector_half_comparator(uint N);
circuit vector_half_comparator(uint N, idvec& avec, idvec& bvec, id& avgbv);
circuit sorted_vector_half_comparator(uint N);
circuit sorted_vector_half_comparator(uint N, idvec& avec, idvec& bvec, id& avgbv);
circuit bit_odd_even_sorter(uint N);

// should not need this if everything is working properly
void build_wire_InOutSets(circuit& ckt);

// for testing only
void test_all(void);

}

#endif /* ENC_H_ */
