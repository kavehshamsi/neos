/*
 * base.h
 *
 *  Created on: Dec 8, 2017
 *      Author: kaveh
 */

#ifndef SRC_BASE_BASE_H_
#define SRC_BASE_BASE_H_

#include <stack>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>
#include <iostream>

#include "utl/idmap.h"

#define neos_error(str) \
	{ std::cout << "\nneos_error: " << str << "\n";\
	exit(1);\
	throw 0; } // suppresses return value warnings

#define neos_abort(str) \
	{ std::cout << "\nneos internal error: " << str << "\n";\
	assert(false);\
	throw 0; }

#define neos_under_construction(str) \
	{ neos_abort("under construction"); }

#define neos_check(cond, message) \
	{ if (!(cond)) \
		neos_abort(message); }

#define neos_println(str) \
	{ std::cout << str << "\n"; }

#define neos_println_l1(str) \
	{ std::cout << "  " << str << "\n"; }

#define neos_println_l2(str) \
	{ std::cout << "    " << str << "\n"; }

#define neos_println_l3(str) \
	{ std::cout << "      " << str << "\n"; }


#define V1(code) { if (verbose >= 1) { code ; } else {} }
#define V2(code) { if (verbose >= 2) { code ; } else {} }
#define V3(code) { if (verbose >= 3) { code ; } else {} }


#define D1(code)
#define D2(code)
#define D3(code)

#ifndef NEOS_DEBUG_LEVEL
	#define NEOS_DEBUG_LEVEL 0
#endif

#if NEOS_DEBUG_LEVEL >= 1
	#undef D1
	#define D1(code)\
	 code ;
#endif
#if NEOS_DEBUG_LEVEL >= 2
	#undef D2
	#define D2(code)\
	 code ;
#endif
#if NEOS_DEBUG_LEVEL >= 3
	#undef D3
	#define D3(code)\
	 code ;
#endif

inline void neos_system(const std::string& str) {
	if ( system((str).c_str()) == -1 ) {
		neos_error("system cmd " << str << " failed");
	}
}

// system with supressed warnings
#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define system_cl(cmd) int CONCAT(_macro_val, __LINE__) = system(cmd); (void )CONCAT(_macro_val, __LINE__)

namespace ckt {

enum class fnct {
	UNDEF = 0,
	AND,
	ANDN,
	NAND,
	NOT,
	NOR,
	OR,
	XOR,
	XNOR,
	BUF,
	TBUF,
	MUX,
	TMUX,
	LUT,
	DFF,
	INST
};

typedef long long int 						id;
typedef std::vector<id> 				idvec;
typedef std::set<id> 					oidset; // ordered id set
typedef std::unordered_set<id> 			idset; // unordered id set
typedef std::pair<id, id> 				idpair;
typedef std::stack<id> 					idstack;
typedef std::queue<id>					idque;
typedef utl::idmap<id> 					id2idmap;
typedef std::unordered_map<id, id>  	id2idHmap;
typedef std::map<std::string,id>		name2idmap;
typedef std::vector<bool>				boolvec;
typedef utl::idmap<bool> 				id2boolmap;
typedef std::unordered_map<id,bool> 	id2boolHmap;
typedef utl::idmap<boolvec> 			id2boolvecmap;

}

// all c++ style experts agree that this is a horrible idea...
using std::string;
using std::vector;

template <typename K, typename D>
using Hmap = std::unordered_map<K, D>;


template <typename K, typename D>
using Omap = std::map<K, D>;

template <typename K>
using Hset = std::unordered_set<K>;

template <typename K>
using Oset = std::set<K>;


#endif /* SRC_BASE_BASE_H_ */
