#!/usr/bin/python

# A Python circuit library with a close API to neos. 

import sys
import os
import re
import networkx as nx
import copy

from enum import Enum, IntEnum

class ntype(IntEnum):
    IN = 1
    INTER = 2
    OUT = 3
    KEY = 4
    GATE = 5
    INST = 6


class portdir(IntEnum):
    INOUT = 0
    IN = 1
    OUT = 2

class ndattr(IntEnum):
    NAME = 0
    NODETYPE = 1
    GFUNCTION = 2
    CELL_NAME = 3
    PORT_INDS = 4
    PORT_WIDS = 5

class gfun(IntEnum):
    NAND = 0
    AND = 1
    NOR = 2
    OR = 3
    XOR = 4
    XNOR = 5
    BUF = 6
    NOT = 7
    DFF = 8
    MUX = 9
    INST = 10


class Cell:
    def __init__(self, cell_name, cell_port_names, port_dirs):
        self.cell_name = cell_name
        self.port_names = cell_port_names
        self.port_dirs = port_dirs

    def __str__(self):
        retstr = 'Cell: name: {0}, ports ['.format(self.cell_name)
        for i in range(len(self.port_names)):
            retstr += ' {0}:{1} '.format(self.port_names[i], self.port_dirs[i])
        retstr += ']'
        return retstr


class CellLibrary:
    def __init__(self):
        self.cell_dict = dict()
        return

    def add_cell(self, cell_name, cell_ports, port_dirs):
        port_inds = list()
        if cell_name not in self.cell_dict:
            cell = Cell(cell_name, cell_ports, port_dirs)
            self.cell_dict[cell_name] = cell
            for ind in range(len(cell_ports)):
                port_inds.append(ind)
        else:
            for cell_port in cell_ports:
                port_inds.append(self.cell_dict[cell_name].port_names.index(cell_port))
        return port_inds

    def remove_cell(self, cell_name):
        self.cells.pop(cell_name)

    def __str__(self):
        retstr = ''
        for cell in self.cell_dict:
            retstr += '{0}\n'.format(cell)
        return retstr



# some helper methods
def list_strs(strings, delimiter):
    retstr = str()
    i = 1
    for string in strings:
        retstr += string
        if i != len(strings):
            retstr += delimiter
        if i % 10 == 0:
            retstr += '\n'
        i += 1
    return retstr


class NetlistError(Exception):
    def __init__(self, message):
        self.message = message
        return


class Circuit:

    def __init__(self, bench_file = None):

        self.circuit_name = str()

        self.in_ids = set()
        self.out_ids = set()
        self.key_ids = set()

        self.gate_ids = set()
        self.dff_ids = set()
        self.inst_ids = set()

        self.name2id = dict()

        self.num_nodes = 0
        self.num_gates = 0
        self.num_inst = 0
        self.max_id = -1


        self.gp = nx.DiGraph()

        self.topsorted_gates = list()
        self.topsorted_wires = list()
        self.removed_ids = list()
        self.simulated = False
        self.instances_linked = False

        self.const0 = None
        self.const1 = None
        self.vdd_count = 0

        self.cell_mgr = None

        if bench_file != None:
            self.read_bench(bench_file)
        
        
        # print self.to_string()
        return

    def read_bench(self, bench_file):
        self.circuit_name = os.path.basename(bench_file).replace('.bench', '')
        fin = open(bench_file, 'r')
        for ln in fin:
            ln = ln.replace(' ', '')
            ln = ln.strip('\n')
            if 'INPUT(' in ln:
                wire_name = re.split('\(|\)', ln)[-2]
                if wire_name[0:8] == 'keyinput':
                    self.add_wire(ntype.KEY, wire_name)
                else:                    
                    self.add_wire(ntype.IN, wire_name)

            elif 'OUTPUT' in ln:
                wire_name = re.split('\(|\)', ln)[-2]
                self.add_wire(ntype.OUT, wire_name)

            elif '=' in ln:
                gate_strs = re.split('\(|\)|,|=', ln)
                if len(gate_strs) == 2:
                    self.add_gate(self.new_gname(), gate_strs[0], gate_strs[1])
                else:
                    self.add_gate(gate_strs[1], gate_strs[0], gate_strs[2:-1])
        fin.close()
        return

    def add_key_wire(self):
        kid = self.add_wire(ntype.KEY, 'keyinput{0}'.format(self.num_keys()))
        return kid

    def predict_new_id(self):
        if len(self.removed_ids) != 0:
            return self.removed_ids[-1]
        return self.max_id + 1

    def get_new_id(self):
        if len(self.removed_ids) != 0:
            return self.removed_ids.pop()
        self.max_id += 1
        return self.max_id

    def remove_id(self, nid):
        if nid == self.max_id:
            self.max_id -= 1
        else:
            self.removed_ids.append(nid)
        return

    def remove_name(self, node_name):
        self.name2id.pop(node_name)
        return

    def add_key(self):
        return self.add_wire(ntype.KEY, 'keyinput{0}'.format(self.num_keys()))

    def add_wire_nocheck(self, wire_type, wire_name=None):

        wid = self.get_new_id()

        self.name2id[wire_name] = wid

        self.gp.add_node(wid)
        nx.set_node_attributes(self.gp, {wid : { ndattr.NODETYPE : wire_type,
                                         ndattr.NAME : wire_name } }
                               )

        if wire_type == ntype.IN:
            self.in_ids.add(wid)
        elif wire_type == ntype.OUT:
            self.out_ids.add(wid)
        elif wire_type == ntype.KEY:
            self.key_ids.add(wid)

        self.num_nodes += 1
        return wid

    def add_wire(self, wire_type, wire_name=None):
        if wire_name is None:
            wire_name = self.new_wname()

        if wire_name == 'gnd':
            return self.get_const0_wid()
        elif wire_name == 'vdd':
            return self.get_const1_wid()

        if wire_name in self.name2id:
            raise Exception('wire {0} already exists'.format(wire_name))

        return self.add_wire_nocheck(wire_type, wire_name)

    def add_gate_wids(self, gate_fun, output_id, input_ids):

        gid = self.get_new_id()
        if isinstance(gate_fun, gfun):
            gateFunEnum = gate_fun
        else:
            gateFunEnum = gfun[gate_fun.upper()]

        self.gp.add_node(gid)
        nx.set_node_attributes(self.gp, {gid: { ndattr.NODETYPE : ntype.GATE,
                                                ndattr.GFUNCTION : gateFunEnum,
                                                ndattr.NAME : 'g{0}'.format(self.num_gates) } }
                               )
        self.gate_ids.add(gid)

        if gateFunEnum == gfun.DFF :
            self.dff_ids.add(gid)

        for input_id in input_ids:
            self.gp.add_edge(input_id, gid)
        self.gp.add_edge(gid, output_id)

        self.num_nodes += 1
        self.num_gates += 1
        return gid

    def add_gate(self, gate_fun, output_name, input_names):

        gid = self.get_new_id()

        if isinstance(gate_fun, gfun):
            gateFunEnum = gate_fun
        else:
            gateFunEnum = gfun[gate_fun.upper()]
        self.gp.add_node(gid);
        nx.set_node_attributes(self.gp, {gid : {ndattr.NODETYPE : ntype.GATE,
                                                ndattr.GFUNCTION : gateFunEnum,
                                                ndattr.NAME : 'g{0}'.format(self.num_gates)} }
                               )
        self.gate_ids.add(gid)

        if gateFunEnum == gfun.DFF:
            self.dff_ids.add(gid)

        self.num_nodes += 1

        for input_name in input_names:
            if input_name not in self.name2id:
                self.add_wire(ntype.INTER, input_name)

            fanin_wid = self.name2id[input_name]
            self.gp.add_edge(fanin_wid, gid)


        if output_name not in self.name2id:
            self.add_wire(ntype.INTER, output_name)

        fanout_wid = self.name2id[output_name]
        self.gp.add_edge(gid, fanout_wid)

        self.num_gates += 1

        return gid

    def stats(self):
        retstr = str()
        retstr += 'inputs: {0}   outputs: {1}   gates: {2}    dffs: {3}   instances: {4}'.format(
            self.num_ins(), self.num_outs(), self.num_gates, self.num_dffs(), self.num_inst()
        )
        return retstr

    def add_instance(self, inst_name, cell_name, name_pairs):
        port_names = list()
        port_dirs = list()
        port_wids = list()

        for i in range(len(name_pairs)):
            port_names.append(name_pairs[i][0])
            port_dirs.append(portdir.INOUT)

        if self.cell_mgr is None:
            self.cell_mgr = CellLibrary()

        port_inds = self.cell_mgr.add_cell(cell_name, port_names, port_dirs)

        for name_pair in name_pairs:
            wid = self.find_wire(name_pair[1])
            if wid is None:
                raise NetlistError('problem finding wire ', name_pair[1])

            port_wids.append(wid)

        gid = self.get_new_id()

        if inst_name == '':
            inst_name = 'u{0}'.format(self.num_inst)

        self.gp.add_node(gid)
        nx.set_node_attributes(self.gp, {gid : {ndattr.NODETYPE : ntype.INST,
                                                ndattr.CELL_NAME : cell_name,
                                                ndattr.PORT_INDS : port_inds,
                                                ndattr.PORT_WIDS : port_wids,
                                                ndattr.NAME : inst_name} }
                               )
        self.inst_ids.add(gid)

        if inst_name in self.name2id:
            raise NetlistError('duplicate instance name {0}\n'.format(inst_name))

        self.name2id[inst_name] = gid

        for i in range(len(port_wids)):
            if port_dirs[i] == portdir.OUT:
                self.gp.add_edge(gid, port_wids[i])
            else:
                self.gp.add_edge(port_wids[i], gid)

        self.num_nodes += 1
        self.num_inst += 1

        self.instances_linked = False
        return gid

    def open_wire(self, wid, new_name='opened'):

        assert(self.nodetype(wid) != ntype.GATE)

        if new_name == 'opened':
            new_name = 'opened{0}'.format(self.num_opened_wires)

        new_id = self.add_wire(ntype.INTER, new_name)

        if self.nodetype(wid) == ntype.OUT:
            right_id = wid
            left_id = new_id
            for fanin in self.fanins(wid):
                self.gp.add_edge(fanin, new_id)
            self.clear_fanins(wid)
        else:
            right_id = new_id
            left_id = wid
            for fanout in self.fanouts(wid):
                self.gp.add_edge(new_id, fanout)
            self.clear_fanouts(wid)

        return left_id, right_id

    def to_verilog(self):
        retstr = '// verilog netlist\n'
        retstr += 'module {0}'.format(self.circuit_name)
        port_set = set(self.in_ids)
        port_set.update(self.out_ids)
        port_set.update(self.key_ids)
        retstr += '({0});\n\n'.format(self.list_wirenames(port_set, ', '))
        retstr += 'input '
        retstr += self.list_wirenames(list(self.allins()), ',') + ';\n\n'
        retstr += 'output '
        retstr += self.list_wirenames(list(self.outputs()), ',') + ';\n\n'
        retstr += 'wire '
        retstr += self.list_wirenames(list(self.inters()), ', ') + ';\n\n'

        for inst in self.instances():
            retstr += self.instance_str_vlog(inst) + '\n'
        for gid in self.gates():
            retstr += self.gate_str_vlog(gid) + '\n'

        retstr += '\nendmodule\n'
        return retstr

    def instance_str_vlog(self, instid):
        inst = self.gp.nodes[instid]
        print(inst)
        retstr = str()
        retstr += '{0} {1} ('.format(inst[ndattr.CELL_NAME], inst[ndattr.NAME])
        conn_pairs = list()
        cell = self.cell_mgr.cell_dict[inst[ndattr.CELL_NAME]]
        for i, port_ind in enumerate(inst[ndattr.PORT_INDS]):
            conn_pairs.append('.{0}({1})'.format(cell.port_names[port_ind], self.name(inst[ndattr.PORT_WIDS][i])))
        retstr += list_strs(conn_pairs, ', ') + ');'
        return retstr

    def gate_str_vlog(self, gid):
        retstr = str()
        retstr += '{0} {1} ('.format(self.gatefun(gid).name.lower(), self.name(gid))
        retstr += '{0}, {1});'.format(self.name(self.fanout(gid)), self.list_wirenames(self.fanins(gid), ', '))
        return retstr

    def list_wirenames(self, collection, delimiter):
        retstr = str()
        i = 1
        for wid in collection:
            retstr += self.name(wid)
            if i != len(collection):
                retstr += delimiter
            if i % 10 == 0:
                retstr += '\n'
            i += 1
        return retstr

    def to_bench(self):
        retstr = '# bench netlist\n'
        for wid in self.in_ids:
            retstr += 'INPUT({0})\n'.format(self.name(wid))
        for wid in sorted(self.key_ids):
            retstr += 'INPUT({0})\n'.format(self.name(wid))
        for wid in self.out_ids:
            retstr += 'OUTPUT({0})\n'.format(self.name(wid))
        for wid in self.instances():
            retstr += '{0}\n'.format(self.inst_str(wid))
        for nid in self.gp.nodes():
            if self.is_gate(nid):
                retstr += self.gate_str(nid)
        return retstr

    def write_bench(self, out_fn = None):
        if out_fn is None:
            print((self.to_bench()))
        else:
            fobj = open(out_fn, 'w')
            fobj.write(self.to_bench())
            fobj.close()
        return

    def gate_str(self, nid):

        fanout_name = self.name(self.fanout(nid))
        retstr = '{0} = {1} ('.format(fanout_name, self.gatefun(nid).name.lower())
        i = 0

        for fanin in self.gp.predecessors(nid):
            i += 1
            retstr += '{0}'.format(self.name(fanin))
            if i != len(list(self.gp.predecessors(nid))):
                retstr += ', '
        retstr += ')\n'
        return retstr

    def insert_gate(self, gatefun, wid, wname='opened'):
        assert(self.is_wire(wid))
        left_wid, right_wid = self.open_wire(wid, wname)

        inputs = [left_wid]

        return self.add_gate_wids(gatefun, right_wid, inputs)

    def set_wiretype(self, wid, wtype):
        old_wtype = self.nodetype(wid)
        assert old_wtype != ntype.GATE

        if old_wtype != wtype:
            if old_wtype == ntype.IN:
                self.in_ids.remove(wid)
            elif old_wtype == ntype.OUT:
                self.out_ids.remove(wid)
            elif old_wtype == ntype.KEY:
                self.key_ids.remove(wid)

            if wtype == ntype.IN:
                self.in_ids.add(wid)
            elif wtype == ntype.OUT:
                self.out_ids.add(wid)
            elif wtype == ntype.KEY:
                self.key_ids.add(wid)

            self.gp.nodes[wid]['nodetype'] = wtype

        return

    def rename_inters(self):
        i  = 0
        for wid in self.wires():
            if self.is_inter(wid) and (self.fanout(wid) not in self.dff_ids)\
                    and (self.name(wid) != 'vdd') and (self.name(wid) != 'gnd') and (self.fanin(wid) not in self.dff_ids):
                while self.find_wire('n{0}'.format(i)) != None:
                    i += 1
                self.set_wirename(wid, 'n{0}'.format(i))
                i += 1

    def new_wname(self):
        index = self.predict_new_id()
        while 'n{0}'.format(index) in self.name2id:
            index += 1
        return 'n{0}'.format(index)

    def new_gname(self):
        index = self.predict_new_id()
        while 'g{0}'.format(index) in self.name2id:
            index += 1
        return 'g{0}'.format(index)

    def add_circuit(self, cir, connMap, postfix=None):
        for wid in cir.wires():
            if wid not in connMap:
                nm = self.new_wname() if postfix is None else (cir.name(wid) + postfix)
                connMap[wid] = self.add_wire(cir.nodetype(wid), nm)

        for gid in cir.gates():
            newfanins = list()
            for cirfanin in cir.fanins(gid):
                newfanins.append(connMap[cirfanin])
            newfanout = connMap[cir.fanout(gid)]
            self.add_gate_wids(cir.gatefun(gid), newfanout, newfanins)

        return connMap

    def join_outputs(self, fun, oname=None, joinset=None):
        if joinset is None:
            joinset = copy.copy(self.outputs())

        for oid in joinset:
            assert(self.is_output(oid))
            self.set_wiretype(oid, ntype.INTER)

        nm = self.new_wname() if oname is None else oname
        noid = self.add_wire(ntype.OUT, nm)

        if len(joinset) == 1:
            if fun == gfun.NOT or fun == gfun.NAND or fun == gfun.XNOR or fun == gfun.NOR:
                self.add_gate_wids(gfun.NOT, noid, list(joinset))
            else:
                self.add_gate_wids(gfun.BUF, noid, list(joinset))
        else:
            self.add_gate_wids(fun, noid, list(joinset))

        return noid

    def gates(self):
        for nid in self.gp.nodes():
            if self.is_gate(nid):
                yield nid

    def wires(self):
        for nid in self.gp.nodes():
            if self.is_wire(nid):
                yield nid

    def instances(self):
        return self.inst_ids

    def ports(self):
        for nid in self.gp.nodes():
            if self.is_port(nid):
                yield nid

    def inputs(self):
        return self.in_ids

    def outputs(self):
        return self.out_ids

    def keys(self):
        return self.key_ids

    def allins(self):
        return list(self.in_ids) + list(self.key_ids)

    def allports(self):
        ret = set(self.allins())
        ret.update(self.outputs())
        return ret

    def inters(self):
        for nid in self.gp.nodes():
            if self.is_inter(nid):
                yield nid

    def find_wcheck(self, wname):
        if wname not in self.name2id:
            raise Exception('wire {0} not found'.format(wname))
        else:
            return self.name2id[wname]

    def find_wire(self, wname):
        if wname in self.name2id:
            return self.name2id[wname]
        else:
            return None

    def get_const1_wid(self):
        if self.const1 == None:
            self.const1 = self.add_wire_nocheck(ntype.INTER, 'vdd')
        return self.const1

    def get_const0_wid(self):
        if self.const0 == None:
            self.const0 = self.add_wire_nocheck(ntype.INTER, 'gnd')
        return self.const0

    def fanout(self, id):
        if len(list(self.gp.successors(id))) == 0:
            return None
        return list(self.gp.successors(id))[0]

    def fanouts(self, id):
        return self.gp.successors(id)

    def gtrans_fanoutgs(self, gid):
        for nid in nx.bfs_successors(self.gp, gid):
            if self.is_gate(nid):
                yield nid

    def gtrans_fanings(self, gid):
        for nid in nx.bfs_predecessors(self.gp, gid):
            if self.is_gate(nid):
                yield nid

    def wtrans_fanoutws(self, wid):
        for nid in nx.bfs_successors(self.gp, wid):
            if self.is_wire(nid):
                yield nid

    def wtrans_faninws(self, wid):
        for nid in  nx.bfs_predecessors(self.gp, wid):
            if self.is_wire(nid):
                yield nid

    def wfaninws(self, wid):
        assert self.is_wire(wid)
        retset = set()
        if self.fanin(wid) is not None:
            for wid2 in self.fanins(self.fanin(wid)):
                retset.add(wid2)
        return retset

    def gfanings(self, gid):
        assert self.is_gate(gid)
        retset = set()
        for wid in self.fanins(gid):
            if self.fanin(wid) is not None:
                retset.add(self.fanin(wid))
        return retset

    def neighbors(self, gid):
        retset = list(self.fanins(gid))
        retset.append(self.fanouts(gid))
        return retset

    def neighborgs(self, gid):
        assert self.is_gate(gid)
        retset = self.gfanings(gid)
        retset.update(self.gfanoutgs(gid))
        return retset

    def gfanoutgs(self, gid):
        assert self.is_gate(gid)
        retset = set()
        wid = self.fanout(gid)
        return list(self.fanouts(wid))

    def wfanoutws(self, wid):
        assert self.is_wire(wid)
        for gid in self.fanouts(wid):
            yield self.fanout(gid)

    def fanin(self, id):
        if len(list(self.gp.predecessors(id))) == 0:
            return None
        return list(self.gp.predecessors(id))[0]

    def fanins(self, id):
        return list(self.gp.predecessors(id))

    def add_edge(self, fanin, fanout):
        num_nodes_prev = self.gp.number_of_nodes()
        self.gp.add_edge(fanin, fanout)
        num_nodes_post = self.gp.number_of_nodes()
        assert(num_nodes_prev == num_nodes_prev)

    def remove_gate(self, gid):

        if self.gatefun(gid) == gfun.DFF:
            self.dff_ids.remove(gid)

        self.gp.remove_node(gid)

        self.num_nodes -= 1
        self.num_gates -= 1
        self.gate_ids.remove(gid)
        self.remove_id(gid)

        return

    def remove_instance(self, gid):

        self.gp.remove_node(gid)

        self.num_nodes -= 1
        self.num_inst -= 1
        self.inst_ids.remove(gid)
        self.remove_id(gid)

        return

    def remove_wire(self, wid):

        assert self.is_wire(wid)

        if self.is_input(wid):
            self.in_ids.remove(wid)
        elif self.is_key(wid):
            self.key_ids.remove(wid)
        elif self.is_output(wid):
            self.out_ids.remove(wid)

        wname = copy.deepcopy(self.name(wid))
        self.gp.remove_node(wid)
        self.remove_id(wid)
        self.num_nodes -= 1
        self.remove_name(wname)

        return

    def remove_bufs(self):

        remove_wset = set()
        remove_gset = set()
        for gid in copy.deepcopy(self.gate_ids):
            if self.gatefun(gid) == gfun.BUF:
                gout = self.fanout(gid)
                gin = self.fanin(gid)
                if self.is_inter(gin) and not self.is_const(gin):
                    to_remove = gin
                    to_keep = gout
                elif self.is_inter(gout) and not self.is_const(gout):
                    to_remove = gout
                    to_keep = gin
                else: # skip input-output buffer
                    continue

                assert(self.gatefun(gid) == gfun.BUF)

                self.remove_gate(gid)

                rm_fanins = copy.deepcopy(self.fanins(to_remove))
                rm_fanouts = copy.deepcopy(self.fanouts(to_remove))

                for rm_fanin in rm_fanins:
                    self.add_edge(rm_fanin, to_keep)

                for rm_fanout in rm_fanouts:
                    self.add_edge(to_keep, rm_fanout)

                self.remove_wire(to_remove)

        return

    def set_gatefun(self, gid, fun):
        self.gp.nodes[gid][ndattr.GFUNCTION] = fun
        
    def trim_circuit_to(self, live_outs):
        Q = list(live_outs)
        wvisited = set()
        gvisited = set()
        
        while len(Q) != 0:
            curwid = Q.pop(0)
            wvisited.add(curwid)
            
            gin = self.fanin(curwid)
            if gin is not None:
                gvisited.add(gin)
                for win in self.fanins(gin):
                    if win not in wvisited:
                        wvisited.add(win)
                        Q.append(win)

        for wid in list(self.wires()):
            if wid not in wvisited:
                self.remove_wire(wid)

        for gid in list(self.gates()):
            if gid not in gvisited:
                self.remove_gate(gid)

    def simplify(self, valmap):
        for wid, val in list(valmap.items()):
            for gfout in list(self.fanouts(wid)):
                gfn = self.gatefun(gfout)
                if gfn == gfun.XOR:
                    if val == 1:
                        self.set_gatefun(gfout, gfun.NOT)
                    else:
                        self.set_gatefun(gfout, gfun.BUF)
                elif gfn == gfun.XNOR:
                    if val == 1:
                        self.set_gatefun(gfout, gfun.BUF)
                    else:
                        self.set_gatefun(gfout, gfun.NOT)
                elif gfn == gfun.AND:
                    if val == 1:
                        self.set_gatefun(gfout, gfun.BUF)
                    else:
                        raise Exception('{0} gate simplification not supported'.format(gfn))

                elif gfn == gfun.OR:
                    if val == 0:
                        self.set_gatefun(gfout, gfun.BUF)
                    else:
                        raise Exception('{0} gate simplification not supported'.format(gfn))

                elif gfn == gfun.MUX:
                    muxfanins = self.fanins(gfout)
                    if muxfanins[1] != muxfanins[2]:
                        if val == 0:
                            self.gp.remove_edge(muxfanins[2], gfout)
                        else:
                            self.gp.remove_edge(muxfanins[1], gfout)
                    self.set_gatefun(gfout, gfun.BUF)
                else:
                    continue

                self.gp.remove_edge(wid, gfout)

            self.remove_wire(wid)

        return

    def clear_fanins(self, id):
        for fanin in copy.deepcopy(self.fanins(id)):
            self.gp.remove_edge(fanin, id)

    def clear_fanouts(self, id):
        for fanout in copy.deepcopy(self.fanouts(id)):
            self.gp.remove_edge(id, fanout)

    def clear_topsorts(self, id):
        self.topsorted_gates.clear()
        self.topsorted_wires.clear()        

    def is_wire(self, wid):
        return self.nodetype(wid) != ntype.GATE

    def is_inter(self, wid):
        return self.nodetype(wid) == ntype.INTER

    def is_const(self, wid):
        return (self.const1 != None and self.const1 == wid) \
            or (self.const0 != None and self.const0 == wid)

    def is_input(self, wid):
        return self.nodetype(wid) == ntype.IN

    def is_output(self, wid):
        return self.nodetype(wid) == ntype.OUT

    def is_key(self, wid):
        return self.nodetype(wid) == ntype.KEY

    def is_port(self, wid):
        return not self.is_gate(wid) and not self.is_inter(wid) and not self.is_inst(wid)

    def is_inst(self, gid):
        return self.nodetype(gid) == ntype.INST
        
    def is_gate(self, gid):
        return self.nodetype(gid) == ntype.GATE

    def name(self, id):
        return self.gp.nodes[id][ndattr.NAME]

    def set_wirename(self, id, new_name):
        del self.name2id[self.name(id)]
        self.gp.nodes[id][ndattr.NAME] = new_name
        self.name2id[new_name] = id

    def nodetype(self, id):
        return self.gp.nodes[id][ndattr.NODETYPE]

    def gatefun(self, id):
        assert(self.is_gate(id))
        return self.gp.nodes[id][ndattr.GFUNCTION]

    def num_ins(self):
        return len(self.in_ids)

    def num_dffs(self):
        return len(self.dff_ids)

    def num_inst(self):
        return len(self.instances)

    def num_outs(self):
        return len(self.out_ids)

    def num_keys(self):
        return len(self.key_ids)

    def num_ins_and_keys(self):
        return len(self.key_ids) + len(self.in_ids)

    def num_allins(self):
        return self.num_keys() + self.num_ins()
        
    def num_ports(self):
        return self.num_ins_and_keys() + self.num_outs()

    def num_wires(self):
        return self.num_nodes - self.num_gates

    def topsort(self):
        if len(self.topsorted_gates) != 0:
            return

        topsorted_nodes = nx.topological_sort(self.gp)
        for nd in topsorted_nodes:
            if self.is_gate(nd):
                self.topsorted_gates.append(nd)
            else:
                self.topsorted_wires.append(nd)
        return

    def simulate(self, sim_map):
        self.topsort()
        
        for gid in self.topsorted_gates:

            # make sure topsort is correct
            #for fanin in self.fanins(gid):
             #   assert(sim_map[fanin] != -1)

            gf = self.gatefun(gid)
            gout = self.fanout(gid)
            gins = self.fanins(gid)

            if gf == gfun.BUF:
                sim_map[gout] = sim_map[gins[0]]

            elif gf == gfun.NOT:
                sim_map[gout] = ~sim_map[gins[0]] & 1

            elif gf == gfun.XOR:
                sim_map[gout] = sim_map[gins[0]] ^ sim_map[gins[1]]
            elif gf == gfun.XNOR:
                sim_map[gout] = ~(sim_map[gins[0]] ^ sim_map[gins[1]]) & 1

            elif gf == gfun.OR or gf == gfun.NOR:
                sim_map[gout] = 0
                for gin in gins:
                    sim_map[gout] |= sim_map[gin]
                if gf == gfun.NOR:
                    sim_map[gout] = ~sim_map[gout] & 1

            elif gf == gfun.AND or gf == gfun.NAND:
                sim_map[gout] = 1
                for gin in gins:
                    sim_map[gout] &= sim_map[gin]
                if gf == gfun.NAND:
                    sim_map[gout] = ~sim_map[gout] & 1
            
            else:
                print('unsupported type', gf, 'for simulation')
                exit()
        # for id, val in self.sim_map.iteritems():
        #     print '{0} {1}'.format(self.name(id), val)

        return

    def link_instances(self, list_output_names):
        for inst in self.instances:
            for port_name in inst.portNameDict:
                if port_name in list_output_names:
                    inst.portNameDict[port_name][1] = ntype.OUT
                else:
                    inst.portNameDict[port_name][1] = ntype.IN
                    
            print(inst.portNameDict)
        
        for inst in self.instances:
            gate_ins = []
            for port_name, port_tuple in inst.portNameDict.items():
                if port_tuple[1] == ntype.IN:
                    gate_ins.append(port_tuple[0])
                elif port_tuple[1] == ntype.OUT:
                    gate_out = port_tuple[0]
            self.add_gate_wids(gfun.INST, gate_out, gate_ins)

        self.instances_linked = True
        return
        
    def translate_instances_to_primitive(self):
        import liberty
        liberty.translate_instances(self)
        return

    def read_verilog(self, verilog_file):
        import vparse
        cir_list = vparse.parse_2_circuit_list(verilog_file)
        self.__dict__ = cir_list[0].__dict__
        return
        
        

# some utility methods

def error(str):
    print(('error:', str))
    exit(1)


def init_decrypt(sim_ckt, enc_ckt):
    enc2sim_idmap = dict()

    if enc_ckt.num_ins() != sim_ckt.num_ins():
        error('input count does not match between sim and enc circuits')

    if enc_ckt.num_outs() != sim_ckt.num_outs():
        error('output count does not match between sim and enc circuits')

    for in_id in enc_ckt.in_ids:
        try:
            enc2sim_idmap[in_id] = sim_ckt.name2id[enc_ckt.name(in_id)]
        except KeyError:
            error('could not find input {0}'.format(enc_ckt.name(in_id)))

    for out_id in enc_ckt.out_ids:
        try:
            enc2sim_idmap[in_id] = sim_ckt.name2id[enc_ckt.name(in_id)]
        except KeyError:
            error('could not find output {0} in sim_ckt'.format(enc_ckt.name(in_id)))

    return enc2sim_idmap


import random

import random, argparse, pickle
import numpy as np
import sklearn
from sklearn import linear_model
from sklearn.metrics import accuracy_score
from sklearn.linear_model import SGDRegressor
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.neural_network import MLPRegressor, MLPClassifier
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import Normalizer
from sklearn import linear_model
from sklearn import tree
from sklearn.kernel_ridge import KernelRidge
from sklearn import ensemble
from sklearn.metrics import accuracy_score

if __name__ == "__main__":

    cir = Circuit(sys.argv[1])
    cir.write_bench()
    exit(1)
    # outid_list = list(cir.outputs())
    # for i in range(cir.num_outs()):
    #     out_dir = 'trimed'
    #     tcir = copy.deepcopy(cir)
    #
    #     outid = outid_list[i]
    #     tcir.trim_circuit_to([outid])
    #     tcir.write_bench(out_dir + '/{}_{}.bench'.format(cir.circuit_name, i))


    for gid in cir.gates():
        keygates = set()
        for gin in cir.fanins(gid):
            if cir.is_key(gin):
                keygates.add(gid)

        nonkeygates = (set(cir.gates())).difference(keygates)

        supernodes = dict()

        supernode_count = 0
        for nkg in list(nonkeygates):
            if nkg not in supernodes:
                supernodes[nkg] = supernode_count
                supernode_count += 1

            gneighbors = set(cir.gfanoutgs(nkg))
            gneighbors = gneighbors.union(cir.gfanings(nkg))

            for nkgfo in gneighbors:
                if nkgfo in nonkeygates:
                    supernodes[nkgfo] = supernodes[nkg]


        for sp in supernodes:
            print('sp {} -> {}'.format(cir.name(cir.fanout(sp)), supernodes[sp]))

    # sim_map = dict();
    #
    # X = []
    # Y = []
    #
    # num_patts = int(sys.argv[2])
    # out_index = int(sys.argv[3])
    #
    # for pat in range(0, num_patts):
    #     X.append(list())
    #
    #     print('x=', end='')
    #     for x in cir.inputs():
    #         b = sim_map[x] = bool(random.getrandbits(1))
    #         print(int(b), end='')
    #         X[-1].append(b)
    #
    #     cir.simulate(sim_map)
    #
    #     print(' -->  y=', end='')
    #     yid = list(cir.outputs())[out_index]
    #     b = sim_map[yid]
    #     Y.append(sim_map[yid])
    #     print(b, end='')
    #
    #     print()
    #
    # print(Y)
    # print(X)
    #
    # indices = np.arange(len(X))
    # X_train, X_test, Y_train, Y_test, indices_train, indices_test = train_test_split(X, Y, indices, test_size=0.2, random_state=42)
    #
    # clf = MLPClassifier()
    # #clf = tree.DecisionTreeClassifier()
    # clf.fit(X_train, Y_train)
    #
    # Y_pred = clf.predict(X_test)
    #
    # print(Y_pred)
    # print(Y_test)
    #
    # print(sklearn.metrics.balanced_accuracy_score(Y_test, Y_pred))
    #
    #c1.write_bench()






