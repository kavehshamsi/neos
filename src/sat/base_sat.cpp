/*
 * base_sat.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: kaveh
 */


#include <sat/base_sat.h>

namespace csat {

// get boolean value of sliteral from sat_solver
bool getValue(const sat_solver& S, slit p) {
	return S.get_value(p);
}

slit add_logic_clause(sat_solver& S,
		const std::vector<slit>& ins, fnct fn) {
	slit y = S.create_new_var();
	add_logic_clause(S, ins, y, fn);
	return y;
}

// add logic condition without wire mapping
void add_logic_clause(sat_solver& S,
		const std::vector<slit>& ins, slit y, fnct fn) {
	switch (fn) {
	case fnct::AND : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			S.add_clause(in, ~y);
			xins.push_back(~in);
		}
		xins.push_back(y);
		S.add_clause(xins);
		break;
	}
	case fnct::OR : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			S.add_clause(~in, y);
			xins.push_back(in);
		}
		xins.push_back(~y);
		S.add_clause(xins);
		break;
	}
	case fnct::NOR : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			S.add_clause(~in, ~y);
			xins.push_back(in);
		}
		xins.push_back(y);
		S.add_clause(xins);
		break;
	}
	case fnct::NAND : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			S.add_clause(in, y);
			xins.push_back(~in);
		}
		xins.push_back(~y);
		S.add_clause(xins);
		break;
	}
	case fnct::NOT : {
		slit a = ins[0];
		S.add_clause(~a, ~y);
		S.add_clause(a, y);
		break;
	}
	case fnct::BUF : {
		slit a = ins[0];
		S.add_clause(~a, y);
		S.add_clause(a, ~y);
		break;
	}
	case fnct::TBUF : {
		slit s = ins[0];
		slit a = ins[1];
		S.add_clause(~s, ~a, y);
		S.add_clause(~s, a, ~y);
		break;
	}
	case fnct::MUX : {
		assert(ins.size() == 3);
		slit s = ins[0];
		slit a = ins[1];
		slit b = ins[2];
		S.add_clause(s , y , ~a);
		S.add_clause(s , ~y , a);
		S.add_clause(~s , y , ~b);
		S.add_clause(~s , ~y , b);
		//S.add_clause(a , b , ~y);
		//S.add_clause(~a , ~b , y);
		break;
	}
	case fnct::XOR : {
		assert(ins.size() == 2);
		slit a = ins[0];
		slit b = ins[1];
		S.add_clause(~a , b , y);
		S.add_clause(a , ~b , y);
		S.add_clause(~a , ~b , ~y);
		S.add_clause(a , b , ~y);
		break;
	}
	case fnct::XNOR : {
		assert(ins.size() == 2);
		slit a = ins[0];
		slit b = ins[1];
		S.add_clause(~a , b , ~y);
		S.add_clause(a , ~b , ~y);
		S.add_clause(~a , ~b , y);
		S.add_clause(a , b , y);
		break;
	}
	case fnct::TMUX : {
		std::vector<slit> sellits;
		assert(ins.size() % 2 == 0);
		for (uint i = 0; i < ins.size() / 2; i++) {
			slit s = ins[i];
			sellits.push_back(s);
			slit a = ins[i + ins.size()/2];
			S.add_clause(~s, ~a, y);
			S.add_clause(~s, a, ~y);
		}
		S.add_clause(sellits);
		break;
	}
	case fnct::DFF : { // beware of the dffs
		std::cout << "warning : trying to add DFF clause\n";
		break;
	}
	default : {
		neos_abort("unkown gate during tseitin transform!\n");
		break;
	}
	}
}

// add logic condition without wire mapping
void get_logic_clause(vector<vector<slit>>& Cls,
		const std::vector<slit>& ins, slit y, fnct fn) {
	switch (fn) {
	case fnct::AND : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			Cls.push_back({in, ~y});
			xins.push_back(~in);
		}
		xins.push_back(y);
		Cls.push_back(xins);
		break;
	}
	case fnct::OR : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			Cls.push_back({~in, y});
			xins.push_back(in);
		}
		xins.push_back(~y);
		Cls.push_back(xins);
		break;
	}
	case fnct::NOR : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			Cls.push_back({~in, ~y});
			xins.push_back(in);
		}
		xins.push_back(y);
		Cls.push_back(xins);
		break;
	}
	case fnct::NAND : {
		std::vector<slit> xins;
		for ( auto& in : ins ){
			Cls.push_back({in, y});
			xins.push_back(~in);
		}
		xins.push_back(~y);
		Cls.push_back(xins);
		break;
	}
	case fnct::NOT : {
		slit a = ins[0];
		Cls.push_back({~a, ~y});
		Cls.push_back({a, y});
		break;
	}
	case fnct::BUF : {
		slit a = ins[0];
		Cls.push_back({~a, y});
		Cls.push_back({a, ~y});
		break;
	}
	case fnct::TBUF : {
		slit s = ins[0];
		slit a = ins[1];
		Cls.push_back({~s, ~a, y});
		Cls.push_back({~s, a, ~y});
		break;
	}
	case fnct::MUX : {
		assert(ins.size() == 3);
		slit s = ins[0];
		slit a = ins[1];
		slit b = ins[2];
		Cls.push_back({s , y , ~a});
		Cls.push_back({s , ~y , a});
		Cls.push_back({~s , y , ~b});
		Cls.push_back({~s , ~y , b});
		//Cls.push_back(a , b , ~y);
		//Cls.push_back(~a , ~b , y);
		break;
	}
	case fnct::XOR : {
		assert(ins.size() == 2);
		slit a = ins[0];
		slit b = ins[1];
		Cls.push_back({~a , b , y});
		Cls.push_back({a , ~b , y});
		Cls.push_back({~a , ~b , ~y});
		Cls.push_back({a , b , ~y});
		break;
	}
	case fnct::XNOR : {
		assert(ins.size() == 2);
		slit a = ins[0];
		slit b = ins[1];
		Cls.push_back({~a , b , ~y});
		Cls.push_back({a , ~b , ~y});
		Cls.push_back({~a , ~b , y});
		Cls.push_back({a , b , y});
		break;
	}
	case fnct::TMUX : {
		std::vector<slit> sellits;
		assert(ins.size() % 2 == 0);
		for (uint i = 0; i < ins.size() / 2; i++) {
			slit s = ins[i];
			sellits.push_back(s);
			slit a = ins[i + ins.size()/2];
			Cls.push_back({~s, ~a, y});
			Cls.push_back({~s, a, ~y});
		}
		Cls.push_back(sellits);
		break;
	}
	case fnct::DFF : { // beware of the dffs
		std::cout << "warning : trying to add DFF clause\n";
		break;
	}
	default : {
		neos_abort("unkown gate during tseitin transform!\n");
		break;
	}
	}
}


} // namespace c_sat

