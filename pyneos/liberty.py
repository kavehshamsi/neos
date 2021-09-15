
# methods for translating instances to primitives gates in verilog netlists
# for now based on using an instance translation table
# TODO: support liberty libraries


from circuit import *
from vparse import *
import re

common_gates_table = """
AND A B Y -> Y = and(A, B)
NAND A B Y -> Y = nand(A, B)
NOR A B Y -> Y = nor(A, B)
OR A B Y -> Y = or(A, B)
BUF A Y -> Y = buf(A)
NOT A Y -> Y = not(A)
XOR A B Y -> Y = xor(A, B)
XNOR A B Y -> Y = xnor(A, B)
DFFSR Q C R S D -> Q = dff(D)
\$_DLATCH_P_ Q E D -> Q = dff(D)
DFF Q C D -> Q = dff(D)
"""

def translate_instances(cir, translation_table = common_gates_table):

    instFun2specs = dict()

    for ln in translation_table.split('\n'):
        ln = ln.strip()
        if len(ln.split('->')) == 2:
            # print ln.split('->')
            inst_str, prim_str = ln.split('->')
            inst_specs = inst_str.rstrip().split(' ')
            # print inst_specs
            inst_name = inst_specs[0]
            inst_ports = inst_specs[1:]
            prim_str = prim_str.replace(' ', '')
            prim_list = re.split(r'=|\(|\)|,', prim_str)
            prim_out = prim_list[0]
            prim_fun = prim_list[1]
            prim_fanins = prim_list[2:-1]

            # add to the dict
            instFun2specs[inst_name] = [inst_ports, prim_fun, prim_out, prim_fanins]
            # print [inst_ports, prim_fun, prim_out, prim_fanins]

    to_keep = list()
    for instid in copy.deepcopy(cir.instances()):
        cell_name = cir.gp.nodes[instid]['cell_name']
        port_inds = cir.gp.nodes[instid]['port_inds']
        port_wids = cir.gp.nodes[instid]['port_wids']
        cellobj = cir.cell_mgr.cell_dict[cell_name]

        if cell_name in instFun2specs:
            inst_ports, prim_fun, prim_out, prim_fanins = instFun2specs[cell_name]
            fanin_ids = list()
            fanout_id = -1
            for i in range(len(port_inds)):
                wid = port_wids[i]
                port_name = cellobj.port_names[i]
                assert(port_name in inst_ports)
                if port_name in prim_fanins:
                    fanin_ids.append(wid)
                else:
                    if port_name == prim_out:
                        fanout_id = wid
            # print prim_fun, fanout_id, fanin_ids
            cir.remove_instance(instid)
            cir.add_gate_wids(prim_fun, fanout_id, fanin_ids)

    return


if __name__ == '__main__':

    text = open(sys.argv[1]).read()

    start_time = time.time()
    T = vlog_parser.parse(text)

    print(('%s' % (time.time() - start_time)))

    trans = vlogTransformer()
    trans.transform(T)

    # translate primitive instances
    for module in trans.modules:
        start_time = time.time()
        translate_instances(module)
        print(('translation took : %s' % (time.time() - start_time)))
        start_time = time.time()
        module.remove_bufs()
        print(('buf removal took : %s' % (time.time() - start_time)))
        print((module.stats()))
        # print module.verilog_string()

    trans.modules[0].write_bench(sys.argv[2])

