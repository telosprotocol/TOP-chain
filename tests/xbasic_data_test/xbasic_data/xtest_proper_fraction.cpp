#include "xbasic/xcodec/xmsgpack/xproper_fraction_codec.hpp"
#include "xbasic/xproper_fraction.hpp"
#include "xcodec/xmsgpack_codec.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(xdata_test_proper_fraction, _1) {
    top::xproper_fraction_t<uint64_t, float> n1(100, 1000);
    std::cout << n1.value() << std::endl;
    n1.set_num(1000);
    std::cout << n1.value() << std::endl;
    n1.set_num(10000);
    std::cout << n1.value() << std::endl;

    // top::xproper_fraction_t<uint64_t, float> n2(1000, 100); // assert false
    // std::cout << n2.value() << std::endl;
}

using test_type = top::xproper_fraction_t<uint64_t, float>;

TEST(xdata_test_proper_fraction, _serde) {
    test_type n(3, 7);
    std::cout << n.value() << std::endl;

    auto bytes = top::codec::msgpack_encode(n);
    std::string object = {std::begin(bytes), std::end(bytes)};

    test_type res = top::codec::msgpack_decode<test_type>({object.begin(), object.end()});
    std::cout << res.value() << std::endl;
}