/*
 * dbitset.h
 *
 *  Created on: Jun 30, 2020
 *      Author: kaveh
 */


#ifndef SRC_UTL_DBITSET_H_
#define SRC_UTL_DBITSET_H_

#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include <boost/dynamic_bitset.hpp>
#include <boost/functional/hash.hpp>

namespace std {
    template <typename Block, typename Alloc> struct hash<boost::dynamic_bitset<Block, Alloc> > {
        size_t operator()(boost::dynamic_bitset<Block, Alloc> const& bs) const {
            return boost::hash_value(bs.m_bits);
        }
    };
}


#endif /* SRC_UTL_DBITSET_H_ */
