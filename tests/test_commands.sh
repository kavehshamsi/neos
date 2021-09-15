#!/bin/sh
set -x

./bin/neos -e xor ./bench/comb/c432.bench ./bench/testing/c432_xor.bench 20
./bin/neos -d ex ./bench/comb/c432.bench ./bench/testing/c432_xor.bench 50
./bin/neos -d hill ./bench/comb/c432.bench ./bench/testing/c432_xor.bench

./bin/neos -e xor ./bench/seq/s27.bench ./bench/testing/s27_enc.bench 10
./bin/neos -d int ./bench/seq/s27.bench ./bench/testing/s27_enc.bench 50
