/*
 * indexed_cont.h
 *
 *  Created on: Oct 24, 2019
 *      Author: kaveh
 */

#ifndef SRC_UTL_INDEXED_CONT_H_
#define SRC_UTL_INDEXED_CONT_H_

#include <vector>
#include <set>
#include <iostream>
#include <iterator>
#include <boost/format.hpp>

namespace utl {

#define fmt(format_str, format_args) ((boost::format(format_str) % format_args).str())
/*
 * indexed container. A container that uses a vector as the backened
 * and can emit IDs which can be used as indices into the vector.
 * If an ID larger than the vector size is added, the vector extends.
 * elements are linked together as a doubly linked list.
 */
template<typename T>
class indexed_cont {
private:
	typedef int32_t id;

	struct elem_t {
		T _t;
		bool alive = false;
		id _next = -1;
		id _prev = -1;
	};

	std::vector<elem_t> _v;

	std::set<id> _available_ids;

	id _last = -1;
	id _first = -1;

	uint32_t _num_elem = 0;

public:
	indexed_cont() {}

    class iterator {
    private:
        indexed_cont& owner;
        id x;
    public:
        iterator(indexed_cont& ic, id x) : owner(ic), x(x) {}
        iterator& operator++() {
            x = owner._v[x]._next;
            return *this;
        }

        std::pair<id, T&> operator*() const { return std::pair<id, T&>(x, owner._v[x]._t); }

        bool operator!=(const iterator& other) {
            return (&other.owner != &owner || other.x != x);
        }
        bool operator==(const iterator& other) {
            return (&other.owner == &owner && other.x == x);
        }
    };

    class const_iterator {
    private:
        const indexed_cont& owner;
        id x;
    public:
        const_iterator(const indexed_cont& ic, id x) : owner(ic), x(x) {}
        const_iterator& operator++() {
            x = owner._v[x]._next;
            return *this;
        }

        std::pair<id, const T&> operator*() { return std::pair<id, const T&>(x, owner._v[x]._t); }

        bool operator!=(const const_iterator& other) {
            return (&other.owner != &owner || other.x != x);
        }
        bool operator==(const const_iterator& other) {
            return (&other.owner == &owner && other.x != x);
        }
    };

    indexed_cont::iterator begin() {
        return indexed_cont::iterator(*this, _first);
    }

    indexed_cont::iterator end() {
        return indexed_cont::iterator(*this, -1);
    }

    indexed_cont::const_iterator begin() const {
        return indexed_cont::const_iterator(*this, _first);
    }

    indexed_cont::const_iterator end() const {
        return indexed_cont::const_iterator(*this, -1);
    }


    uint32_t size() const { return _num_elem; }


	id create_new_entry(const T& nt) {

		if (_v.size() == 0) {
			_first = 0;
		}

		id retid, olast = _last;

		if (_available_ids.empty()) {
			_v.push_back({nt, 1, -1, _last});
			retid = _last = _v.size() - 1;
		}
		else {
			retid = *_available_ids.begin();
			_available_ids.erase(retid);
			if (!entry_exists(retid + 1) && (retid + 1 != (id)_v.size()) )
				_available_ids.insert(retid + 1);
			assert(!entry_exists(retid));

			_v[retid] = {nt, 1, -1, _last};
			_v[_last]._next = retid;
			_last = retid;
		}

		if (olast != -1) {
    		_v[olast]._next = retid;
		}

		_num_elem++;

		return retid;
	}

	id create_new_entry() {
    	return create_new_entry(T());
    }

	bool entry_exists(id x) const {
		return (x < (id)_v.size() && _v[x].alive);
	}

	void insert_entry(id x, const T& nt) {
		if (entry_exists(x)) {
			throw std::out_of_range( fmt("indexed_cont: index %d already exists in the container", x) );
		}

        if (x >= (id)_v.size()) {
            _v.resize(x + 1);
        }
        else {
            _available_ids.erase(x);
        }

		if (_num_elem == 0) {
			_first = x;
		}

		_v[x] = {nt, 1, -1, _last};
		if (_last != -1)
    		_v[_last]._next = x;
		_last = x;

		_num_elem++;

	}

	void remove_entry(id x) {
		if (!entry_exists(x)) {
			throw std::out_of_range( fmt("indexed_cont: index %d not present for removal", x) );
		}

		auto& e = _v[x];
		e.alive = false;
		e._t = T(); // this rewrite should free some memory for large T objects
		if (e._prev != -1) {
			_v[e._prev]._next = e._next;
		}
		else {
			assert(x == _first);
			_first = e._next;
		}

		if (e._next != - 1) {
			_v[e._next]._prev = e._prev;
			_available_ids.insert(x);
		}
		else {
			assert(x == _last);
			_v.pop_back();
			_last = e._prev;
		}

		_num_elem--;

	}

	// inserts element at x at initializes to default constructor
	// imitating the map operator[] semantics
	T& operator[](id x) {
		if (entry_exists(x))
			return _v[x]._t;

		insert_entry(x, T());
		return _v[x]._t;
	}

	T& at(id x) const {
		if (x >= (id)_v.size()) {
			throw std::out_of_range( fmt("indexed_cont: the index %d is out of container range (%d)", x % _v.size()) );
		}

		// casting to non-const: even though at is a const function it can be used to modify
		// the data without modifying the containers id-link-chain or creating new entries
		elem_t& re = (elem_t&)_v[x];
		if (!re.alive) {
			throw std::out_of_range( fmt("indexed_cont: element at index %d is not alive", x) );
		}
		return re._t;
	}

    indexed_cont::const_iterator find(id x) const {
    	if (entry_exists(x))
    		return indexed_cont::const_iterator(*this, x);
    	else
    		return end();
    }

    indexed_cont::iterator find(id x) {
    	if (entry_exists(x))
    		return indexed_cont::iterator(*this, x);
    	else
    		return end();
    }

    void clear() {
    	_v.clear();
    	_available_ids.clear();

    	_last = -1;
    	_first = -1;
    	_num_elem = 0;
    }

	void print_cont() {
	    std::cout << " (" << _first << ", " << _last << ")\n";
        for (uint i = 0; i < _v.size(); i++) {
            std::cout << fmt("%d:%d:%d->(%d,%d)\n", i % _v[i].alive % _v[i]._t % _v[i]._prev % _v[i]._next);
        }

        std::cout << "ranged\n";
        for (const auto& x : *this) {
            std::cout << fmt("%d:%d\n", x.first % x.second);
        }
	}
};

}



#endif /* SRC_UTL_INDEXED_CONT_H_ */
