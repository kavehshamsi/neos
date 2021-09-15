#!/usr/bin/python2.7

from circuit import *
import random
import sys
import copy
import argparse
import networkx as nx
import math
import matplotlib.pyplot as plt
from networkx.algorithms import community
from networkx.drawing.nx_agraph import graphviz_layout


def lock_random_xor(cir, num_keys, key=int(0)):

    wire_ids = list(cir.wires())
    locked_nets = random.sample(wire_ids, num_keys)
    num_added_keys = 0
    i = 0
    for net in locked_nets:
        if key >> i == 0:
            gid = insert_gate(cir, 'xor', net, 'enc{0}'.format(num_added_keys))
        else:
            gid = insert_gate(cir, 'xnor', net, 'enc{0}'.format(num_added_keys))
        num_added_keys += 1
        i += 1
        kid = cir.add_key_wire()
        cir.gp.add_edge(kid, gid)
    return


def add_key_mux(cir, nid, other_muxins):

    left_wid, right_wid = cir.open_wire(nid, 'enc{0}'.format(cir.num_keys()))
    muxins = [left_wid] + other_muxins
    muxsize = len(other_muxins) + 1
    num_select = int(math.log(muxsize, 2)) + 1

    layer_keys = dict()
    for ns in range(num_select):
        layer_keys[ns] = cir.add_key_wire()

    level = dict()
    for muxin in muxins:
        level[muxin] = 0

    cur_layer = list(muxins)
    next_layer = list()
    layer_count = 0
    while len(cur_layer) > 1:

        muxin0 = cur_layer.pop()
        muxin1 = cur_layer.pop()

        if len(cur_layer) == 0 and len(next_layer) == 0:
            muxout = right_wid
            cir.add_gate_wids(gfun.MUX, muxout, [layer_keys[layer_count], muxin0, muxin1])
            break

        muxout = cir.add_wire(ntype.INTER)
        next_layer.append(muxout)
        cir.add_gate_wids(gfun.MUX, muxout, [layer_keys[layer_count], muxin0, muxin1])

        if len(cur_layer) == 1:
            cur_layer += next_layer
            layer_count += 1
            del next_layer[:]

    return right_wid


def add_key_gate(cir, gtype, nid):
    gid = insert_gate(cir, gtype, nid, 'enc{0}'.format(cir.num_keys()))
    kid = cir.add_key_wire()
    cir.gp.add_edge(kid, gid)
    return gid


def insert_gate(cir, gatefun, wid, wname='opened'):
    assert(cir.is_wire(wid))
    left_wid, right_wid = cir.open_wire(wid, wname)

    inputs = [left_wid]

    return cir.add_gate_wids(gatefun, right_wid, inputs)


# lock %intensity layeres with #step distance
def lock_layered(cir, interlayer_intensity, layer_step, mux_size = 0):
    orig_num_gates = cir.num_gates

    layer_list = [set()]
    visited = set()

    que = list()

    for in_id in cir.in_ids:
        out_set = set(cir.fanouts(in_id))
        layer_list[0].update(out_set)
        visited.update(set(cir.fanouts(in_id)))

    cur_layer = 0
    while True:
        term = True
        for gid in layer_list[cur_layer]:
            for fanout in cir.fanouts(cir.fanout(gid)):
                if fanout not in visited:
                    term = False
                    visited.add(fanout)
                    if len(layer_list)-1 == cur_layer:
                        layer_list.append(set())
                    layer_list[cur_layer+1].add(fanout)

        if term:
            break

        cur_layer += 1

    for i in range(0, len(layer_list), layer_step):
        print('l{0} : {1}'.format(i, len(layer_list[i])), end=' ')
        num_added_keys = int(len(layer_list[i])*interlayer_intensity)
        print(' nk: {0}'.format(num_added_keys))
        locked_gates = random.sample(layer_list[i], num_added_keys)
        xor_outs = list()
        for gid in locked_gates:
            xor_outs.append(cir.fanout(add_key_gate(cir, 'XOR', cir.fanout(gid))) )

        # add muxes for shuffling
        if len(xor_outs) >= mux_size:
            for xor_out in xor_outs:
                xor_outs_shuffle = list(xor_outs)
                random.shuffle(xor_outs_shuffle)
                xor_outs_shuffle.remove(xor_out)
                muxins = list()
                for muxin in xor_outs_shuffle:
                    skip = False

                    for wid in cir.wtrans_fanoutws(xor_out):
                        for succ in cir.wfanoutws(wid):
                            if wid == xor_out:
                                skip = True

                    if not skip:
                        muxins.append(muxin)
                        if len(muxins) == mux_size - 1:
                            add_key_mux(cir, xor_out, muxins)
                            break


    return


def partition_ckt(cir, specs):
    p = Part(cir, [4, 4])
    p.topsort_partitioning(int(specs[0]))
    # p.iterative_partitioning(5000)
    p.partition_stats()

    # itp.iterative_partitioning(100)
    # itp.partition_stats()
    p.lock_partition()

    return

import time

# a class for storing partitioning data
class Part:

    # specs : [target_ins, target_outs, target_gates]
    def __init__(self, cir, specs):

        self.cir = cir
        self.specs = specs
        # partitions data struct is dict(set())
        self.partitions = dict()
        self.gate2part = dict()
        self.part_ins = dict()
        self.part_outs = dict()

    def lock_partition(self):
        MUXIN_SIZE = 2

        self.partition_stats()
        xor_locked = set()
        mux_locked = set()

        for pid, cluster in self.partitions.items():
            for inid in self.part_ins[pid]:
                if inid not in xor_locked:
                    add_key_gate(self.cir, gfun.XOR, inid)
                    xor_locked.add(inid)

            for inid in self.part_outs[pid]:

                if inid not in mux_locked and len(self.part_outs[pid]) >= MUXIN_SIZE:
                    outs = set(self.part_outs[pid])
                    outs.remove(inid)
                    muxins = random.sample(outs, MUXIN_SIZE - 1)
                    skip = False
                    for muxin in muxins:
                        for wid in self.cir.wtrans_fanoutws(inid):
                            for succ in self.cir.wfanoutws(wid):
                                if succ == muxin:
                                    skip = True
                                    break
                    if skip:
                        continue
                    mux_locked.add(inid)
                    add_key_mux(self.cir, inid, muxins)

        return

    def iterative_partitioning(self, steps):

        self.partitions.clear()
        self.gate2part.clear()

        for gid in self.cir.gates():
            self.add_partition([gid])

        step = 0

        random.seed(time.time())

        cost = sys.maxsize
        while step < steps:
            step += 1
            while True:
                pid2, gid2 = self.migrate_one_node()
                if pid2 is not None:
                    break
            cand_cost = self.get_score(self.partitions)
            if cand_cost < cost:
                print('score {0}'.format(cand_cost))
                cost = cand_cost
            else:
                self.revert_update(pid2, gid2)

        return

    def migrate_one_node(self):
        pid1 = random.choice(list(self.partitions.keys()))
        pid2 = None; gid2 = None
        done = False
        gids = list(self.partitions[pid1])
        random.shuffle(gids)
        for gid1 in gids:
            ngs = list(self.cir.neighborgs(gid1))
            random.shuffle(ngs)
            for gid2 in ngs:
                if gid2 not in self.partitions[pid1]:
                    # migrate node
                    pid2 = self.gate2part[gid2]
                    self.partitions[pid1].add(gid2)
                    self.gate2part[gid2] = pid1
                    self.partitions[pid2].remove(gid2)
                    if len(self.partitions[pid2]) == 0:
                        del self.partitions[pid2]
                        done = True
                        break
            if done:
                break

        return pid2, gid2
        #return score, cand_partitions, cand_gate2part

    # move gid to its previous partition pid
    def revert_update(self, pid, gid):
        self.partitions[self.gate2part[gid]].remove(gid)
        if pid not in self.partitions:
            self.partitions[pid] = set()
        self.partitions[pid].add(gid)
        self.gate2part[gid] = pid
        return

    def topsort_partitioning(self, num_slices):

        sorted_gates = list()
        for nd in networkx.topological_sort(self.cir.gp):
            if self.cir.is_gate(nd):
                sorted_gates.append(nd)

        slice_len = len(sorted_gates) / num_slices + 1

        for gate_list in [sorted_gates[i:i + slice_len] for i in range(0, len(sorted_gates), slice_len)]:
            self.add_partition(gate_list)

        return

    def add_partition(self, gates):
        part_set = set()
        pid = self.new_partition_id()
        for gid in gates:
            part_set.add(gid)
            self.gate2part[gid] = pid
        self.partitions[pid] = part_set
        return

    def new_partition_id(self):
        pid = len(self.partitions)
        while pid in self.partitions:
            pid += 1
        return pid

    # compute cost function over partition
    def get_score(self, partitions):
        cost = 0
        for pid, cluster in partitions.items():
            outs = 0
            ins = 0
            in_wires = set(); out_wires = set()
            for gid in cluster:
                for wid in self.cir.fanins(gid):
                    if self.cir.is_input(wid) or self.cir.fanin(wid) not in cluster:
                        in_wires.add(wid)
                        ins += 1
                for wid in self.cir.fanouts(gid):
                    if self.cir.is_output(wid):
                        outs += 1
                        out_wires.add(wid)
                    else:
                        for gid2 in self.cir.fanouts(wid):
                            if gid2 not in cluster:
                                outs += 1
                                out_wires.add(wid)
                                break

            cost_cluster = (abs(self.specs[0] - ins)**3 + abs(self.specs[1] - outs)**3)
            cost += cost_cluster

        return cost

    def partition_stats(self):

        self.part_ins.clear()
        self.part_outs.clear()
        c = 0
        for pid, cluster in self.partitions.items():
            self.part_ins[pid] = set()
            self.part_outs[pid] = set()
            outs = 0
            ins = 0
            in_wires = set(); out_wires = set()
            for gid in cluster:
                for wid in self.cir.fanins(gid):
                    if self.cir.is_input(wid) or self.cir.fanin(wid) not in cluster:
                        in_wires.add(wid)
                        self.part_ins[pid].add(wid)
                        ins += 1
                for wid in self.cir.fanouts(gid):
                    if self.cir.is_output(wid):
                        outs += 1
                        out_wires.add(wid)
                        self.part_outs[pid].add(wid)
                    else:
                        for gid2 in self.cir.fanouts(wid):
                            if gid2 not in cluster:
                                outs += 1
                                out_wires.add(wid)
                                self.part_outs[pid].add(wid)
                                break

            print('\ncluster: {0}, gates: {1}, ins: {2}, outs: {3}'.format(c, len(cluster), ins, outs))
            print('cost : {0}'.format(abs(self.specs[0] - ins) + abs(self.specs[1] - outs)))

            # print 'inputs: '
            # for in_wire in in_wires:
            #     print self.cir.name(in_wire),
            # print '\noutputs:'
            # for out_wire in out_wires:
            #     print self.cir.name(out_wire),

            # for inwid in in_wires:
            #     add_key_gate(cir, gfun.XOR, inwid)

            c += 1
        return self.cir


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='lock circuits in bench format.')
    parser.add_argument('-m', metavar='method', type=str, help='encryption method')
    parser.add_argument('simfile', metavar='<sim_file>', help='original circuit bench file')
    parser.add_argument('encfile', metavar='<enc_file>', help='encrypted circuit bench file to create')
    parser.add_argument('specs', nargs='*', metavar='[specs*]', help='encryption specs')

    args = vars(parser.parse_args(sys.argv[1:]))
    # print args
    sim_ckt = Circuit(args['simfile'])
    enc_ckt = copy.deepcopy(sim_ckt)
    # lock_random_xor(c1, 3)
    method = args['m']
    if method == 'rnd':
        lock_random_xor(enc_ckt, int(args['specs'][0]))
    elif method == 'lr':
        lock_layered(enc_ckt, float(args['specs'][0]), int(args['specs'][1]), int(args['specs'][1]))
    elif method == 'part':
        partition_ckt(enc_ckt, args['specs'])
    else:
        print('unknown method', method, '. options are : [rnd, lr]')
        exit(1)

    print('num added keys: {0}'.format(enc_ckt.num_keys()))
    overhead = float(enc_ckt.num_gates - sim_ckt.num_gates) / sim_ckt.num_gates
    print('gate overhead: {0:.2f}%'.format(overhead*100))

    enc_ckt.write_bench(args['encfile'])
    
    