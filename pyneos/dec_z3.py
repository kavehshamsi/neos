from circuit import *
import z3
import time


def add_gate_clause(solver, fun, fanoutvar, faninvars):
    if fun == gfun.AND:
        solver.add(fanoutvar == z3.And(faninvars))
    elif fun == gfun.NAND:
        solver.add(fanoutvar == z3.Not(z3.And(faninvars)))
    elif fun == gfun.OR:
        solver.add(fanoutvar == z3.Or(faninvars))
    elif fun == gfun.NOR:
        solver.add(fanoutvar == z3.Not(z3.Or(faninvars)))
    elif fun == gfun.XOR:
        solver.add(fanoutvar == z3.Xor(*faninvars))
    elif fun == gfun.XNOR:
        solver.add(fanoutvar == z3.Not(z3.Xor(*faninvars)))
    elif fun == gfun.BUF:
        solver.add(fanoutvar == faninvars[0])
    elif fun == gfun.NOT:
        solver.add(fanoutvar == z3.Not(faninvars[0]))
    else:
        raise Exception('cant add clause for unsupported gate type {0}'.format(fun))

    return


def new_bool_variable():
    nv = z3.Bool('v{0}'.format(new_bool_variable.vcount))
    new_bool_variable.vcount += 1
    return nv
new_bool_variable.vcount = 0

def new_int_variable():
    nv = z3.Int('iv{0}'.format(new_int_variable.vcount))
    new_int_variable.vcount += 1
    return nv
new_int_variable.vcount = 0


def add_circuit_to_solver(cir, solver, varmap):
    for wid in cir.wires():
        if wid not in varmap:
            varmap[wid] = new_bool_variable()

    for gid in cir.gates():
        gout = cir.fanout(gid)
        assert(gout is not None)
        faninvars = list()
        for finid in cir.fanins(gid):
            faninvars.append(varmap[finid])
        fanoutvar = varmap[gout]
        add_gate_clause(solver, cir.gatefun(gid), fanoutvar, faninvars)

    return

def bool_to_int(bv):
    return z3.If(bv, z3.IntVal(1), z3.IntVal(0))

def add_count_ones_to_solver(inidvec, solver, id2varmap):
    # K = new_int_variable()
    # inboolvec = [id2varmap[xid] for xid in inidvec]
    # accu = z3.PbEq(inboolvec, K)

    accu = z3.IntVal(0)
    for xid in inidvec:
        accu = accu + bool_to_int(id2varmap[xid])
    accu_int = new_int_variable()
    solver.add(accu_int == accu)
    return accu_int

def parse_key_arg(key_str):
    keypart = key_str.split('=')[1]
    corrkey = [bool(int(s)) for s in keypart]
    print('corrkey is {}'.format(corrkey))
    return corrkey


def simplify_cir_with_key(cir, key):
    retcir = copy.deepcopy(cir)
    constmap = dict()
    ind = 0
    for kid in cir.keys():
        constmap[kid] = key[ind]
        ind += 1
    retcir.simplify(constmap)
    return retcir

def get_key_error(cir, corrkey, candkey):
    smap1 = dict()
    smap2 = dict()

    for i, kid in enumerate(cir.keys()):
        smap1[kid] = corrkey[i]
        smap2[kid] = candkey[i]

    num_patts = 1000
    hd = 0.0
    for np in range(num_patts):
        for xid in cir.inputs():
            smap2[xid] = smap1[xid] = bool(random.getrandbits(1))
        cir.simulate(smap1)
        cir.simulate(smap2)

        for oid in cir.outputs():
            if smap1[oid] != smap2[oid]:
                hd += 1

    hd /= num_patts * cir.num_outs()
    return hd


def check_equivalence_z3(cir1, cir2):
    added2new = dict()
    for xid1 in cir1.allins():
        nm = cir1.name(xid1)
        xid2 = cir2.find_wire(nm)
        #print('linking {0} to {1}'.format(nm, cir2.name(xid2)))
        if xid2 is None:
            raise Exception('cannot find cir1 wire {0} in cir2'.format(nm))
        added2new[xid2] = xid1

    eqcir = copy.deepcopy(cir1)
    eqcir.add_circuit(cir2, added2new, '_$1')

    ind = 0
    eqouts = []
    for yido2 in cir2.outputs():
        ynm = cir2.name(yido2)
        yid1 = cir1.find_wire(ynm)
        if yid1 is None:
            raise Exception('cannot find cir1 wire {0} in cir2'.format(ynm))
        yid2 = added2new[yido2]
        #print('linking {0} to {1}'.format(ynm, cir1.name(yid1)))
        eqoutid = eqcir.join_outputs(gfun.XOR, 'eq_${0}'.format(ind), {yid1, yid2})
        eqouts.append(eqoutid)
        ind += 1

    eqcir.join_outputs(gfun.OR, 'eqOut')
    #eqcir.write_bench()

    S = z3.Solver()

    varmap = dict()
    add_circuit_to_solver(eqcir, S, varmap)
    outid = list(eqcir.outputs())[0]
    status = S.check(varmap[outid])
    print('eq status is', status)
    if status == z3.sat:
        print('not equal')
        return False
    elif status == z3.unsat:
        print('equivalent')
        return True
    else:
        raise Exception('problem with solver')


class dipObj:
    def __init__(self):
        self.inputs = []
        self.outputs = []
        return


class CirDecrypt:
    def __init__(self, sim_cir, enc_cir, corrkey = None):
        self.enc_cir = enc_cir

        if corrkey is not None:
            self.corrkey = corrkey
            assert(len(self.corrkey) == self.enc_cir.num_keys())

        if sim_cir is not None:
            self.sim_cir = sim_cir
            self.enc2sim = dict()
            self.enc2ind = dict()
            self.sim2ind = dict()
            self.link_io(self.sim_cir, self.enc_cir)

        return

    def link_io(self, sim_cir, enc_cir):
        ind = 0
        for xid in sim_cir.allports():
            nm = sim_cir.name(xid)
            #print('linking {0}'.format(nm))
            exid = enc_cir.find_wire(nm)
            if exid is None:
                raise Exception('cannot find sim_cir wire {0} in enc_cir'.format(nm))

            self.enc2sim[exid] = xid
            self.enc2ind[exid] = ind
            self.sim2ind[xid] = ind

            ind += 1

    def check_key(self, keyvec):
        cir1 = simplify_cir_with_key(self.enc_cir, keyvec)
        cir2 = self.sim_cir
        if cir2 is None:
            cir2 = simplify_cir_with_key(cir1, self.corrkey)

        eq = check_equivalence_z3(cir1, cir2)
        if not eq:
            print('Not equivalent')
            print('Key error is : {}'.format(get_key_error(self, self.enc_cir, self.corrkey, keyvec)))




class CirDecryptZ3(CirDecrypt):
    def __init__(self, sim_cir, enc_cir, iteration_limit=20):
        self.mittwid2var = dict()
        self.iteration = 0
        self.iteration_limit = iteration_limit
        self.mitter = None
        self.ioout_var= None
        super().__init__(sim_cir, enc_cir)
        return


    def build_mitter(self, enc_cir):
        self.mitter = copy.deepcopy(enc_cir)
        added2new = dict()
        for xid in enc_cir.inputs():
            added2new[xid] = xid

        self.mitter.add_circuit(enc_cir, added2new, '_$1')
        #self.mitter.write_bench()

        ind = 0
        mitt_outs = []
        for oid in enc_cir.outputs():
            onm1 = enc_cir.name(oid)
            onm2 = onm1 + '_$1'
            oid1 = self.mitter.find_wcheck(onm1)
            oid2 = self.mitter.find_wcheck(onm2)
            mitto = self.mitter.join_outputs(gfun.XOR, 'mitt_{0}'.format(ind), {oid1, oid2})
            mitt_outs.append(mitto)
            ind += 1

        self.mittout_wid = self.mitter.join_outputs(gfun.OR, 'mittOut', mitt_outs)
        #self.mitter.write_bench()
        return

    def build_iocir(self, enc_cir):
        self.iocir = copy.deepcopy(enc_cir)
        io_outs = []
        for oid in copy.copy(self.iocir.outputs()):
            self.iocir.set_wiretype(oid, ntype.INTER)
            onm = self.iocir.name(oid)
            noid = self.iocir.add_wire(ntype.OUT)
            ioin = self.iocir.add_wire(ntype.IN, onm + '_$io')
            io_outs.append(noid)
            self.iocir.add_gate_wids(gfun.XNOR, noid, {ioin, oid})

        self.ioout_wid = self.iocir.join_outputs(gfun.AND, 'ioOut', set(io_outs))
        # self.iocir.write_bench()
        return


    def init_solver(self):
        self.solver = z3.Solver()
        add_circuit_to_solver(self.mitter, self.solver, self.mittwid2var)
        self.mittout_var = self.mittwid2var[self.mittout_wid]
        self.solver_zero = z3.Not(True)
        self.solver_one = z3.Not(self.solver_zero)
        self.ioout_var = self.solver_one
        return

    def solve_for_di(self):
        status = self.solver.check([self.mittout_var, self.ioout_var])

        if status == z3.unsat:
            print('mitter unsat')
            return None
        else:
            dis = dipObj()
            print('dis: x=', end='')
            #print(self.solver.model())
            for xid in self.enc_cir.inputs():
                xid = self.mitter.find_wcheck(self.enc_cir.name(xid))
                var = self.mittwid2var[xid]
                b = z3.is_true(self.solver.model()[var])
                print(int(b), end='')
                dis.inputs.append(b)
            print()
            return dis

    def add_io_constraint(self, di):

        for kc in [0, 1]:
            varmap = dict()
            for kid in self.iocir.keys():
                knm = self.iocir.name(kid)
                knm = knm if kc == 0 else (knm + '_$1')
                #print(knm)
                mittwid = self.mitter.find_wcheck(knm)
                varmap[kid] = self.mittwid2var[mittwid]

            ind = 0
            for xeid in self.enc_cir.inputs():
                xid = self.iocir.find_wcheck(self.enc_cir.name(xeid))
                xval = self.solver_one if di.inputs[ind] else self.solver_zero
                varmap[xid] = xval
                ind += 1

            ind = 0
            for yid in self.enc_cir.outputs():
                onm = self.enc_cir.name(yid)
                ioinid = self.iocir.find_wcheck(onm + '_$io')
                yval = self.solver_one if di.outputs[ind] else self.solver_zero
                varmap[ioinid] = yval
                ind += 1

            add_circuit_to_solver(self.iocir, self.solver, varmap)
            new_ioout_var = varmap[self.ioout_wid]
            self.ioout_var = z3.And(new_ioout_var, self.ioout_var)

        return

    def query_di(self, di):
        simmap = dict()
        ind = 0
        for xid in self.enc_cir.inputs():
            sxid = self.enc2sim[xid]
            simmap[sxid] = di.inputs[ind]
            ind += 1

        self.sim_cir.simulate(simmap)

        di.outputs.clear()
        print('dis: y=', end='')
        for yid in self.enc_cir.outputs():
            syid = self.enc2sim[yid]
            di.outputs.append(simmap[syid])
            print(int(simmap[syid]), end='')
        print()
        return

    def extract_key(self):
        if not self.solver.check(self.ioout_var):
            print('constraints not solvable')
            exit(1)

        extkey = []
        for kid in self.enc_cir.keys():
            knm = self.enc_cir.name(kid)
            mittid = self.mitter.find_wcheck(knm)
            kv = self.mittwid2var[mittid]
            extkey.append(z3.is_true(self.solver.model()[kv]))

        return extkey

    def solve_exact(self):

        tm0 = time.time()

        self.build_mitter(self.enc_cir)
        self.build_iocir(self.enc_cir)
        self.init_solver()
        self.iteration = 0
        # print(self.solver)

        while True:
            di = self.solve_for_di()

            if di is None:
                print('no more dips')
                break

            self.iteration += 1

            if self.iteration_limit != -1 and self.iteration >= self.iteration_limit:
                print('reached iteration limit')
                break

            self.query_di(di)
            self.add_io_constraint(di)


        self.curkey = self.extract_key()
        print('last key=', end='')
        for kb in self.curkey:
            print(int(kb), end='')
        print()

        self.check_key(self.curkey)

        print('time: {0:.4f}'.format(time.time() - tm0))

        return


if __name__ == '__main__':
    if len(sys.argv) != 5:
        print('usage: dec_z3.py <sim_cir> <enc_cir> <correct_key> <iterations>')
        exit(1)
    sim_cir = Circuit(sys.argv[1])
    enc_cir = Circuit(sys.argv[2])

    corrkey = [int(x) for x in (sys.argv[3].split('=')[1])]

    cdec = CirDecryptScaZ3(sim_cir, enc_cir, corrkey, int(sys.argv[4]))
    cdec.solve_pwr()

