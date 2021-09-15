
# create circuits resembling block ciphers with tunable parameters

from circuit import *
import sys
import os
import gennet
import random


def create_spn(shared_key, num_box, box_inlen, num_rounds, mixing_layer):

    sbox_cir_file = './_$randnet_tmp.bench'

    input_len = num_box * box_inlen
    key_len = input_len

    x = [0] * input_len
    xnext = [0] * input_len
    y = [0] * input_len
    k = [0] * input_len

    enc_ckt = Circuit()

    # build permutation
    perm = [i for i in range(input_len)]
    random.shuffle(perm)
    print(('bit permutation: ', perm))

    for r in range(num_rounds):
        if r == 0:
            for xindex in range(input_len):
                x[xindex] = enc_ckt.add_wire(ntype.IN, 'in{0}'.format(xindex))

        for xindex in range(input_len):
            if not shared_key or r == 0:
                k[xindex] = enc_ckt.add_key()
            out = enc_ckt.add_wire(ntype.INTER, 'sxor{0}_{1}'.format(xindex, r))
            if mixing_layer == 'xor':
                enc_ckt.add_gate_wids(gfun.XOR, out, [x[xindex], k[xindex]])
            elif mixing_layer == 'and':
                enc_ckt.add_gate_wids(gfun.AND, out, [x[xindex], k[xindex]])
            elif mixing_layer == 'or':
                enc_ckt.add_gate_wids(gfun.OR, out, [x[xindex], k[xindex]])
            elif mixing_layer == 'mux':
                secindex = random.randint(0, input_len-1)
                enc_ckt.add_gate_wids(gfun.MUX, out, [ k[xindex], x[xindex], x[secindex] ])
            else:
                raise Exception('bad mixing layer {0}\n'.format(mixing_layer))
            xnext[xindex] = out

        x = xnext

        # one sbox circuit
        gennet.genrandnet(box_inlen, box_inlen, sbox_cir_file, 0)
        box_ckt = Circuit(sbox_cir_file)
        os.remove(sbox_cir_file)

        for nb in range(num_box):
            connMap = dict()
            for bindex in range(box_inlen):
                connMap[box_ckt.find_wire('in{0}'.format(bindex))] = x[nb * box_inlen + bindex]
            connMap = enc_ckt.add_circuit(box_ckt, connMap)

            boindex = 0
            for oid in box_ckt.out_ids:
                yindex = nb * box_inlen + boindex
                y[yindex] = connMap[oid]
                xindex = perm[yindex]
                x[xindex] = y[yindex]
                print('at round ', r)
                if r == num_rounds - 1:
                    print('setting', enc_ckt.name(connMap[oid]), 'to out')
                    enc_ckt.set_wiretype(connMap[oid], ntype.OUT)
                    enc_ckt.set_wirename(connMap[oid], 'out{0}'.format(yindex))
                boindex += 1

    # enc_ckt.write_bench()

    sim_ckt = copy.deepcopy(enc_ckt)
    valMap = dict()
    for kid in sim_ckt.key_ids:
        if mixing_layer == 'xor':
            valMap[kid] = random.randint(0, 1)
        elif mixing_layer == 'or' or mixing_layer == 'mux':
            valMap[kid] = 0
        elif mixing_layer == 'and':
            valMap[kid] = 1
    sim_ckt.simplify(valMap)
    # sim_ckt.write_bench()

    return sim_ckt, enc_ckt


if __name__ == '__main__':

    if len(sys.argv) != 8:
        print('usage: blocks.py <shared_key> <box_len> <box_num> <round> <mixing_layer> <sim_file> <enc_file>')
        exit(1)

    sk = bool(sys.argv[1])
    bl = int(sys.argv[2])
    bn = int(sys.argv[3])
    rn = int(sys.argv[4])
    ml = sys.argv[5]

    sim_file = sys.argv[6]
    enc_file = sys.argv[7]

    sim_ckt, enc_ckt = create_spn(sk, bn, bl, rn, ml)

    sim_ckt.write_bench(sim_file)
    enc_ckt.write_bench(enc_file)
    
