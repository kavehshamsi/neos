/*
 * utility.h
 *
 *  Created on: Sep 21, 2016
 *      Author: kaveh
 */

#ifndef UTILITY_CONT_H_
#define UTILITY_CONT_H_

#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <array>
#include <cstdarg>
#include <chrono>
#include <bitset>
#include <cmath>
#include <boost/format.hpp>

#include "utl/idmap.h"

namespace utl {

#define fmt(format_str, format_args) ((boost::format(format_str) % format_args).str())

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

template<typename T, typename K>
inline bool _is_in(const T& x, const std::vector<K>& vec) {
	return std::find(vec.begin(), vec.end(), x) != vec.end();
}

template<typename T, typename K>
inline bool _is_not_in(const T& x, const std::vector<K>& vec) {
	return std::find(vec.begin(), vec.end(), x) == vec.end();
}

template<typename T, typename S>
inline bool _is_in(const T& x, const S& setormap) {
	return setormap.find(x) != setormap.end();
}

template<typename T, typename S>
inline bool _is_not_in(const T& x, const S& setormap) {
	return setormap.find(x) == setormap.end();
}

template<typename T>
inline bool _is_in(const T& x, const std::string& setormap) {
	return setormap.find(x) != std::string::npos;
}

template<typename T>
inline bool _is_not_in(const T& x, const std::string& str) {
	return str.find(x) == std::string::npos;
}

inline bool _starts_with(const std::string& sub, const std::string& super) {
	return (super.substr(0, sub.size()) == sub);
}

/**
 *  return random boolean vector of lentgh n
 */
inline std::vector<bool> random_boolvec(uint n) {

	std::vector<bool> ret(n,0);
	for (uint i = 0; i < n; i++) {
		ret[i] = ( (rand() % 2) == 0 );
	}
	return ret;
}

template<typename T>
inline int64_t find_vec_index(const std::vector<T>& vec, const T& e) {
	for (size_t i = 0; i < vec.size(); i++)
		if (vec[i] == e)
			return i;
	return -1;
}

/**
 * flip vector in-place
 */
template <typename T>
inline void flip_vec(std::vector<T>& vec){
	std::vector<T> temp;
	temp = vec;

	for (uint i = 0; i < vec.size(); i++){
		vec[i] = temp[vec.size() - i - 1];
	}
}

template <typename T, typename B>
T clip(const T& n, const B& lower, const B& upper) {
  return MAX(lower, MIN(n, upper));
}

/**
 * print vector content
 */
template <typename T>
inline void printvec(const std::vector<T>& vec, const std::string& delim = ""){
	for (auto x : vec){
		std::cout << x << delim;
	}
}

// simple swap
template <typename T>
inline void swap(T& a, T& b) {
	T t = a;
	a = b;
	b = t;
}

/**
 * container translations
 */
template<typename T, template<typename> class S>
inline std::vector<T> to_vec(const S<T>& incont) {
	return std::vector<T>(incont.begin(), incont.end());
}

template<typename T, template<typename> class S>
inline std::unordered_set<T> to_hset(const S<T>& incont) {
	std::unordered_set<T> retset(incont.begin(), incont.end());
	return retset;
}

template<typename T, template<typename> class S>
inline std::set<T> to_oset(const S<T>& incont) {
	std::set<T> retset(incont.begin(), incont.end());
	return retset;
}

template<typename T, template<typename> class S>
inline std::queue<T> to_que(const S<T>& incont) {
	std::queue<T> retq;
	for (const auto& x : incont) {
		retq.push(x);
	}
	return retq;
}


/*
 * ranged containter to delimited string
 */
template <typename M>
inline std::string to_delstr(const M& set, const std::string& del = ""){
	std::stringstream retss;
	uint i = 0;
	for (const auto& e : set) {
		if (i++ != set.size() - 1)
			retss << e << del;
		else
			retss << e;
	}
	return retss.str();
}

template<typename T>
inline std::vector<T> linspace(T a, T b, T step) {
	assert(a < b);
	assert(step != 0);
	assert((a - b) / step <= 2e9);
	std::vector<T> retvec;
	for (T i = a; i < b; i += step) {
		retvec.push_back(i);
	}
	return retvec;
}

/**
 * print set content
 */
template <typename T, typename H>
inline void printset(const std::set<T, H>& inset){
	std::cout << "{";
	for (auto x : inset){
		std::cout << x << ", ";
	}
	std::cout << "}\n";
}

template <typename T, typename H>
inline void printset(const std::unordered_set<T, H>& inset){
	std::cout << "{";
	for (auto x : inset){
		std::cout << x << ", ";
	}
	std::cout << "}\n";
}


/**
 * print vector content
 */
template <typename T>
inline void printvec(const std::string& str, const std::vector<T>& vec){
	std::cout << str;
	for (auto x : vec){
		std::cout << x;
	}
	std::cout << "\n";
}

inline std::vector<bool> to_boolvec(uint val, uint N, bool reverse = false) {
	std::vector<bool> vec(N, 0);
	uint acc = val;
	for (uint i = 0; i < N; i++) {
		vec[i] = acc & 1;
		acc >>= 1;
	}
	if (reverse)
		std::reverse(vec.begin(), vec.end());
	return vec;
}

inline int hex_to_int(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    throw std::invalid_argument("could not hex-convert to int");
    return -1;
}

/**
 * convert vector to string
 */
template <typename T>
inline std::string to_str(const std::vector<T>& vec, const std::string& delim = ""){
	std::stringstream retss;
	for (size_t i = 0; i < vec.size(); i++){
		retss << vec[i];
		if (i != vec.size() - 1)
			retss << delim;
	}
	return retss.str();
}

template<typename T, size_t N>
inline std::string to_str(const std::array<T, N>& arr, const std::string& delim = "") {
	std::stringstream retss;
	for (auto x : arr) {
		retss << x << delim;
	}
	return retss.str();
}


/**
 * remove from vec by value O(n) time
 * FIXME: get rid of this
 */
template<typename T>
inline void erase_byval(std::vector<T>& invec, T elem) {
	while (true) {
		auto it = std::find(invec.begin(), invec.end(), elem);
		if (it != invec.end())
			invec.erase(it);
		else
			break;
	}
}

template<typename T>
inline void erase_byindex(std::vector<T>& invec, size_t x) {
	invec.erase(invec.begin() + x);
}

/**
 * get a vector of pairs of a map sorted by their value
 */
template<typename K, typename V>
inline std::vector< std::pair<K, V> >
get_sortbyval_vec(std::map<K, V>& inmap) {
	std::vector<std::pair<K, V> > retvec(inmap.begin(), inmap.end());
	std::sort(retvec.begin(), retvec.end(),
			[](const std::pair<K, V>& a, const std::pair<K, V>& b)
			{ return a.second > b.second;} );
	return retvec;
}

/**
 *  dequeue operation which is missing in the STL
 */
template<typename T>
inline T utldeque(std::queue<T>& que){
	T tmp = que.front();
	que.pop();
	return tmp;
}

/**
 * append to end of vector
 */
template<typename T, typename R>
inline void push_all(std::vector<T>& vec1, const R& r2) {
	vec1.insert(vec1.end(), r2.begin(), r2.end());
}

/**
 * cut from end of vector
 */
template<typename T>
inline void pop_all(std::vector<T>& vec1, const std::vector<T>& vec2) {
	for (size_t i = 0; i < vec2.size(); i++)
		vec1.pop_back();
}

/**
 * append to end of queue
 */
template<typename Q, typename S>
inline void push_all(Q& q1, const S& set1) {
	for (auto& x : set1)
		q1.push(x);
}

/**
 * advance function for container
 */
template<class T>
inline void advance(T& it, uint r) {
	for (uint i = 0; i < r; i++)
		it++;
}

/**
 *  dequeue operation which is missing in the STL
 */
template<typename T>
inline T deque(std::vector<T>& vec) {
	T tmp = vec.back();
	vec.pop_back();
	return tmp;
}

inline int64_t to_int(const std::vector<bool>& invec, bool msbfirst = false) {
	assert(invec.size() < 64);
	int64_t ret = 0;
	for (uint i = 0; i < invec.size(); i++) {
		if (msbfirst)
			ret += (invec[i] << i);
		else
			ret += (invec[i] << (invec.size()-i-1));
	}
	return ret;
}


/**
 * return subvector with inclusive range
 */

template<typename T>
inline std::vector<T> subvec(const std::vector<T>& vec, uint s, uint e) {
	assert(s >= 0);
	assert(e < vec.size());

	std::vector<T> retvec;
	for (uint i = s; i <= e; i++) {
		retvec.push_back(vec[i]);
	}

	return retvec;
}

inline size_t hamming_weight(const std::vector<bool>& invec) {
	size_t ret = 0;
	for (auto b : invec) {
		ret += int(b);
	}
	return ret;
}

/**
 * print str to the command line with red font.
 * works only in linux
 */
inline void printred(std::string str){
	std::cout << "\033[1;31m" << str << "\033[0m";
}
#define _COLOR_RED(str) "\033[1;31m" << str << "\033[0m"


/**
 * random subset from (0, ..., N)
 */

inline std::vector<size_t> rand_indices(size_t N, int32_t max = -1) {

	max = (max == -1) ? N:max;

	std::vector<size_t> retvec(max);
	for (size_t i = 0; i < max; i++) {
		retvec[i] = i;
	}
	std::random_shuffle(retvec.begin(), retvec.end());

	retvec.resize(N);

	return retvec;
}

/**
 * randomly pick N elements from a vector
 */
template<typename T>
inline std::vector<T>
rand_from_vec(const std::vector<T>& tvec, uint N) {
	assert (N <= tvec.size());
	typename std::vector<T> retvec;
	std::vector<size_t> rand_index_vec = rand_indices(tvec.size());

	for (uint i = 0; i < N; i++) {
		retvec.push_back(tvec[rand_index_vec[i]]);
	}

	return retvec;
}

/**
 * randomly pick an element from a set
 */
template<typename T, template<typename> class S>
inline T rand_from_set(const S<T>& tset) {

	assert(!tset.empty());
	auto it = tset.begin();
	uint r = rand() % tset.size();
	advance(it, r);

	return *it;
}

template<typename K, typename D, template<typename, typename> class M>
inline std::vector<K> map_firsts(const M<K, D>& mp) {
	std::vector<K> retvec;
	for (const auto& p : mp) {
		retvec.push_back(p.first);
	}
	return retvec;
}

/**
 * randomly pick n distinct elements from a set
 */
template<typename T, template<typename> class S>
inline S<T> rand_from_set(const S<T>& tset, uint N){
	assert (N <= tset.size());
	if (N == tset.size())
		return tset;

	typename std::vector<T> tmpvec = to_vec(tset);
	typename std::vector<T> retvec = rand_from_vec(tmpvec, N);
	S<T> retset(retvec.begin(), retvec.end());

	return retset;
}

/**
 * set bit in word
 */
template<typename T>
inline void setbit(T& word, uint loc, bool val) {
	if (val)
		word |= 1ULL << loc;
	else
		word &= ~(1ULL << loc);
}

/**
 * set bit in word
 */
template<typename T>
inline bool getbit(T word, uint loc) {
	return (word >> loc) & 1ULL;
}



/**
 * randomly pick an element from a vector
 */
template<typename T>
inline T rand_from_vec(const std::vector<T>& tvec){
	uint r = rand() % tvec.size();
	return tvec[r];
}


/**
 * return the set union of r1, and r2
 */
template<typename S>
inline S set_union(const S& r1, const S& r2){
	auto ret = r1;
	ret.insert(r2.begin(), r2.end());
	return ret;
}

/**
 * return the set union of r1, and r2
 */
template<typename S>
inline S set_intersect(const S& r1, const S& r2){
	S ret;
	for (const auto& x : r1) {
		if (_is_in(x, r2)) {
			ret.insert(x);
		}
	}
	return ret;
}


/**
 * subtract s2 from s1
 */
template<typename S, typename R>
inline S set_subtract(const S& s, const R& r){
	auto ret = s;
	for (auto x : r) {
		ret.erase(x);
	}
	return ret;
}

/**
 * subtract s2 from s1
 */
template<typename S, typename R>
inline bool contains(const S& sub, const R& sup) {
	for (const auto& x : sub) {
		if (_is_not_in(x, sup))
			return false;
	}

	return true;
}


/**
 * union r2 into r1. modifies r1
 */
template<typename S, typename R>
inline void add_all(S& r1, const R& r2){
	r1.insert(r2.begin(), r2.end());
}


/**
 * devide set to separate vecs,
 *
 */
template<typename T>
inline void devide_set2vec(const std::set<T>& inset,
		std::vector<T> vecs[], uint k){

	uint setsize = inset.size();
	assert(setsize % k == 0);
	std::vector<T> invec(inset.begin(), inset.end());

	uint vecsize = setsize/k;

	for (uint i = 0; i < k; i++) {
		vecs[i] = std::vector<T>(invec.begin() + i*vecsize,
				invec.begin() + (i+1)*vecsize);
	}
}
/*
 * converted to ordered set
 */

template<typename T>
inline std::set<T> ordered(const std::unordered_set<T>& inset) {
	std::set<T> retset;
	for (auto& x : inset)
		retset.insert(x);
	return retset;
}

/*
template<typename T>
inline void _devide_set2vec(const std::unordered_set<T>& inset,
		std::vector<T> vecs[], uint k){

	uint setsize = inset.size();
	assert(setsize % k == 0);
	std::vector<T> invec(inset.begin(), inset.end());

	uint vecsize = setsize/k;

	for (uint i = 0; i < k; i++) {
		vecs[i] = std::vector<T>(invec.begin() + i*vecsize,
				invec.begin() + (i+1)*vecsize);
	}
}
*/

/*
 * return immediate empty set for default arguments
 */
template<class T>
inline T empty(void){
	T retT;
	return retT;
}

/*
 * disabling and enabling stdout
 */
inline void _disable_stdout() {
	std::cout.setstate(std::ios_base::failbit);
}
inline void _enable_stdout() {
	std::cout.clear();
}

/*
 * time probes
 */
inline clock_t _start_cpu_timer() {
	auto start = std::clock();
	return start;
}

inline clock_t _stop_cpu_timer(clock_t& start) {
	auto stop = std::clock();
	return (stop - start);
}

typedef std::chrono::time_point<std::chrono::system_clock,
		std::chrono::nanoseconds> cpp_time_t;

inline cpp_time_t _start_wall_timer() {
	auto start = std::chrono::system_clock::now();
	return start;
}

inline double _stop_wall_timer(cpp_time_t& start) {
	auto stop = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double> >(stop - start);
	return elapsed.count();
}


/*
 * file path utitlities
 */
inline std::string _base_name(const std::string& path) {
	return path.substr(path.find_last_of("/") + 1);
}

inline std::string _base_path(const std::string& path) {
	return path.substr(0, path.find_last_of("/"));
}

inline std::string getexepath() {
	char result[1000];
	ssize_t count = readlink( "/proc/self/exe", result, 1000 );
	return std::string( result, (count > 0) ? count : 0 );
}

inline std::string getexedir() {
	return _base_path(getexepath());
}

inline bool _file_exists(const std::string& path) {
    std::ifstream infile(path);
    return infile.good();
}

inline void insert_in_file_atline(const std::string filename, uint line_pos, const std::string& line2add) {
	std::ifstream inf(filename);
	assert(inf.good());
	std::vector<std::string> lines;
	std::string line;
	while (getline(inf, line)) {
		lines.push_back(line);
	}
	inf.close();

	assert(line_pos < lines.size());

	lines.insert(lines.begin() + line_pos, line2add);

	std::ofstream onf(filename, std::ios::out);

	for (auto& x : lines) {
		onf << x << "\n";
	}
}

inline std::string _remove_extention(std::string const& filename) {
	std::string::size_type const p(filename.find_last_of("."));
	return p > 0 && p != std::string::npos
			? filename.substr(0, p) : filename;
}


inline bool _replace(std::string& str,
		const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if(start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

inline std::vector<std::string> _split(const std::string& str, char delim) {
	std::stringstream ss(str);
	std::vector<std::string> ret;
	std::string piece;
	while (std::getline(ss, piece, delim)) {
		ret.push_back(piece);
	}
	return ret;
}




/*
 * random one-time STL container visitor class
 */

#define for_rand_vec_traversal(x, invec, invec_type)  \
	if (bool _macro_bool_k = true) \
		for (auto rvis = random_vector_visitor<invec_type>(invec);\
			_macro_bool_k; _macro_bool_k = false)\
			for (invec_type x = rvis.start(); !rvis.is_done(); x = rvis.next(), (void)x)

#define for_rand_set_traversal(x, inset, inset_type) \
	if (bool _macro_bool_k = true) \
		for (auto rvis = random_vector_visitor<inset_type>(to_vec(inset));\
			_macro_bool_k; _macro_bool_k = false)\
			for (inset_type x = rvis.start(); !rvis.is_done(); x = rvis.next(), (void)x)

template<typename T>
class random_vector_visitor {

private:

	std::vector<T> vec;
	bool done;
	typename std::vector<T>::iterator it;

public:
	random_vector_visitor(const std::vector<T>& invec) : vec(invec), done(false) {
		if (invec.size() == 0)
			done = true;
		std::random_shuffle(vec.begin(), vec.end());
		it = vec.begin();
	};

	const T& start() { return *it; }

	bool is_done() { return done; }

	const T& next() {
		if (it == vec.end()-1) {
			done = true;
		}
		else
			++it;
		return *it;
	}
};


}

#endif /* UTILITY_CONT_H_ */
