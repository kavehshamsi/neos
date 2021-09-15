#!/usr/bin/python

import gennet
from circuit import *
import sys
import os
import re
import random


# randomly adds dffs and feedbacks to make circuit sequential
def add_dffs(cir, dff_num):
    added_ffs = 0

    while added_ffs < dff_num:
        gid1 = random.sample(list(cir.gates()), 1)[0]
        fanins = list(cir.gtrans_fanings(gid1))
        if len(fanins) > 2:
            gid2 = random.sample(fanins, 1)[0]
            if (cir.gatefun(gid2) != gfun.XOR and
                    cir.gatefun(gid2) != gfun.XNOR and
                    cir.gatefun(gid2) != gfun.BUF and
                    cir.gatefun(gid2) != gfun.DFF and
                    cir.gatefun(gid2) != gfun.NOT):

                dffout = cir.insert_gate(gfun.DFF, cir.fanout(gid2), "ns{0}".format(added_ffs))
                cir.add_edge(cir.fanout(dffout), gid2)
                added_ffs += 1
    return


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print('usage : getrandseq <input_width> <output_width> <state_width> <output_file>')
        exit()

    tmp_file = './__getrandseq_tmp.bench'
    gennet.genrandnet(int(sys.argv[1]), int(sys.argv[2]), tmp_file, 0.2)

    cir = Circuit(tmp_file)
    # print cir.to_bench()

    add_dffs(cir, int(sys.argv[3]))

    cir.write_bench(sys.argv[4])

    cmd = 'rm {0}'.format(tmp_file)
    print(cmd)
    os.system(cmd)
