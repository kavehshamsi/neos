/*
 * indexed_cont.h
 *
 *  Created on: Oct 24, 2019
 *      Author: kaveh
 */

#ifndef SRC_UTIL_IDMAP_H_
#define SRC_UTIL_IDMAP_H_

#include <vector>
#include <set>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include <map>
#include <chrono>
#include <functional>
#include <boost/format.hpp>

namespace utl {

#define fmt(format_str, format_args) ((boost::format(format_str) % format_args).str())

/*
 * indexed container. A container that uses a vector as the backened
 * and can emit IDs which can be used as indices into the vector.
 * If an ID larger than the vector size is added, the vector extends.
 * elements are linked together as a doubly linked list. Supports multiple
 * 'views' with the NV parameter which are subsets of the overall list.
 */
template<typename T, int8_t NV = 1, typename ID = long long int>
class idmap {
private:
	typedef ID ind;

	struct elem_t {
        std::pair<ind, T> _t;
		bool alive = false;
		ind _next = -1;
		ind _prev = -1;
	};

	std::vector<elem_t> _v;

	int8_t active_view = 0;

	ind _last[NV];
	ind _first[NV];

	ind _last_removed = -1;

	uint32_t _num_elem[NV] = {0};

public:

	typedef ID key_type;
	typedef T mapped_type;
	typedef T data_type;

	idmap() {
	    init();
	}
	
	idmap(size_t sz) {  
	    init();
	    resize(sz);
	}

	idmap(const std::vector<T>& vec) {
	    init();
		resize(vec.size());
		for (size_t i = 0; i < vec.size(); i++) {
			(*this)[i] = vec[i];
		}
	}

	idmap(const std::unordered_map<ind, T>& hmp) {
	    init();
		for (const auto& p : hmp) {
			(*this)[p.first] = p.second;
		}
	}

    template<bool is_constant>
    class base_iterator {
    private:
        using ret_pair_ref_t = typename std::conditional<is_constant,
                    const std::pair<ind, T>&, std::pair<ind, T>&>::type;
        using ret_pair_pointer_t = typename std::conditional<is_constant,
                    const std::pair<ind, T>*, std::pair<ind, T>*>::type;
        using own_ref_t = typename std::conditional<is_constant,
						const idmap&, idmap&>::type;
        own_ref_t& owner;
        ind x;

        friend class idmap;
    public:
        base_iterator(own_ref_t& ic, ind x) : owner(ic), x(x) {}
        base_iterator(const base_iterator& itb) : owner(itb.owner), x(itb.x) {}

        base_iterator& operator=(const base_iterator& itb) { assert(&owner == &itb.owner); x = itb.x; return *this; }

        base_iterator operator++(int) {
        	auto ret = *this;
            x = owner._v[x]._next;
            return ret;
        }
        base_iterator& operator++() {
            x = owner._v[x]._next;
            return *this;
        }

        template< class = std::enable_if<is_constant> >
        base_iterator<is_constant>(const base_iterator<is_constant>& rhs) : owner(rhs.owner), x(rhs.x) {}  // OK

        // implicit conversion from iterator to const_iterator
        template< class = std::enable_if<!is_constant> >
        operator base_iterator<true>() const { return base_iterator<true>(owner, x); }

        ret_pair_ref_t operator*() const {
            return owner._v[x]._t;
        }

        ret_pair_pointer_t operator->() const {
            return &(owner._v[x]._t);
        }

        bool operator!=(const base_iterator& other) {
            return (&other.owner != &owner || other.x != x);
        }
        bool operator==(const base_iterator& other) {
            return (&other.owner == &owner && other.x == x);
        }
    };

    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;

    idmap::const_iterator begin() const {
        return idmap::const_iterator(*this, _first[active_view]);
    }

    idmap::const_iterator end() const {
        return idmap::const_iterator(*this, -1);
    }

    idmap::const_iterator cbegin() const {
        return idmap::const_iterator(*this, _first[active_view]);
    }

    idmap::const_iterator cend() const {
        return idmap::const_iterator(*this, -1);
    }

    idmap::iterator begin() {
        return idmap::iterator(*this, _first[active_view]);
    }

    idmap::iterator end() {
        return idmap::iterator(*this, -1);
    }

    // sets the active view
    idmap<T, NV>& view(int8_t av) const {
    	((idmap<T, NV>*)this)->active_view = av;
    	return *((idmap<T, NV>*)this);
    }

    void reserve(ind sz) { _v.reserve((size_t)sz); }
    void resize(ind sz) {
        assert(sz != 0);
        ind ols = (ind)_v.size();
        _v.resize(sz);
        for (ind i = ols; i < sz - 1; i++) {
            _push_to_removed(i);
        }
    }

    uint32_t size() const { return _num_elem[active_view]; }
    uint32_t size(int8_t av) const { return _num_elem[av]; }
    uint32_t max_id() const { return _v.size(); }
    bool empty() const { return _num_elem[active_view] == 0; }
    bool empty(int8_t av) const { return _num_elem[av] == 0; }

    inline ind create_new_entry(const T& nt) { return create_new_entry(nt, active_view); }
	ind create_new_entry(const T& nt, int8_t av) {

		ind retid, olast = _last[av];

		if (_last_removed == -1) {
			retid = _v.size();
			_v.push_back( {{retid, nt}, true, -1, _last[av]} );
			_last[av] = retid;
		}
		else {
			retid = _pop_from_removed();
			assert(!_v[retid].alive);
			_v[retid] = {{retid, nt}, true, -1, _last[av]};
			if (_last[av] != -1)
    			_v[_last[av]]._next = retid;
			_last[av] = retid;
		}

		if (olast != -1) {
    		_v[olast]._next = retid;
		}

		if (_num_elem[av]++ == 0) {
			_first[av] = retid;
		}

		return retid;
	}

	template<class ...Args>
	inline ind create_new_entry(Args... args) {
    	return create_new_entry(T(&args...));
    }

	inline ind create_new_entry() {
    	return create_new_entry(T());
    }

	inline bool entry_exists(ind x) const {
		return (x < (ind)_v.size() && x >= 0 &&  _v[x].alive );
	}

	void insert_entry(ind x, const T& nt) { insert_entry(x, nt, active_view); }
	void insert_entry(ind x, const T& nt, int8_t av) {
		/*if (entry_exists(x)) {
			throw std::out_of_range( fmt("indexed_cont: index %d already exists in the container", x) );
		}*/

        if (x >= (ind)_v.size()) {
            resize(x + 1);
        }
        else {
    		_del_from_removed(x);
        }

		_v[x] = {{x, nt}, 1, -1, _last[av]};
		if (_last[av] != -1)
    		_v[_last[av]]._next = x;
		_last[av] = x;
		
		if (_num_elem[av]++ == 0) {
			_first[av] = x;
		}
	}

	idmap::iterator erase(ind x) { return erase(x, active_view); }
	idmap::iterator erase(ind x, int8_t av) {
        if (!entry_exists(x))
            return end();

        return remove_entry(x, av);
    }
    
    

    void erase_ensure(ind x) { erase_ensure(x, active_view); }
    void erase_ensure(ind x, int8_t av) {
        if (!entry_exists(x))
        	throw std::out_of_range( fmt("indexed_cont: index %d not present for removal", x) );

        remove_entry(x, av);
    }

	// inserts element at x and initializes to default constructor
	// imitating the map operator[] semantics
	T& operator[](ind x) {
		if ( entry_exists(x) )
			return _v[x]._t.second;

		insert_entry(x, T());
		return _v[x]._t.second;
	}

	T& at(ind x) const {
		// I wish people would not pass negative indices to this map so that we could remove this check :-(
		// I don't want to trust casting x to unsigned which would be a possible solution
		if (x >= (ind)_v.size() || x < 0) {
			throw std::out_of_range( fmt("idmap: the index %d is out of container range (%d)", x % _v.size()) );
		}

		// casting to non-const: even though at() is a const function it can be used to modify
		// the data without modifying the containers id-link-chain or creating new entries
		elem_t& re = (elem_t&)_v[x];
		if (!re.alive) {
			throw std::out_of_range( fmt("idmap: element at index %d is not alive", x) );
		}
		return re._t.second;
	}

	// fast indexing. does not do bounds checking.
	T& atf(ind x) const {
		// casting to non-const: even though at() is a const function it can be used to modify
		// the data without modifying the containers id-link-chain or creating new entries
		elem_t& re = (elem_t&)_v[x];
		if (!re.alive) {
			throw std::out_of_range( fmt("idmap: element at index %d is not alive", x) );
		}
		return re._t.second;
	}

	// super-fast indexing. does not do bounds checking nor alive checking.
	T& atff(ind x) const {
		// casting to non-const: even though at() is a const function it can be used to modify
		// the data without modifying the containers id-link-chain or creating new entries
		return ((elem_t&)_v[x])._t.second;
	}


	bool operator==(const idmap<T, NV, ID>& mp) {
		for (const auto& p : mp) {
			auto it = find(p.first);
			if (it == end()) {
				return false;
			}
			else {
				if (it->second != p.second) {
					return false;
				}
			}
		}
		return true;
	}

	bool operator!=(const idmap<T, NV, ID>& mp) {
		return !((*this) == mp);
	}

    idmap::const_iterator find(ind x) const {
    	if (entry_exists(x))
    		return idmap::const_iterator(*this, x);
    	else
    		return end();
    }

    idmap::iterator find(ind x) {
    	if (entry_exists(x))
    		return idmap::iterator(*this, x);
    	else
    		return end();
    }

    void clear() {
    	_v.clear();
    	for (int8_t i = 0; i < NV; i++) {
			_last[i] = -1;
			_first[i] = -1;
			_num_elem[i] = 0;
    	}
    	_last_removed = -1;
    }

	void print_cont() {
	    for (int8_t av = 0; av < NV; av++) {
            std::cout << "av:" << (int)av << "(first:" << _first[av] << ", last:" 
                << _last[av] << ", last_removed:" << _last_removed << ")\n";
        }
        for (uint i = 0; i < _v.size(); i++) {
            std::cout << fmt("%d:%d:%d->(%d,%d)\n", i % _v[i].alive % _v[i]._t.second % _v[i]._prev % _v[i]._next);
        }

        std::cout << "ranged\n";
        for (const auto& x : *this) {
            std::cout << fmt("%d:%d\n", x.first % x.second);
        }

        std::cout << "available ids: " << _last_removed << " \n";
        for (ind x = _last_removed; x != -1; x = _v[x]._prev) {
            std::cout << x << "\n";
        }
	}

protected:

    void init() {
        for (int8_t av = 0; av < NV; av++) {
            _first[av] = -1;
            _last[av] = -1;
        }
    }

    // will return an iterator to the element that follows the deleted one.
	iterator remove_entry(ind x, int8_t av) {

		idmap::iterator ret = end();

		auto& e = _v[x];
		assert(e.alive && _num_elem[av] > 0);
		e.alive = false;
		e._t.second = T(); // this rewrite should free some memory for inflated T objects

		if (e._prev != -1) {
			_v[e._prev]._next = e._next;
		}
		else {
		    assert(_first[av] == -1 || x == _first[av]);
		    _first[av] = e._next;
		}

		if (e._next != - 1) {
			ret.x = e._next;
			_v[e._next]._prev = e._prev;
		}
		else {
			//assert(x == _last);
			_last[av] = e._prev;
		}

		/* special case: when last element is removed the
		 internal vector can be shrunk */
		if (x == _v.size() - 1) {
			_v.pop_back();
			for (ind y = x - 1; y >= 0; y--) {
				if (!_v[y].alive) {
					_del_from_removed(y);
					_v.pop_back();
				}
				else {
					break;
				}
			}
		}
		else {
			_push_to_removed(x);
		}

		_num_elem[av]--;

		return ret;
	}

    void _push_to_removed(ind x) {
        assert(x != _last_removed);
        //assert(!_v[x].alive);
        if (_last_removed == -1) {
            _v[x]._prev = -1;
            _v[x]._next = -1;
            _last_removed = x;
        }
        else {
            _v[_last_removed]._next = x;
            _v[x]._prev = _last_removed;
            _v[x]._next = -1;
            _last_removed = x;
        }
    }

    void _del_from_removed(ind x) {
        // std::cout << "deleting " << x << " from removed\n";
        if (_last_removed != -1) {
            assert(!_v[x].alive);
            ind pr = _v[x]._prev;
            ind nx = _v[x]._next;
            if (pr != -1) {
                //assert(!_v[pr].alive);
                _v[pr]._next = nx;
            }

            if (nx != -1) {
                //assert(!_v[nx].alive);
                _v[nx]._prev = pr;
            }
            else {
                //assert(x == _last_removed);
                _last_removed = pr;
            }
        }
    }

    ind _pop_from_removed() {
        //assert(_last_removed != -1);
        ind ret = _last_removed;
        _last_removed = _v[ret]._prev;
        if (_last_removed != -1) {
            _v[_last_removed]._next = -1;
            //assert(_v[_last_removed].alive == false);
        }

        return ret;
    }
};

}



#endif /* SRC_UTIL_IDMAP_H_ */

//#define IDMAP_PROFILE
#ifdef IDMAP_PROFILE

#include <iomanip>

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

int main() {
    using namespace utl;
        
    const size_t sz = 1000000;
    
    struct nln {
        int x = 0;
        std::vector<int> v;
    };

    std::map<int, nln> hmap;
    hmap[0];
    hmap[1];

    for (const auto& p : hmap) {
        std::cout << typeid(p).name() << "\n";
        //p.second.x = 1;
    }

    for (auto& p : hmap) {
        std::cout << typeid(p).name() << "\n";
        p.second.x = 1;
    }


    utl::idmap<nln> nlnmp;

    nlnmp[0];
    nlnmp[2];
//
//
//    std::pair<int, nln&> pr(0, nlnmp.at(0));
//    const auto& prr = pr;
//    prr.second = nln();
//
    for (auto& p : nlnmp) {
        std::cout << typeid(p).name() << "\n";
        std::cout << p.second.x << "\n";
        p.second.x = 1;
    }
    std::cout << "\n";
//
//
    for (const auto& p : nlnmp) {
        std::cout << typeid(p).name() << "\n";
        std::cout << p.second.x << "\n";
        //p.second.x = 1;
    }


   std::unordered_map<int, int> hmp;
   idmap<int, 1> mp(sz);
   mp.view(0);

   //mp.print_cont();


   int arr[sz] = {0};
   std::vector<int> vec(sz, 0);

   auto tm = _start_wall_timer();
   for (uint32_t i = 0; i < sz; i++) {
       vec[i] = i;
   }
   auto vec_tm = _stop_wall_timer(tm);

   tm = _start_wall_timer();
   for (uint32_t i = 0; i < sz; i++) {
       mp[i] = i;
   }
   auto mp_tm = _stop_wall_timer(tm);

   tm = _start_wall_timer();
   for (uint32_t i = 0; i < sz; i++) {
       hmp[i] = i;
   }
   auto hmp_tm = _stop_wall_timer(tm);

   tm = _start_wall_timer();
   for (uint32_t i = 0; i < sz; i++) {
       arr[i] = i;
   }
   auto arr_tm = _stop_wall_timer(tm);

   tm = _start_wall_timer();
   for (auto x : mp) {
       x.second = 1;
   }
   auto imp_tm = _stop_wall_timer(tm);


   std::cout << "time arr  : " << arr_tm << "\n";
   std::cout << "time vec  : " << vec_tm << "\n";
   std::cout << "time map  : " << mp_tm << "\n";
   std::cout << "time map  : " << hmp_tm << "\n";
   std::cout << "time map  : " << imp_tm << "\n";
   std::cout << "hmap ratio: " << std::setprecision(3) << (hmp_tm/arr_tm) << "\n";
   std::cout << "map ratio: " << std::setprecision(3) << (mp_tm/arr_tm) << "\n";
   std::cout << "vec ratio: " << std::setprecision(3) << (vec_tm/arr_tm) << "\n";
   std::cout << "imap ratio: " << std::setprecision(3) << (imp_tm/arr_tm) << "\n";

   std::cout << arr[sz - 1] << "\n";
   std::cout << vec[sz - 1] << "\n";
    
    return 0;
}  
    
    
  
#endif

