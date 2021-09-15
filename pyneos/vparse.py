from lark import Lark
from lark import Transformer
import os
import sys
import time
from circuit import *

# verilog grammer
vlog_parser = Lark(r"""
    netlist : module+

    module : module_decl statement_list ENDMODULE

    statement_list : statement+

    ?statement : statement SEMCOLON+ | always_decl | wire_decl | output_decl | input_decl | gate_decl | instance_decl | assign_decl

    module_decl : MODULE IDENTIFIER LPAREN ident_list RPAREN ";"+
    wire_decl : WIRE range? ident_list
    output_decl : OUTPUT range? ident_list
    input_decl : INPUT range? ident_list
    gate_decl : PRIM_GATE IDENTIFIER LPAREN netvar_list RPAREN
    instance_decl : IDENTIFIER IDENTIFIER LPAREN (conn_pair_list | ident_list) RPAREN
    assign_decl : ASSIGN netvar "=" netvar
    always_decl : ALWAYS LPAREN sense_list RPAREN BEGIN proc_list END

    proc_list : proc+
    proc : if_statement | block_assign | nonblock_assign
    
    block_assign : netvar EQ netvar
    nonblock_assign : netvar NBEQ netvar
    if_statement : IF BEGIN proc END | IF BEGIN proc END ELSE BEGIN proc END 
    
    sense_list : sense_var ("," sense_var)
    sense_var : ATSIGN netvar
    
    conn_pair_list : conn_pair ("," conn_pair)*
    conn_pair :  DOT IDENTIFIER LPAREN netvar RPAREN

    ident_list : IDENTIFIER ("," IDENTIFIER)*

    range : LBR INTEGER COLON INTEGER RBR
    constant : HEXCONST | BINCONST | DECCONST
    HEXCONST.3 : INTEGER "'h" /[0-9a-fx]/+
    BINCONST.3 : INTEGER "'b" /[01x]/+
    DECCONST.3 : INTEGER "'d" /[0-9]/+

    IDENTIFIER.1 : /[a-zA-Z\\$_][a-zA-Z0-9\\.$_]*/

    netvar_list : netvar ("," netvar)*

    ?netvar : constant
       | IDENTIFIER
       | IDENTIFIER LBR INTEGER RBR -> indexed_var
       | IDENTIFIER range -> ranged_var
       | LBC netvar ("," netvar)* RBC -> concat_var


    COMMENT : "//" /(.)*/ "\r"? "\n" | "/*" /(.)*/ "*/" | "(" /[*]/ /(.)*/ /[*]/ ")"

    DOT : "."
    LPAREN : "("
    RPAREN : ")"
    LBR : "["
    RBR : "]"
    LBC : "{"
    RBC : "}"
    COLON : ":"
    SEMCOLON : ";"
    EQ : "="
    NBEQ : "<="
    ATSIGN : "@"
    DEQ : "=="
    NEQ : "!="
    MODULE : "module"
    WIRE : "wire"
    INPUT : "input"
    OUTPUT : "output"
    ASSIGN : "assign"
    ALWAYS : "always"
    BEGIN : "begin"
    END : "end"
    IF : "if"
    ELSE : "else"
    ENDMODULE : "endmodule"
    INTEGER.2 : /[0-9]+/

    PRIM_GATE.3 : "nand" | "nor" | "and" | "or" | "not" | "buf" | "xor" | "xnor" | "mux" | "dff"

    %import common.WS
    %ignore WS
    %ignore COMMENT

    """, start='netlist', parser='lalr', lexer='standard')


# some helper methods

def range_list(first, second):
    if first > second:
        return list(range(first, second - 1, -1))
    else:
        return list(range(first, second + 1))


# verilog AST to circuit transformer
class vlogTransformer(Transformer):

    def __init__(self):
        super(vlogTransformer, self).__init__()
        # list of circuit modules
        self.modules = list()

        # map variable names to their range
        self.range_map = dict()

        # variables to connect during port resolution
        self.submod_var_pairs = list()

    def module(self, items):
        cir = Circuit()
        cir.circuit_name = items[0].children[1]
        # clear the range map
        self.range_map.clear()

        # first add all wires and ports
        for statement in items[1].children:
            sttype = statement.children[0].data
            if sttype == 'wire_decl' or sttype == 'input_decl' or sttype == 'output_decl':
                self.add_wires(statement, cir)

        # add gates and instances after that
        for statement in items[1].children:
            sttype = statement.children[0].data
            if sttype == 'instance_decl':
                self.add_instance(statement, cir)
            elif sttype == 'gate_decl':
                self.add_gate(statement, cir)
            elif sttype == 'assign_decl':
                self.add_assignment(statement, cir)

        # add to list of modules
        self.modules.append(cir)

    def add_wires(self, st, cir):
        nd = st.children[0]
        if nd.children[1].data == 'ident_list':
            wirenames = nd.children[1].children
        elif nd.children[1].data == 'range':
            varname = nd.children[2].children[0]
            first = int(nd.children[1].children[1])
            second = int(nd.children[1].children[3])
            self.range_map[nd.children[2].children[0]] = [first, second]

            wirenames = list()
            for index in range_list(first, second):
                wirenames.append('{0}.{1}'.format(varname, index))

        for wirename in wirenames:
            if nd.data == 'input_decl':
                if wirename[0:8] == 'keyinput':
                    cir.add_wire(ntype.KEY, wirename)
                else:                    
                    cir.add_wire(ntype.IN, wirename)
            elif nd.data == 'output_decl':
                cir.add_wire(ntype.OUT, wirename)
            elif nd.data == 'wire_decl':
                cir.add_wire(ntype.INTER, wirename)

    def add_instance(self, st, cir):
        inst_module = st.children[0].children[0]
        inst_name = st.children[0].children[1]
        list_type = st.children[0].children[3].data
        connections = st.children[0].children[3].children

        name_pairs = list()
        i = 0
        for con in connections:
            if list_type == 'conn_pair_list':
                port_name = str(con.children[1])
                conn_var = con.children[3]
            else:
                port_name = '_$tbd$_%i' % i
                i += 1
                conn_var = con

            rhs_names = self.expand_var(cir, conn_var)
            index = 0
            if len(rhs_names) == 1:
                name_pairs.append([port_name, rhs_names[0]])
            else:
                for rhs_name in rhs_names:
                    name_pairs.append(['{0}.{1}'.format(port_name, index), rhs_name])
                    index += 1

        cir.add_instance(inst_name, inst_module, name_pairs)

    # expands variables recursively and returns ordered list of bits
    def expand_var(self, cir, var):
        expanded_names = list()
        if hasattr(var, 'data'):
            if var.data == 'indexed_var':
                expanded_names.append( '{0}.{1}'.format(var.children[0], var.children[2]) )

            elif var.data == 'constant':
                dat_str = var.children[0]
                type = '\'h' in dat_str
                bit_len, hex_str = dat_str.split('\'h') \
                    if type else dat_str.split('\'h')
                hex_str = hex_str.replace('x', '0')
                bit_len = int(bit_len)
                bin_str = bin(int(hex_str, 16))[2:].zfill(bit_len) \
                    if type else bin(int(hex_str, 10))[2:].zfill(bit_len)
                for i in range(0, bit_len):
                    if bin_str[i] == 1:
                        rhs_str = 'vdd'
                    else:
                        rhs_str = 'gnd'
                    expanded_names.append(rhs_str)

            elif var.data == 'ranged_var':
                var_name = var.children[0]
                first = int(var.children[1].children[1])
                second = int(var.children[1].children[3])
                for index in range_list(first, second):
                    expanded_names.append('{0}.{1}'.format(var_name, index))

            elif var.data == 'concat_var':
                for cvar in var.children[1:-1]:
                    expanded_names.extend(self.expand_var(cir, cvar))

        else: # single var
            var_name = str(var)
            # for mulit-bit nets
            if var_name in self.range_map:
                first, second = self.range_map[var_name]
                for index in range_list(first, second):
                    expanded_names.append('{0}.{1}'.format(var_name, index))
            # is not a multi-bit net
            else:
                expanded_names.append(str(var))

        return expanded_names

    def add_gate(self, st, cir):
        gate_fun = st.children[0].children[0]
        gate_name = st.children[0].children[1]

        gate_ports = list()

        for gate_in in st.children[0].children[3].children:
            gate_ports.extend(self.expand_var(cir, gate_in))

        if gate_ports[0] in self.range_map:
            print('cannot drive multibit output with primitive gate')
            exit(1)

        cir.add_gate(gate_fun, gate_ports[0], gate_ports[1:])

        return

    def add_assignment(self, st, cir):
        lhs = st.children[0].children[1]
        rhs = st.children[0].children[2]

        rhs_names = self.expand_var(cir, rhs)
        lhs_names = self.expand_var(cir, lhs)

        # print lhs_names, rhs_names

        if len(lhs_names) != len(rhs_names):
            print('bit-length mismatch in assigment')
            print('rhs = {0}\nlhs = {1}'.format(rhs_names, lhs_names))
            exit(1)

        # connect nets with buffer
        for i in range(0, len(rhs_names)):
            cir.add_gate('BUF', lhs_names[i], [rhs_names[i]])

        return


def parse_2_circuit_list(infile):
    cirlist = []
    with open(infile, 'r') as fv:
        text = fv.read()
        T = vlog_parser.parse(text)
        trans = vlogTransformer()
        trans.transform(T)
        
        return trans.modules
    

if __name__ == '__main__':

    if len(sys.argv) != 2:
        print('usage: python3 vparse.py <infile>')
        exit(1)
        
    text = open(sys.argv[1]).read()
    # text = open('./bench/lverilog/rs232.v').read()

    start_time = time.time()
    T = vlog_parser.parse(text)

    print('%s'% (time.time() - start_time))

    trans = vlogTransformer()
    trans.transform(T)
    for module in trans.modules:
        print(module.circuit_name)
        print(module.stats())
    # print T.pretty()



