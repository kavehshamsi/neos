# ** Netlist encryption and obfuscation suite ** #

* This project is an object-oriented framework written in modern C++ for gate-level netlist obfuscation/deobfuscation. The tool contains a set of obfuscation and deobfuscation utilities and can be used through the command line. For now we have an available binary tested on ubuntu16 and centos7. Please send problems (error-cases) to me (Kaveh Shamsi).

* Version 2.0

* Contacts: 
    - Kaveh Shamsi : kaveh.shamsi@utdallas.edu

# Capabilities #

* neos has parsers for netlist files in Bench and Verilog formats supporting combinational circuits that use AND/NAND/OR/NOR/BUF/NOT/XOR/XNOR gates. A set of benchmark circuits are included in the /bench directory.

* Combinational and Sequential SAT based deobfuscation using the Glucouse SAT solver with various key-condition-crunching techniques. Sequential deobfuscation is based on model-checking using integrated bounded-model-checking (BMC) routines. 

* various locking schemes including: interconnect locking (cross-bar insertion, mux-flooding), LUT insertion, XOR/XNOR-AND/OR random insertion and insertion based on flip-rates or symbolic probability values, plus SAT-resilient schemes such as antisat etc.

# Source Code #

The source code is released under the LICENSE_SRC license. The `./src` directory includes neos's source code, and the `./lib` directory includes external librarires, such as sat solvers and `cudd`. The `./pyneos` directory includes a python library for doing circuit operations and a z3 based deobufscation tool along with some graph utilities and benchmark generation (SPNs and random truth-tables synthesized by ABC). The `./misc` directory includes some miscellaneous code, namely for sequential and combinational oracle instances. 

You will need to build `cudd` in order to compile from source. You can execute the following before running `make` in the `neos` directory to make sure cudd compiles correctly:

```
cd ./lib/cudd/
autoreconf
./configure --enable-obj --enable-shared --enable-static
```


# Usage #

there are two main ways to use neos. First is by simply passing flags and arguments, and second is using the interactive mode. The interactive mode is a yet experimental feature that dynamically links to libreadline and the non-interactive mode uses boost_program_options. 

For Ubuntu18 users, Ubuntu18 by default has `libreadline5` and `libreadline7` without `libreadline6`. Until this issue is resolved, you can manually copy `libreadline.so.7` or `libreadline.so.5` in your system to `libreadline.so.6` to get around this issue. 
These libraries should be in `/usr/lib/x86_64-linux-gnu/` on your linux system.

Note that the `abc` binary included in `./bin` differs slightly from official ABC releases. Namely, the `ioReadBench` interface in ABC has been modified to read lower-case gates in bench format, and to recognize MUX(s, a, b) gates as a zero on the s-input meaning the first input is chosen.

We start with the non-interactive mode:

```
usage: neos [options] <positional_args>
Allowed options:
  -h [ --help ]               see help
  -i [ --int ]                interactive mode
  -c [ --cmd ]                script execution mode (open interactive shell and
                              run commands)
  -e [ --enc ] [=arg(=undef)] encryption/obfuscation mode
  -d [ --dec ] [=arg(=undef)] decryption/deobfuscation mode: <method> : 
                              decryption method
  -q [ --eq ]                 equivalence checker (key verification)
  -m [ --misc ]               miscellaneous routines
  --use_verilog               use verilog as input and output
  -v [ --verbose ] arg        verbosity level
  --cir_stats                 print circuit stats
  --to arg                    set timeout in minutes
  --mo arg                    set memory limit in MB
  --pos_args arg              positional arguments

```

We can mix the `-h` flag with other flags to get more help. For the encryption mode we have:

```
usage: neos -e <lock_method> <input_bench> <output_bench> [args]
   ant            : Anti-SAT locking
   ant_dtl        : Anti-SAT DTL locking
   aor            : AND/OR insertion
   crlk           : Cross-bar locking (crosslock)
   cstwr          : cost-function based interconnect obfuscation
   cyc            : Cyclic locking (N loops of length L)
   cyc2           : Cyclic locking (N loops of length L) + cyclify original
   deg            : degree-driven insertion
   int            : interconnect obfuscation suite
   lut            : Random k-LUT insertion
   mmux           : insert N muxes of size M
   mmux_cg        : graph-density MUX insertion
   par            : Cross-bar locking (crosslock)
   sfll_point     : Corrected output locking
   shfk           : input-output permutation insertion
   univ           : Universal circuit locking
   xor            : Random XOR/XNOR insertion
   xorflip        : fliprate-driven XOR/XNOR insertion
   xorprob        : XOR/XNOR insertion in points with first-level symbolic probability closest to 0.5
```

We can now obfuscate some circuits. Create a new directory `mkdir ./bench/testing`. We can then do a simple random XOR/XNOR insertion with 20 key-bits with the following :

```
#!bash
./bin/neos -e xor ./bench/comb/c432.bench ./bench/testing/c432_xor.bench 20
#width xor: 20
encryption complete for ./bench/comb/c432.bench
number of inserted key-bits: 20
overhead: 12%
```

Hopefully the terminal colloring works on your machine as well. 
We can now deobfsucate this circuit with :

```
#!bash
running combinatonal attack...
iteration:    1; variables: 445 clauses: 1224 assignments: 1  propagations: 662
iteration:    2; variables: 823 clauses: 1941 assignments: 141  propagations: 2903
iteration:    3; variables: 1201 clauses: 2559 assignments: 319  propagations: 5096
iteration:    4; variables: 1579 clauses: 3179 assignments: 495  propagations: 9710
iteration:    5; variables: 1957 clauses: 3497 assignments: 755  propagations: 12706
iteration:    6; variables: 2335 clauses: 3811 assignments: 1013  propagations: 15531
iteration:    7; variables: 2713 clauses: 4149 assignments: 1261  propagations: 19027
iteration:    8; variables: 3091 clauses: 4445 assignments: 1525  propagations: 26548

Finished! 
iteration=8
key=00001001100001001011
checking key 00001001100001001011

equivalent

time: 0.026041

```

neos now supports binary/executable oracle instead of an original circuit. For the combinational case, the oracle must be an executable that will take a line of 0/1 ending in a newline-char, with the order of inputs as they are listed in the locked circuit's bench/verilog file. It will then output a string of 0/1s that follow the order of the outputs as they are listed in the locked circuit's bench/verilog file.

The `misc` directory includes source files for simulating such binary oracles using combinational and sequential benchmarks. After compiling these using the `compile_oracles` script, you can run a sat attack against the binary oracle for the c432 benchmark by running the following:
```
#!bash
./bin/neos -d ex --bin_oracle=./misc/comb.out ./bench/testing/c432_xor.bench 20
```

The `s27` sequential benchmark is a great way to test basic sequential attacks:

```
#!bash
./bin/neos -e xor ./bench/seq/s27.bench ./bench/testing/s27_enc.bench 10
#width xor: 20
encryption complete for ./bench/seq/s27.bench
number of inserted key-bits: 10
overhead: 15%
```

We can deobfuscate this with simple unrolling:
```
#!bash
enc-ckt has DFFs. running sequnetial attack...
dec method is: int
vebrosity: 0
key-condition sweeping mode: 0
BMC sweeping mode: 0
AIG mode : 0
Relative to kckt SAT-sweeping: 0
with propagation limit: -1
with mitter sweep: 0
BDD analysis option: 0
KC BDD size limit: 0
BMC bound limit: 600
frame: 0  stats -> variables: 80 clauses: 173 assignments: 0  propagations: 0
adding termination links
DIS iter: 0
  DIP clk_0 -> 1110  1
  dis: 1  stats -> variables: 120 clauses: 273 assignments: 7  propagations: 249
DIS iter: 1
  DIP clk_0 -> 0110  1
  dis: 2  stats -> variables: 158 clauses: 367 assignments: 13  propagations: 481
DIS iter: 2
  DIP clk_0 -> 1010  1
  dis: 3  stats -> variables: 196 clauses: 465 assignments: 17  propagations: 861
DIS iter: 3
  DIP clk_0 -> 1011  0
  dis: 4  stats -> variables: 234 clauses: 555 assignments: 23  propagations: 1253
DIS iter: 4
  DIP clk_0 -> 1111  1
  dis: 5  stats -> variables: 272 clauses: 641 assignments: 31  propagations: 1683
frame: 1  stats -> variables: 325 clauses: 804 assignments: 40  propagations: 5351
DIS iter: 5
  DIP clk_0 -> 0011  0
  DIP clk_1 -> 1001  0
  dis: 6  stats -> variables: 401 clauses: 996 assignments: 53  propagations: 5673
DIS iter: 6
  DIP clk_0 -> 1011  0
  DIP clk_1 -> 1001  0
  dis: 7  stats -> variables: 477 clauses: 1188 assignments: 66  propagations: 6411
DIS iter: 7
  DIP clk_0 -> 1011  0
  DIP clk_1 -> 0111  0
  dis: 8  stats -> variables: 553 clauses: 1368 assignments: 83  propagations: 7481
DIS iter: 8
  DIP clk_0 -> 0111  1
  DIP clk_1 -> 0010  1
  dis: 9  stats -> variables: 629 clauses: 1556 assignments: 97  propagations: 8836
DIS iter: 9
  DIP clk_0 -> 1010  1
  DIP clk_1 -> 1111  1
  dis: 10  stats -> variables: 705 clauses: 1744 assignments: 111  propagations: 10197
no trace up to bound 2
increasing bmc bound to 3
termination checking up to round: 1. current BMC checked bound: 1
reached combinational termination!

sat_time:  0.000000
term_time: 0.000239
gen_time:  0.000000
bmc_time:  0.003398
simp_time: 0.000000
total_time: 0.005929

dis_count: 10
dis_maxlen: 2

key=1000110000
checking key

executable path: /home/kaveh/Development/eclipse/neos_bin_release/bin/neos
neos dir : /home/kaveh/Development/eclipse/neos_bin_release
abc path : /home/kaveh/Development/eclipse/neos_bin_release/bin/abc
abc libs: /home/kaveh/Development/eclipse/neos_bin_release/cells/simpler.lib
abc libs: /home/kaveh/Development/eclipse/neos_bin_release/cells/xoronly.lib
UC Berkeley, ABC 1.01 (compiled Apr 22 2020 16:07:37)
abc 01> dsec ./tmp45739/cir1.bench  ./tmp45739/cir2.bench
Warning: 6 registers in this network have don't-care init values.
The don't-care are assumed to be 0. The result may not verify.
Use command "print_latch" to see the init values of registers.
Use command "zero" to convert or "init" to change the values.
Networks are equivalent.  Time =     0.01 sec

equivalent

done

```
The sequential attack performs queries at different depths and does an equivalence checking using ABC at the end. 


The `s400` benchmark circuit is a great test case for deep sequential deobfuscation with key-condition-crunching (kc2).


kc2 help information:

```
#!bash
./bin/neos -d int --use_aig --kc_sweep=2 -h ./bench/rnseq/s400.bench ./bench/testing/s400_xor.bench 100
enc-ckt has DFFs. running sequnetial attack...
dec method is: int

neos_error: usage : neos -d [mcdec method] <orig file> <enc file> [attack_specific_args]:

  mcdec method:
       nuxmv : use nuxmv external model-checker. Need nuxmv binary in ./bin/ directory
       int   : use internal BMC for model-checking.
       hill  : sequential hill climbing.
  Options:
       --use_aig : use AIGs instead of circuit when applicable (int)
       --kc_sweep_iterative: 			0: apply kc-sweep to entire kc_ckt only
			1: apply kc_sweep to new DIS every step
       --kc_sweep: key-condition simplification every --kc_sweep_period number of DISs
           0: no key-condition sweeping (default)
           1: use key-circuit but not simplify
           2: noes-internal simplification (fraig for AIGs and netlists applied to sum of key-conditions)
           3: use abc for simplification (abc-based strash/fraig/map. faster than neos alternatives)
       --mitt_sweep: mitter sweep:
           0: no mitter-sweep (default)
           1: sweep mitter
       --to: timeout in minutes
       --mo: memory limit in MB (default=1000)

```

You can compare the output of the following commands to see the effect of kc2 on SAT-solver clause counts:
```
#!bash
./bin/neos -e xor ./bench/testing/s400.bench ./bench/testing/s400_enc.bench 25

./bin/neos -d int ./bench/testing/s400.bench ./bench/testing/s400_enc.bench 100 | grep 'var'
./bin/neos -d int --kc_sweep=2 ./bench/testing/s400.bench ./bench/testing/s400_enc.bench 100 | grep 'var'
./bin/neos -d int --kc_sweep=3 --kc_sweep_period=5 ./bench/testing/s400.bench ./bench/testing/s400_enc.bench 100 | grep 'var'
./bin/neos -d int --kc_sweep=3 --mitt_sweep=2 ./bench/testing/s400.bench ./bench/testing/s400_enc.bench 100 | grep 'var'
```
 

neos also supports an interactive mode using `libreadline`. If your system supports this you can try 
```
#!bash
./bin/neos -i 
```
 for the interactive mode. Tab-completion on file-names and commands can help you around.
 We can do the above sequential obfuscation using the command mode with the `-c` flag which simply takes a string in double-quotes and passes it to the interactive-mode interpreter. 
 
```
#!bash
./bin/neos -c "read_bench ./bench/seq/s27.bench; draw_graph; enc xor 10; draw_graph; quit;"
```

make sure to have `xdot` installed on your linux system to view the dot files. In ubuntu you can run:

`sudo apt-get install xdot`




