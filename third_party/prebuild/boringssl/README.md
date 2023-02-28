### What this directory for?

This directory stores static library of boringssl built in GCC 9.

For now, our Project relay on GCC 4.8.5. While boringssl, which is dependent on xquic, need higher version compiler like GCC 9. 


### build boringssl with prefix to avoid symbols conflicts

more information reference: > boringssl BUILDING.md `### Building with Prefixed Symbols`

``` BASH
# cd boringssl && mkdir build && cd build 

cmake -DBUILD_SHARED_LIBS=0 -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC" -DBORINGSSL_PREFIX=FORQUIC -DBORINGSSL_PREFIX_SYMBOLS=../../boringssl_static_libs/prefixed_symbols.txt ..

make ssl -j4

cp -r ssl/libssl.a ../../boringssl_static_libs/libbrssl.a
cp -r crypto/libcrypto.a ../../boringssl_static_libs/libbrcrypto.a
cp -r symbol_prefix_include/boringssl_prefix_symbols.h ../../boringssl_static_libs/

```

NOTED: PREFIX `-DBORINGSSL_PREFIX=FORQUIC` MUST be same as `CMakeLists.txt`'s `add_definitions(-DBORINGSSL_PREFIX=FORQUIC)`

prefixed_symbols.txt stores all symbols that conflicts with openssl for now. might need to update in the future.