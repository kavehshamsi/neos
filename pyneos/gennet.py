#!/usr/bin/python
import math
import sys
import random
import os
import myabc
import binascii

def genlut(num_of_ins, f):
    orig_stdout = sys.stdout
    sys.stdout = f
    print(("# LUT\n# Number of inputs: ", num_of_ins))
    num_of_keys = int(math.pow(2, num_of_ins))
    print(("# Number of keybits", num_of_keys))
    for i in range(0, num_of_keys):
        print(("INPUT(keyinput%d)" % (i)))
    for i in range(0, num_of_ins):
        print(("INPUT(in%d)" % (i)))
    print("OUTPUT(out)")

    for i in range(0, num_of_ins):
        print(("in%dnot = not(in%d)" % (i, i)))
    for i in range(0, num_of_keys):
        str = "T%d = and(" % (i)
        for j in range(0, num_of_ins):
            str += ("in%d" % (j))
            if ((i >> j) & 0x01 == 0x01):
                str += "not"
            if j == num_of_ins - 1:
                str += ",keyinput%d)" % (i)
            else:
                str += ","
        print(str)
    str = "out = or("
    for i in range(0, num_of_keys):
        str += "T%d" % (i)
        str += "," if (i != num_of_keys - 1) else ")"
    print(str)
    return;


def genchainedlut(num_of_ins, depth, f, forg):
    orig_stdout = sys.stdout
    sys.stdout = f
    print(("# LUT\n# Number of inputs: ", num_of_ins * depth - depth))
    num_of_keys = int(math.pow(2, num_of_ins))
    print(("# Number of keybits", num_of_keys))
    for d in range(0, depth):
        for i in range(0, num_of_keys):
            print(("INPUT(keyinput%dx%d)" % (i, d)))
        for i in range(0, num_of_ins):
            if (i == 0) and (d > 0):
                print("")
            else:
                print(("INPUT(in%dx%d)" % (i, d)))
        if d == depth - 1:
            print(("OUTPUT(out%d)" % (depth - 1)))

    for d in range(0, depth):
        for i in range(0, num_of_ins):
            if (i == 0) and (d > 0):
                print(("out%dnot = not(out%d)" % (d - 1, d - 1)))
            else:
                print(("in%dx%dnot = not(in%dx%d)" % (i, d, i, d)))
        for i in range(0, num_of_keys):
            str = "T%dx%d = and(" % (i, d)
            for j in range(0, num_of_ins):
                if (d > 0) and (j == 0):
                    str += "out%d" % (d - 1)
                else:
                    str += ("in%dx%d" % (j, d))
                if ((i >> j) & 0x01 == 0x01):
                    str += "not"
                if j == num_of_ins - 1:
                    str += ",keyinput%dx%d)" % (i, d)
                else:
                    str += ","
            print(str)
        str = "out%d = OR(" % (d)
        for i in range(0, num_of_keys):
            str += "T%dx%d" % (i, d)
            str += "," if (i != num_of_keys - 1) else ")"
        print(str)
    f.close()
    sys.stdout = forg
    print(("# LUT\n#Number of inputs: ", num_of_ins * depth - depth))
    for d in range(0, depth):
        for i in range(0, num_of_ins):
            if (i == 0) and (d > 0):
                print("")
            else:
                print(("INPUT(in%dx%d)" % (i, d)))
        if d == depth - 1:
            print(("OUTPUT(out%d)" % (depth - 1)))

    str = "out%d = OR(" % (depth - 1)
    for d in range(0, depth):
        for i in range(0, num_of_ins):
            if (i == 0) and (d > 0):
                continue
            else:
                str += "in%dx%d" % (i, d)
            if d == depth - 1 and i == num_of_ins - 1:
                str += ")"
            else:
                str += ","
    print(str)
    forg.close()

    return;


def fullyentangledlut(num_of_ins, f, forg):
    orig_stdout = sys.stdout
    sys.stdout = f
    print(("# LUT\n#Number of inputs: ", num_of_ins * num_of_ins))
    num_of_keys = int(math.pow(2, num_of_ins))
    print(("# Number of keybits", num_of_keys))
    for d in range(0, num_of_ins):
        for i in range(0, num_of_keys):
            print(("INPUT(keyinput%dx%d)" % (i, d)))
        for i in range(0, num_of_ins):
            print(("INPUT(in%dx%d)" % (i, d)))

    for i in range(0, num_of_keys):
        print(("INPUT(keyinput%d)" % (i)))

    print("OUTPUT(out)")

    for d in range(0, num_of_ins):
        for i in range(0, num_of_ins):
            print(("in%dx%dnot = not(in%dx%d)" % (i, d, i, d)))
        for i in range(0, num_of_keys):
            str = "T%dx%d = and(" % (i, d)
            for j in range(0, num_of_ins):
                str += ("in%dx%d" % (j, d))
                if ((i >> j) & 0x01 == 0x01):
                    str += "not"
                if j == num_of_ins - 1:
                    str += ",keyinput%dx%d)" % (i, d)
                else:
                    str += ","
            print(str)
        str = "out%d = OR(" % (d)
        for i in range(0, num_of_keys):
            str += "T%dx%d" % (i, d)
            str += "," if (i != num_of_keys - 1) else ")"
        print(str)

    for i in range(0, num_of_ins):
        print(("out%dnot = not(out%d)" % (i, i)))
    for i in range(0, num_of_keys):
        str = "T%d = and(" % (i)
        for j in range(0, num_of_ins):
            str += "out%d" % (j)
            if ((i >> j) & 0x01 == 0x01):
                str += "not"
            if j == num_of_ins - 1:
                str += ",keyinput%d)" % (i)
            else:
                str += ","
        print(str)
    str = "out = OR("
    for i in range(0, num_of_keys):
        str += "T%d" % (i)
        str += "," if (i != num_of_keys - 1) else ")"
    print(str)

    f.close()
    sys.stdout = forg
    print(("# LUT\n#Number of inputs: ", num_of_ins * num_of_ins))
    for d in range(0, num_of_ins):
        for i in range(0, num_of_ins):
            print(("INPUT(in%dx%d)" % (i, d)))
    print("OUTPUT(out)")

    str = "out = OR("
    for d in range(0, num_of_ins):
        for i in range(0, num_of_ins):
            str += "in%dx%d" % (i, d)
            if d == num_of_ins - 1 and i == num_of_ins - 1:
                str += ")"
            else:
                str += ","
    print(str)
    forg.close()

    sys.stdout = orig_stdout

    return


def genorig (num_of_ins, filename):

    f = open(filename, 'w')
    orig_stdout = sys.stdout
    sys.stdout = f
    print(("# Orig \n#Number of inputs: ", num_of_ins))
    for i in range(0, num_of_ins):
        print(("INPUT(in%d)" % (i)))
    print("OUTPUT(out)")
    str = "out = OR("
    for i in range(0, num_of_ins):
        str += "in%d" % (i)
        str += "," if (i != num_of_ins - 1) else ")"
    print(str)
    sys.stdout = orig_stdout

    return;


def genandtree (num_of_ins, f):

    orig_stdout = sys.stdout
    sys.stdout = f
    print(("# AND-tree\n# Number of inputs: ", num_of_ins))
    num_of_keys = num_of_ins
    print(("# Number of keybits", num_of_keys))
    for i in range(0, num_of_keys):
        print(("INPUT(keyinput%d)" % (i)))
    for i in range(0, num_of_ins):
        print(("INPUT(in%d)" % (i)))
    print("OUTPUT(out)")

    for i in range(0, num_of_ins):
        print(("in%dxor = xor(in%d,keyinput%d)" % (i, i, i)))
    str = "out = and("
    for i in range(0, num_of_keys):
        str += "in%dxor" % (i)
        str += "," if (i != num_of_keys - 1) else ")"
    print(str)

    return


def genandtreeorig(num_of_ins, f):

    orig_stdout = sys.stdout
    sys.stdout = f
    get_bin = lambda x, n: format(x, 'b').zfill(n)
    print(("# AND-tree\n# Number of inputs: ", num_of_ins))
    num_of_keys = num_of_ins
    print(("# Number of keybits", num_of_keys))
    random.seed()
    key = random.getrandbits(num_of_ins)
    print(("# key: ", get_bin(key, num_of_keys)))
    for i in range(0, num_of_ins):
        print(("INPUT(in%d)" % (i)))
    print("OUTPUT(out)")

    for i in range(0, num_of_ins):
        if (key >> i) & 0x01 == 0x01:
            print(("in%dxor = not(in%d)" % (i, i)))
        else:
            print(("in%dxor = buf(in%d)" % (i, i)))
    str = "out = and("
    for i in range(0, num_of_keys):
        str += "in%dxor" % (i)
        str += "," if (i != num_of_keys - 1) else ")"
    print(str)
    return;


def genresblock(num_of_ins, fl0, fl1, correct_comp=0):

    f0 = open(fl0, 'w')
    f1 = open(fl1, 'w')
    random_source = True
    orig_stdout = sys.stdout
    sys.stdout = f0
    print(("# SATblock\n# Number of inputs: ", num_of_ins))
    num_of_keys = num_of_ins
    num_of_comps = int(math.pow(2, num_of_keys))
    comp_len = int(math.pow(2, num_of_ins))
    print(("# Number of keybits: ", num_of_keys))
    print(("# key: ", "0" * num_of_keys))
    for i in range(0, num_of_keys):
        print(("INPUT(keyinput%d)" % (i)))
    for i in range(0, num_of_ins):
        print(("INPUT(in%d)" % (i)))
    print("OUTPUT(out)")

    get_bin = lambda x, n: format(x, 'b').zfill(n)

    if random_source:
        random.seed()
        correct_comp = random.getrandbits(comp_len)

    compflip = comp_len - 1

    comp = 0
    # print "correct completion ", get_bin(correct_comp,comp_len)

    excess = num_of_ins - 8
    str = ''
    if excess <= 0:
        comp_flips = list()
        str += "out = LUT 0x"
        str += format(correct_comp, 'x')
        for i in range(0, num_of_comps - 1):
            compflip = randomflip(comp_flips, comp_len)
            comp = correct_comp ^ (0x01 << compflip)
            str += format(comp, 'x')
            # print "fliped completion ", get_bin(comp,comp_len)
        print(("# ", comp_flips))
        str = printins(num_of_ins, str)
    else:
        lutnums = int(math.pow(2, excess))
        lutnum_of_comps = num_of_comps / lutnums
        comp_flips = list()
        for i in range(0, lutnums):
            str += "out%d = LUT 0x" % (i)
            for i in range(0, lutnum_of_comps):
                compflip = randomflip(comp_flips, comp_len)
                comp = correct_comp ^ (0x01 << compflip)
                str += format(comp, 'x')
                # print "fliped completion ", get_bin(comp,comp_len)
            str = printins(num_of_ins - excess, str)
        str = printmux(num_of_ins, str)
        # print comp_flips
    print(str)

    sys.stdout = f1
    print(("# SATblock\n#Number of inputs: ", num_of_ins))
    comp_len = int(math.pow(2, num_of_ins))
    for i in range(0, num_of_ins):
        print(("INPUT(in%d)" % (i)))
    print("OUTPUT(out)")

    str = "out = LUT 0x"

    str += format(correct_comp, 'x')

    str += " ("
    for i in range(0, num_of_ins):
        str += "in%d" % (i)
        str += "," if i != num_of_ins - 1 else ")"

    print(str)

    sys.stdout = orig_stdout

    return


def genrandnet(num_of_ins, num_of_outs, out_file, ttbias=0, lib_file=myabc.abclib):
    """ generate circuit from random truth-table
    latency of the coin can be specified in the range [0,1]"""
    assert -0.5 <= ttbias <= 0.5
    out_file_base = os.path.split(out_file)[1]
    tmp_file = "./gennet_tmp_file1"

    genrandnet_lut(num_of_ins, num_of_outs, tmp_file, ttbias)
    myabc.resynthesize(tmp_file, out_file, lib_file)
    cmd = 'rm {0}'.format(tmp_file)
    print(cmd)
    os.system(cmd)


def getbiasedstring(lentgh, bias):
    """ get a random string with bias"""
    assert-0.5 <= bias <= 0.5
    randbits = random.choices(['0', '1'], weights=(0.5 - bias, 0.5 + bias), k=lentgh)
    print(randbits)
    rstr = ''
    for rb in randbits:
        rstr += str(rb)
    hexstr = "{0:0>3x}".format(int(rstr, 2))
    print(rstr)
    print(hexstr)
    return hexstr


def genrandnet_lut (num_of_ins, num_of_outs, out_file, ttbias=0):

    assert -0.5 <= ttbias <= 0.5

    print((os.getcwd()))
    fout = open(out_file, 'w')

    random_source = True

    str = "# random cone\n# Number of inputs: {0}\n".format(num_of_ins)
    table_len = int(math.pow(2, num_of_ins))
    for i in range(0, num_of_ins):
        str += "INPUT(in%d)\n" % (i)

    if num_of_outs == 1:
        str += "OUTPUT(out)\n"
    else:
        for i in range(0, num_of_outs):
            str += "OUTPUT(out%d)\n" % (i)

    excess = num_of_ins - 8
    for o in range(0, num_of_outs):
        if excess <= 0:
            if random_source:
                random.seed()
            str += "out%d = LUT 0x" % (o) \
                if (num_of_outs > 1) else "out = LUT 0x"
            str += getbiasedstring(table_len, ttbias)
            str += " ("
            str = printinsonly(num_of_ins, str)

        else:
            lutnums = int(math.pow(2, excess))
            lut_table_len = table_len / lutnums
            for i in range(0, lutnums):
                lut_table_vector = random.getrandbits(lut_table_len)
                str += "out%d_%d = LUT 0x" % (i, o) \
                    if (num_of_outs > 1) else "out%d = LUT 0x" % (i)
                str += getbiasedstring(table_len, ttbias)
                str += " ("
                # print "fliped completion ", get_bin(comp,comp_len)
                str = printinsonly(8, str)
            str = printmuxonly(num_of_ins, str)
            # print comp_flips
    fout.write(str)

    return


def printmuxonly(num_of_ins, str):
    extra_ins = num_of_ins - 8;
    num_of_ands = pow(2, extra_ins)
    for i in range(8, num_of_ins):
        str += "in%dnot = NOT(in%d)\n" % (i, i)
    for i in range(0, num_of_ands):
        str += "T%d = AND(" % (i)
        for j in range(0, extra_ins):
            str += "in%d" % (j + 8)
            if (i >> j) & 0x01 == 0x01:
                str += "not"
            if j == extra_ins - 1:
                str += ",out%d)\n" % (i)
            else:
                str += ","
    str += "out = OR("
    for i in range(0, num_of_ands):
        str += "T%d" % (i)
        str += "," if (i != num_of_ands - 1) else ")\n"
    return str


def printinsonly(num_of_ins, strg):
    for i in range(0, num_of_ins):
        strg += "in%d" % i
        strg += "," if i != num_of_ins - 1 else ")\n"
    return strg


def randomflip(comp_flips, comp_len):
    while True:
        compflip = random.randint(0, comp_len - 1)
        if compflip not in comp_flips:
            comp_flips.append(compflip)
            return compflip


def printins(numins, str):
    str += " ("
    for i in range(0, numins):
        str += "in%d," % (i)
    for i in range(0, numins):
        str += "keyinput%d" % (i)
        str += "," if i != numins - 1 else ")\n"
    return str


def printmux(num_of_ins, str):
    num_of_keys = int(math.pow(2, (num_of_ins - 4) * 2))
    for i in range(4, num_of_ins):
        str += "in%dnot = NOT(in%d)\n" % (i, i)
        str += "keyinput%dnot = NOT(keyinput%d)\n" % (i, i)
    for i in range(0, num_of_keys):
        str += "T%d = AND(" % (i)
        for j in range(0, (num_of_ins - 4) * 2):
            str += ("in%d" % ((j + 1 / 2) + 4)) if (j < (num_of_ins - 4)) else (
                "keyinput%d" % ((j + 1) / 2 + 4 - (num_of_ins - 4)))
            if (i >> j) & 0x01 == 0x01:
                str += "not"
            if j == ((num_of_ins - 4) * 2) - 1:
                str += ",out%d)\n" % (i)
            else:
                str += ","
    str += "out = OR("
    for i in range(0, num_of_keys):
        str += "T%d" % (i)
        str += "," if (i != num_of_keys - 1) else ")\n"
    return str
