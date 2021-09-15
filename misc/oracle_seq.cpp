
#include "base/circuit.h"
#include <vector>
#include <string>

int main(int argc, char** argv) {

    using namespace std;
    using namespace ckt;

    circuit cir("./bench/testing/s400.bench");

    vector<id2boolmap> smaps;

    while (true) {

    	std::string instr;
    	std::getline(std::cin, instr);
    	std::stringstream ss(instr);
    	std::string tok;

    	//std::cout << "input string is " << instr << "\n";

    	smaps.clear();

    	while ( std::getline(ss, tok, ' ') ) {
			smaps.push_back(id2boolmap());
			int i = 0;

			if (tok.size() != cir.nInputs()) {
				std::cout << "error: mismatch between pattern length " << tok.size() << " and number of inputs " << cir.nInputs() << "\n";
				exit(1);
			}
			for (auto xid : cir.inputs()) {
				//std::cout << "arg: " << arg_vec.at(1)[i] << "\n";
				smaps.back()[xid] = (tok[i++] == '1');
			}
		}

		cir.simulate_seq(smaps);

		for (int d = 0; d < smaps.size(); d++) {
			for (auto yid : cir.outputs()) {
				std::cout << smaps[d].at(yid);
			}
			std::cout << " ";
		}

		std::cout << "\n";

    }

    return 0;
}
