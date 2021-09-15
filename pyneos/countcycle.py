#!/usr/bin/python

import gennet
from circuit import *
import sys
import networkx as nx
import os
import re
import random

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print('usage : countcycle <input_file> <cycle limit>')
        exit()

    cir = Circuit(sys.argv[1])
    cyc_limit = int(sys.argv[2])
    # print cir.to_bench()

    cyc_count = 0
    for c in nx.simple_cycles(cir.gp):
        print('\r', end='')
        print(cyc_count, end='')
        cyc_count += 1
        if cyc_count == cyc_limit:
            print('\n')
            break
    print('\n')


