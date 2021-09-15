#!/usr/bin/python

import gennet
import sys
import myabc

if __name__ == "__main__":
    if len(sys.argv) < 4 or len(sys.argv) > 5 :
        print('usage : getrandnet <input_width> <output_width> <output_file> [xorlib/abclib]')
        exit()

    if len(sys.argv) == 4 or sys.argv[4] == 'xorlib':
        gennet.genrandnet(int(sys.argv[1]), int(sys.argv[2]), sys.argv[3], 0.8)
    elif sys.argv[4] == 'abclib':
        gennet.genrandnet(int(sys.argv[1]), int(sys.argv[2]), sys.argv[3], 0.8, myabc.abclib)
    else:
        print(('bad library', sys.argv[4]))
# getlazyrandstring(64, 40)

