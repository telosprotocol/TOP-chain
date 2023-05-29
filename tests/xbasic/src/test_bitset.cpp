// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xbitset.h"

#include <gtest/gtest.h>

NS_BEG2(top, tests)

TEST(xbitset_t, build_from_uint) {
    auto bs1 = xbitset_t<8>::build_from(0x01u);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x01u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), xbytes_t{0x01u});
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), xbytes_t{0x01u});

    bs1 = xbitset_t<8>::build_from(0xf0u);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), xbytes_t{0xf0u});
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), xbytes_t{0xf0u});

    bs1 = xbitset_t<8>::build_from(0x23u);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x23u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x23u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x23u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x23u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), xbytes_t{0x23u});
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), xbytes_t{0x23u});

    bs1 = xbitset_t<8>::build_from(0x00u);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), xbytes_t{0x00u});
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), xbytes_t{0x00u});

    bs1 = xbitset_t<8>::build_from(0xffu);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), xbytes_t{0xffu});
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), xbytes_t{0xffu});

    bs1 = xbitset_t<8>::build_from(0x1234u);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x34u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x34u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x34u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x34u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), xbytes_t{0x34u});
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), xbytes_t{0x34u});

    auto bs2 = xbitset_t<16>::build_from(0x00u);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x00u, 0x00u}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00u, 0x00u}));

    bs2 = xbitset_t<16>::build_from(0x1234u);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x34u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x1234u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x1234u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x1234u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x34u, 0x12u}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x12u, 0x34u}));

    bs2 = xbitset_t<16>::build_from(0x12345678u);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x78u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x5678u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x5678u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x5678u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x78u, 0x56u}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x56u, 0x78u}));

    auto bs3 = xbitset_t<32>::build_from(0x00u);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::little), (xbytes_t{0x00u, 0x00u, 0x00u, 0x00u}));
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00u, 0x00u, 0x00u, 0x00u}));

    bs3 = xbitset_t<32>::build_from(0x12345678u);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x78u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x5678u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x12345678u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x12345678u);
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::little), (xbytes_t{0x78u, 0x56u, 0x34u, 0x12u}));
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::big), (xbytes_t{0x12u, 0x34u, 0x56u, 0x78u}));

    bs3 = xbitset_t<32>::build_from(0x123456789abcdef0u);
    ASSERT_EQ(bs3.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0xdef0u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x9abcdef0u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x9abcdef0u);
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::little), (xbytes_t{0xf0u, 0xdeu, 0xbcu, 0x9au}));
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::big), (xbytes_t{0x9au, 0xbcu, 0xdeu, 0xf0u}));

    auto bs4 = xbitset_t<64>::build_from(0x00u);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs4.to<xbytes_t>(xendian_t::little), (xbytes_t{0, 0, 0, 0, 0, 0, 0, 0}));
    ASSERT_EQ(bs4.to<xbytes_t>(xendian_t::big), (xbytes_t{0, 0, 0, 0, 0, 0, 0, 0}));

    bs4 = xbitset_t<64>::build_from(0x123456789abcdef0u);
    ASSERT_EQ(bs4.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0xdef0u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x9abcdef0u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x123456789abcdef0u);
    ASSERT_EQ(bs4.to<xbytes_t>(xendian_t::little), (xbytes_t{0xf0u, 0xdeu, 0xbcu, 0x9au, 0x78u, 0x56u, 0x34u, 0x12u}));
    ASSERT_EQ(bs4.to<xbytes_t>(xendian_t::big), (xbytes_t{0x12u, 0x34u, 0x56u, 0x78u, 0x9au, 0xbcu, 0xdeu, 0xf0u}));

    auto bs5 = xbitset_t<128>::build_from(0x00u);
    ASSERT_EQ(bs5.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs5.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs5.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs5.to<uint64_t>(), 0x00u);
    {
        xbytes_t bytes;
        bytes.resize(128 / 8);

        ASSERT_EQ(bs5.to<xbytes_t>(xendian_t::little), bytes);
        ASSERT_EQ(bs5.to<xbytes_t>(xendian_t::big), bytes);
    }

    bs5 = xbitset_t<128>::build_from(0x123456789abcdef0u);
    ASSERT_EQ(bs5.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs5.to<uint16_t>(), 0xdef0u);
    ASSERT_EQ(bs5.to<uint32_t>(), 0x9abcdef0u);
    ASSERT_EQ(bs5.to<uint64_t>(), 0x123456789abcdef0u);
    for (size_t i = 64; i < 128; ++i) {
        ASSERT_FALSE(bs5[i]);
    }
    {
        auto bytes = bs5.to<xbytes_t>(xendian_t::little);
        ASSERT_EQ(bytes[0], 0xf0);
        ASSERT_EQ(bytes[1], 0xde);
        ASSERT_EQ(bytes[2], 0xbc);
        ASSERT_EQ(bytes[3], 0x9a);
        ASSERT_EQ(bytes[4], 0x78);
        ASSERT_EQ(bytes[5], 0x56);
        ASSERT_EQ(bytes[6], 0x34);
        ASSERT_EQ(bytes[7], 0x12);
        for (size_t i = 8; i < 128 / 8; ++i) {
            ASSERT_EQ(bytes[i], 0x00);
        }

        bytes = bs5.to<xbytes_t>(xendian_t::big);
        for (size_t i = 0; i < 128 / 8 - 8; ++i) {
            ASSERT_EQ(bytes[i], 0x00);
        }

        ASSERT_EQ(bytes[15], 0xf0);
        ASSERT_EQ(bytes[14], 0xde);
        ASSERT_EQ(bytes[13], 0xbc);
        ASSERT_EQ(bytes[12], 0x9a);
        ASSERT_EQ(bytes[11], 0x78);
        ASSERT_EQ(bytes[10], 0x56);
        ASSERT_EQ(bytes[9], 0x34);
        ASSERT_EQ(bytes[8], 0x12);
    }
}

TEST(xbitset_t, build_from_binary_string) {
    std::error_code ec;
    auto bs1 = xbitset_t<8>::build_from_bin("00000000", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0}));

    bs1 = xbitset_t<8>::build_from_bin("00000000", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0}));

    bs1 = xbitset_t<8>::build_from_bin("0b00000000", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0}));

    bs1 = xbitset_t<8>::build_from_bin("0b00000000", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0}));

    bs1 = xbitset_t<8>::build_from_bin("0B00000000", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0}));

    bs1 = xbitset_t<8>::build_from_bin("0B00000000", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0}));

    bs1 = xbitset_t<8>::build_from_bin("11111111", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0xff}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0xff}));

    bs1 = xbitset_t<8>::build_from_bin("11111111", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0xff}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0xff}));

    bs1 = xbitset_t<8>::build_from_bin("0b11111111", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0xff}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0xff}));

    bs1 = xbitset_t<8>::build_from_bin("0b11111111", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0xff}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0xff}));

    bs1 = xbitset_t<8>::build_from_bin("0B11111111", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0xff}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0xff}));

    bs1 = xbitset_t<8>::build_from_bin("0B11111111", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xffu);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xffu);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0xff}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0xff}));

    bs1 = xbitset_t<8>::build_from_bin("00010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x12}));

    bs1 = xbitset_t<8>::build_from_bin("00010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x48u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x48}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x48}));

    bs1 = xbitset_t<8>::build_from_bin("0b00010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x12}));

    bs1 = xbitset_t<8>::build_from_bin("0b00010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x48u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x48}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x48}));

    bs1 = xbitset_t<8>::build_from_bin("0B00010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x12}));

    bs1 = xbitset_t<8>::build_from_bin("0B00010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x48u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x48u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x48}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x48}));

    bs1 = xbitset_t<8>::build_from_bin("0010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x02u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x02}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x02}));

    bs1 = xbitset_t<8>::build_from_bin("0010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x04u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x04u}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x04u}));

    bs1 = xbitset_t<8>::build_from_bin("0b0010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x02u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x02}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x02}));

    bs1 = xbitset_t<8>::build_from_bin("0b0010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x04u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x04u}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x04u}));

    bs1 = xbitset_t<8>::build_from_bin("0B0010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x02u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x02u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x02}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x02}));

    bs1 = xbitset_t<8>::build_from_bin("0B0010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x04u);
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::little), (xbytes_t{0x04u}));
    ASSERT_EQ(bs1.to<xbytes_t>(xendian_t::big), (xbytes_t{0x04u}));

    auto bs2 = xbitset_t<16>::build_from_bin("0010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x02u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x02, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x02}));

    bs2 = xbitset_t<16>::build_from_bin("0010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x04u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x04u, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x04u}));

    bs2 = xbitset_t<16>::build_from_bin("0b0010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x02u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x02, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x02}));

    bs2 = xbitset_t<16>::build_from_bin("0b0010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x04u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x04u, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x04u}));

    bs2 = xbitset_t<16>::build_from_bin("0B0010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x02u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x02u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x02, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x02}));

    bs2 = xbitset_t<16>::build_from_bin("0B0010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x04u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x04u, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x04u}));

    bs2 = xbitset_t<16>::build_from_bin("00010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x12}));

    bs2 = xbitset_t<16>::build_from_bin("00010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x48u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x48u, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x48u}));

    bs2 = xbitset_t<16>::build_from_bin("0b00010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x12}));

    bs2 = xbitset_t<16>::build_from_bin("0b00010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x48u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x48u, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x48u}));

    bs2 = xbitset_t<16>::build_from_bin("0B00010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x12}));

    bs2 = xbitset_t<16>::build_from_bin("0B00010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x48u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x48u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x48u, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x48u}));

    bs2 = xbitset_t<16>::build_from_bin("0000000000010010", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x12u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x12u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x12, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x12}));

    bs2 = xbitset_t<16>::build_from_bin("0000000000010010", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x4800u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x4800u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x4800u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x00, 0x48}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x48, 0x00}));

    bs2 = xbitset_t<16>::build_from_bin("0b1000000000000001", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x01, 0x80}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x80, 0x01}));

    bs2 = xbitset_t<16>::build_from_bin("0b1000000000000001", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x01, 0x80}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x80, 0x01}));

    bs2 = xbitset_t<16>::build_from_bin("0B10001000000000000001", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x8001u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x01, 0x80}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x80, 0x01}));

    bs2 = xbitset_t<16>::build_from_bin("0B10001000000000000001", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x11u);
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::little), (xbytes_t{0x11, 0x00}));
    ASSERT_EQ(bs2.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x11}));

    auto bs3 = xbitset_t<32>::build_from_bin("0B10001000000000000001", MSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x8001u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x088001u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x088001u);
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::little), (xbytes_t{0x01, 0x80, 0x08, 0x00}));
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x08, 0x80, 0x01}));

    bs3 = xbitset_t<32>::build_from_bin("0B10001000000000000001", LSB0, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x80011u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x80011u);
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::little), (xbytes_t{0x11, 0x00, 0x08, 0x00}));
    ASSERT_EQ(bs3.to<xbytes_t>(xendian_t::big), (xbytes_t{0x00, 0x08, 0x00, 0x11}));

    auto bs4 = xbitset_t<128>::build_from_bin(
        "11111111"
        "11111111"
        "11111111"
        "11111111"
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "10101010"
        "10101010"
        "10101010"
        "10101010"
        "01010101"
        "01010101"
        "01010101"
        "01010101",
        MSB0,
        ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x55u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x5555u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x55555555u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0xAAAAAAAA55555555u);
    for (size_t i = 64; i < 64 + 32; ++i) {
        ASSERT_FALSE(bs4[i]);
    }
    for (size_t i = 64 + 32; i < 128; ++i) {
        ASSERT_TRUE(bs4[i]);
    }
    {
        auto bytes = bs4.to<xbytes_t>(xendian_t::little);
        ASSERT_EQ(bytes.size(), 16);
        for (size_t i = 0; i < 4; ++i) {
            ASSERT_EQ(bytes[i], 0x55);
        }
        for (size_t i = 4; i < 8; ++i) {
            ASSERT_EQ(bytes[i], 0xAA);
        }
        for (size_t i = 8; i < 12; ++i) {
            ASSERT_EQ(bytes[i], 0x00);
        }
        for (size_t i = 12; i < 16; ++i) {
            ASSERT_EQ(bytes[i], 0xFF);
        }

        bytes = bs4.to<xbytes_t>(xendian_t::big);
        ASSERT_EQ(bytes.size(), 16);
        for (size_t i = 0; i < 4; ++i) {
            ASSERT_EQ(bytes[i], 0xFF);
        }
        for (size_t i = 4; i < 8; ++i) {
            ASSERT_EQ(bytes[i], 0x00);
        }
        for (size_t i = 8; i < 12; ++i) {
            ASSERT_EQ(bytes[i], 0xAA);
        }
        for (size_t i = 12; i < 16; ++i) {
            ASSERT_EQ(bytes[i], 0x55);
        }
    }

    bs4 = xbitset_t<128>::build_from_bin(
        "11111111"
        "11111111"
        "11111111"
        "11111111"
        "00000000"
        "00000000"
        "00000000"
        "00000000"
        "10101010"
        "10101010"
        "10101010"
        "10101010"
        "01010101"
        "01010101"
        "01010101"
        "01010101",
        LSB0,
        ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs4.to<uint8_t>(), 0xFFu);
    ASSERT_EQ(bs4.to<uint16_t>(), 0xFFFFu);
    ASSERT_EQ(bs4.to<uint32_t>(), 0xFFFFFFFFu);
    ASSERT_EQ(bs4.to<uint64_t>(), 0xFFFFFFFFu);
    for (size_t i = 64; i < 64 + 32; ++i) {
        ASSERT_EQ(!static_cast<bool>(i % 2), bs4.test(i));
    }
    for (size_t i = 64 + 32; i < 128; ++i) {
        ASSERT_EQ(static_cast<bool>(i % 2), bs4.test(i));
    }
    {
        auto bytes = bs4.to<xbytes_t>(xendian_t::little);
        ASSERT_EQ(bytes.size(), 16);
        for (size_t i = 0; i < 4; ++i) {
            ASSERT_EQ(bytes[i], 0xFF);
        }
        for (size_t i = 4; i < 8; ++i) {
            ASSERT_EQ(bytes[i], 0x00);
        }
        for (size_t i = 8; i < 12; ++i) {
            ASSERT_EQ(bytes[i], 0x55);
        }
        for (size_t i = 12; i < 16; ++i) {
            ASSERT_EQ(bytes[i], 0xAA);
        }

        bytes = bs4.to<xbytes_t>(xendian_t::big);
        ASSERT_EQ(bytes.size(), 16);
        for (size_t i = 0; i < 4; ++i) {
            ASSERT_EQ(bytes[i], 0xAA);
        }
        for (size_t i = 4; i < 8; ++i) {
            ASSERT_EQ(bytes[i], 0x55);
        }
        for (size_t i = 8; i < 12; ++i) {
            ASSERT_EQ(bytes[i], 0x00);
        }
        for (size_t i = 12; i < 16; ++i) {
            ASSERT_EQ(bytes[i], 0xFF);
        }
    }

    auto bs1_failed = xbitset_t<8>::build_from_bin("0B20001000000000000001", MSB0, ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(bs1_failed.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1_failed.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1_failed.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1_failed.to<uint64_t>(), 0x00u);

    ec.clear();
    bs1_failed = xbitset_t<8>::build_from_bin("0B20001000000000000001", LSB0, ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(bs1_failed.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1_failed.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1_failed.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1_failed.to<uint64_t>(), 0x00u);
}

TEST(xbitset_t, build_from_hex_string) {
    std::error_code ec;

    auto bs1 = xbitset_t<8>::build_from_hex("", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0x0", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0x0", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0X00", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0X00", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("01", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x01u);

    bs1 = xbitset_t<8>::build_from_hex("01", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x01u);

    bs1 = xbitset_t<8>::build_from_hex("0x10", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0x10", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0X20", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x20u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x20u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x20u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x20u);

    bs1 = xbitset_t<8>::build_from_hex("0X20", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x20u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x20u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x20u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x20u);

    bs1 = xbitset_t<8>::build_from_hex("00000000", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("00000000", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0x00000000", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0x00000000", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0X00000000", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0X00000000", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("11111111", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x11u);

    bs1 = xbitset_t<8>::build_from_hex("11111111", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x11u);

    bs1 = xbitset_t<8>::build_from_hex("0x11111111", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x11u);

    bs1 = xbitset_t<8>::build_from_hex("0x11111111", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x11u);

    bs1 = xbitset_t<8>::build_from_hex("0x11111111", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x11u);

    bs1 = xbitset_t<8>::build_from_hex("0x11111111", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x11u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x11u);

    bs1 = xbitset_t<8>::build_from_hex("01020304", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x04u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x04u);

    bs1 = xbitset_t<8>::build_from_hex("01020304", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x01u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x01u);

    bs1 = xbitset_t<8>::build_from_hex("0x00010010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0x00010010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0X00010010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0X00010010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0x0010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0x0010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from_hex("0X0010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x10u);

    bs1 = xbitset_t<8>::build_from_hex("0X0010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    auto bs2 = xbitset_t<16>::build_from_hex("0010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x10u);

    bs2 = xbitset_t<16>::build_from_hex("0010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x1000u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x1000u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x1000u);

    bs2 = xbitset_t<16>::build_from_hex("0x0010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x10u);

    bs2 = xbitset_t<16>::build_from_hex("0x0010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x1000u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x1000u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x1000u);

    bs2 = xbitset_t<16>::build_from_hex("0X0010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x10u);

    bs2 = xbitset_t<16>::build_from_hex("0X0010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x1000u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x1000u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x1000u);

    bs2 = xbitset_t<16>::build_from_hex("00010010", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x10u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x10u);

    bs2 = xbitset_t<16>::build_from_hex("00010010", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0100u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0100u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0100u);

    bs2 = xbitset_t<16>::build_from_hex("0x102030405", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x05u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0405u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0405u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0405u);

    bs2 = xbitset_t<16>::build_from_hex("0x102030405", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0201u);

    bs2 = xbitset_t<16>::build_from_hex("0X01020304", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0304u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0304u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0304u);

    bs2 = xbitset_t<16>::build_from_hex("0X01020304", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0201u);

    bs2 = xbitset_t<16>::build_from_hex("0102030405060708", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0708u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0708u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0708u);

    bs2 = xbitset_t<16>::build_from_hex("0102030405060708", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0201u);

    bs2 = xbitset_t<16>::build_from_hex("0xf0e0d0c0b0a09081", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x81u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x9081u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x9081u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x9081u);

    bs2 = xbitset_t<16>::build_from_hex("0xf0e0d0c0b0a09081", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0xe0f0u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0xe0f0u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0xe0f0u);

    bs2 = xbitset_t<16>::build_from_hex("0X0f0e0d0c0b0a09080701", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0701u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0701u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0701u);

    bs2 = xbitset_t<16>::build_from_hex("0X0f0e0d0c0b0a09080701", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x0fu);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0e0fu);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0e0fu);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0e0fu);

    auto bs3 = xbitset_t<32>::build_from_hex("0X1020304050607080900A", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x0au);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x900au);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x7080900Au);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x7080900Au);

    bs3 = xbitset_t<32>::build_from_hex("0X1020304050607080900A", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x2010u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x40302010u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x40302010u);

    auto bs4 = xbitset_t<64>::build_from_hex("0X102030405060708090a0b0c0d0e0f0", xendian_t::big, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs4.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0xe0f0u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0xc0d0e0f0u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x8090a0b0c0d0e0f0u);

    bs4 = xbitset_t<64>::build_from_hex("0X102030405060708090a0b0c0d0e0f0", xendian_t::little, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x10u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x2010u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x40302010u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x8070605040302010u);

    auto bs5 = xbitset_t<8>::build_from_hex("0B20001000000000000001", xendian_t::big, ec); // note: 0B or 0b are valid hex data.
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs5.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs5.to<uint16_t>(), 0x01u);
    ASSERT_EQ(bs5.to<uint32_t>(), 0x01u);
    ASSERT_EQ(bs5.to<uint64_t>(), 0x01u);

    bs5 = xbitset_t<8>::build_from_hex("0B20001000000000000001", xendian_t::little, ec);  // note: 0B or 0b are valid hex data.
    ASSERT_TRUE(!ec);
    ASSERT_EQ(bs5.to<uint8_t>(), 0x0bu);
    ASSERT_EQ(bs5.to<uint16_t>(), 0x0bu);
    ASSERT_EQ(bs5.to<uint32_t>(), 0x0bu);
    ASSERT_EQ(bs5.to<uint64_t>(), 0x0bu);

    ec.clear();
    auto bs2_failed = xbitset_t<16>::build_from_hex("0xabcdefgh", xendian_t::big, ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(bs2_failed.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2_failed.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs2_failed.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs2_failed.to<uint64_t>(), 0x00u);

    ec.clear();
    bs2_failed = xbitset_t<16>::build_from_hex("0xabcdefgh", xendian_t::little, ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(bs2_failed.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs2_failed.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs2_failed.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs2_failed.to<uint64_t>(), 0x00u);
}

TEST(xbitset_t, build_from_bytes_le) {
    auto bs1 = xbitset_t<8>::build_from(xbytes_t{0x00, 0x01, 0x02, 0x03}, xendian_t::little);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x00u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x00u);

    bs1 = xbitset_t<8>::build_from(xbytes_t{0xf0}, xendian_t::little);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xf0u);

    auto bs2 = xbitset_t<16>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04}, xendian_t::little);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0201u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0201u);

    bs2 = xbitset_t<16>::build_from(xbytes_t{0x08, 0x07, 0x06}, xendian_t::little);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0708u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0708u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0708u);

    bs2 = xbitset_t<16>::build_from(xbytes_t{0x08}, xendian_t::little);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x08u);

    auto bs3 = xbitset_t<32>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04}, xendian_t::little);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x04030201u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x04030201u);

    bs3 = xbitset_t<32>::build_from(xbytes_t{0x08, 0x07}, xendian_t::little);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x08u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x0708u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x0708u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x0708u);

    auto bs4 = xbitset_t<512>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04}, xendian_t::little);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x04030201u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x04030201u);

    bs4 = xbitset_t<512>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}, xendian_t::little);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x01u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x0201u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x04030201u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x0807060504030201u);
}

TEST(xbitset_t, build_from_bytes_be) {
    auto bs1 = xbitset_t<8>::build_from(xbytes_t{0x00, 0x01, 0x02, 0x03}, xendian_t::big);
    ASSERT_EQ(bs1.to<uint8_t>(), 0x03u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0x03u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0x03u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0x03u);

    bs1 = xbitset_t<8>::build_from(xbytes_t{0xf0}, xendian_t::big);
    ASSERT_EQ(bs1.to<uint8_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint16_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint32_t>(), 0xf0u);
    ASSERT_EQ(bs1.to<uint64_t>(), 0xf0u);

    auto bs2 = xbitset_t<16>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04}, xendian_t::big);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0304u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0304u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0304u);

    bs2 = xbitset_t<16>::build_from(xbytes_t{0x08, 0x07, 0x06}, xendian_t::big);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x06u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x0706u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x0706u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x0706u);

    bs2 = xbitset_t<16>::build_from(xbytes_t{0x08}, xendian_t::big);
    ASSERT_EQ(bs2.to<uint8_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint16_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint32_t>(), 0x08u);
    ASSERT_EQ(bs2.to<uint64_t>(), 0x08u);

    auto bs3 = xbitset_t<32>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04}, xendian_t::big);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x0304u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x01020304u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x01020304u);

    bs3 = xbitset_t<32>::build_from(xbytes_t{0x08, 0x07}, xendian_t::big);
    ASSERT_EQ(bs3.to<uint8_t>(), 0x07u);
    ASSERT_EQ(bs3.to<uint16_t>(), 0x0807u);
    ASSERT_EQ(bs3.to<uint32_t>(), 0x0807u);
    ASSERT_EQ(bs3.to<uint64_t>(), 0x0807u);

    auto bs4 = xbitset_t<512>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04}, xendian_t::big);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x04u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x0304u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x01020304u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x01020304u);

    bs4 = xbitset_t<512>::build_from(xbytes_t{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}, xendian_t::big);
    ASSERT_EQ(bs4.to<uint8_t>(), 0x08u);
    ASSERT_EQ(bs4.to<uint16_t>(), 0x0708u);
    ASSERT_EQ(bs4.to<uint32_t>(), 0x05060708u);
    ASSERT_EQ(bs4.to<uint64_t>(), 0x0102030405060708u);
}

TEST(xbitset_t, to_bytes) {
#define TEST_CASE(BITS, INPUT_BYTES, INPUT_ENDIAN, OUTPUT_ENDIAN)                                                                                                                  \
    do {                                                                                                                                                                           \
        {                                                                                                                                                                          \
            auto const & input_bytes = INPUT_BYTES;                                                                                                                                \
            auto const bs = xbitset_t<BITS>::build_from(input_bytes, INPUT_ENDIAN);                                                                                                \
            auto output_bytes = bs.to<xbytes_t>(OUTPUT_ENDIAN);                                                                                                                    \
            auto const bs1 = xbitset_t<BITS>::build_from(output_bytes, OUTPUT_ENDIAN);                                                                                             \
            ASSERT_EQ(bs, bs1);                                                                                                                                                    \
            if ((BITS) != 8 && !std::all_of(std::begin(input_bytes), std::end(input_bytes), [&input_bytes](xbyte_t const b) { return b == input_bytes.front(); }) &&               \
                (INPUT_ENDIAN) != (OUTPUT_ENDIAN)) {                                                                                                                               \
                auto const bs2 = xbitset_t<BITS>::build_from(output_bytes, INPUT_ENDIAN);                                                                                          \
                ASSERT_NE(bs, bs2);                                                                                                                                                \
            }                                                                                                                                                                      \
        }                                                                                                                                                                          \
    } while (false)

    TEST_CASE(8, xbytes_t{0x00}, xendian_t::big, xendian_t::big);
    TEST_CASE(8, xbytes_t{0x00}, xendian_t::big, xendian_t::little);
    TEST_CASE(8, xbytes_t{0x00}, xendian_t::little, xendian_t::big);
    TEST_CASE(8, xbytes_t{0x00}, xendian_t::little, xendian_t::little);

    TEST_CASE(8, xbytes_t{0x01}, xendian_t::big, xendian_t::big);
    TEST_CASE(8, xbytes_t{0x01}, xendian_t::big, xendian_t::little);
    TEST_CASE(8, xbytes_t{0x01}, xendian_t::little, xendian_t::big);
    TEST_CASE(8, xbytes_t{0x01}, xendian_t::little, xendian_t::little);

    TEST_CASE(8, xbytes_t{0x80}, xendian_t::big, xendian_t::big);
    TEST_CASE(8, xbytes_t{0x80}, xendian_t::big, xendian_t::little);
    TEST_CASE(8, xbytes_t{0x80}, xendian_t::little, xendian_t::big);
    TEST_CASE(8, xbytes_t{0x80}, xendian_t::little, xendian_t::little);

    TEST_CASE(8, xbytes_t{0xff}, xendian_t::big, xendian_t::big);
    TEST_CASE(8, xbytes_t{0xff}, xendian_t::big, xendian_t::little);
    TEST_CASE(8, xbytes_t{0xff}, xendian_t::little, xendian_t::big);
    TEST_CASE(8, xbytes_t{0xff}, xendian_t::little, xendian_t::little);

    TEST_CASE(8, (xbytes_t{0x01, 0x02}), xendian_t::big, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x01, 0x02}), xendian_t::big, xendian_t::little);
    TEST_CASE(8, (xbytes_t{0x01, 0x02}), xendian_t::little, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x01, 0x02}), xendian_t::little, xendian_t::little);

    TEST_CASE(8, (xbytes_t{0x80, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x80, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(8, (xbytes_t{0x80, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x80, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(8, (xbytes_t{0xff, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0xff, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(8, (xbytes_t{0xff, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0xff, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(8, (xbytes_t{0x00, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x00, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(8, (xbytes_t{0x00, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x00, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(8, (xbytes_t{0x01, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x01, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(8, (xbytes_t{0x01, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(8, (xbytes_t{0x01, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, xbytes_t{0x00}, xendian_t::big, xendian_t::big);
    TEST_CASE(16, xbytes_t{0x00}, xendian_t::big, xendian_t::little);
    TEST_CASE(16, xbytes_t{0x00}, xendian_t::little, xendian_t::big);
    TEST_CASE(16, xbytes_t{0x00}, xendian_t::little, xendian_t::little);

    TEST_CASE(16, xbytes_t{0x01}, xendian_t::big, xendian_t::big);
    TEST_CASE(16, xbytes_t{0x01}, xendian_t::big, xendian_t::little);
    TEST_CASE(16, xbytes_t{0x01}, xendian_t::little, xendian_t::big);
    TEST_CASE(16, xbytes_t{0x01}, xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x01, 0x02}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x01, 0x02}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x01, 0x02}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x01, 0x02}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x00, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x00, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x01, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x01, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x01, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x01, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x00, 0x01}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0x01}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x00, 0x01}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0x01}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x80, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x80, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x80, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x80, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x00, 0x80}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0x80}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x00, 0x80}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0x80}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0xff, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0xff, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0xff, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0xff, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x00, 0xff}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0xff}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x00, 0xff}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x00, 0xff}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0xff, 0xff}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0xff, 0xff}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0xff, 0xff}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0xff, 0xff}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x01, 0x02, 0x03, 0x04}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x01, 0x02, 0x03, 0x04}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x01, 0x02, 0x03, 0x04}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x01, 0x02, 0x03, 0x04}), xendian_t::little, xendian_t::little);

    TEST_CASE(16, (xbytes_t{0x04, 0x03, 0x02, 0x01}), xendian_t::big, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x04, 0x03, 0x02, 0x01}), xendian_t::big, xendian_t::little);
    TEST_CASE(16, (xbytes_t{0x04, 0x03, 0x02, 0x01}), xendian_t::little, xendian_t::big);
    TEST_CASE(16, (xbytes_t{0x04, 0x03, 0x02, 0x01}), xendian_t::little, xendian_t::little);

    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(32, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(32, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::big, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::big, xendian_t::little);
    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::little, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::little, xendian_t::little);

    TEST_CASE(32, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::big, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::big, xendian_t::little);
    TEST_CASE(32, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::little, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::little, xendian_t::little);

    TEST_CASE(32, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(32, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(32, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(64, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(64, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x01, 0x00, 0x00, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::big, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::big, xendian_t::little);
    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::little, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x00, 0x00, 0x01}), xendian_t::little, xendian_t::little);

    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::big, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::big, xendian_t::little);
    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::little, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03}), xendian_t::little, xendian_t::little);

    TEST_CASE(64, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::big, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::big, xendian_t::little);
    TEST_CASE(64, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::little, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x03, 0x02, 0x01, 0x00}), xendian_t::little, xendian_t::little);

    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}), xendian_t::big, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}), xendian_t::big, xendian_t::little);
    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}), xendian_t::little, xendian_t::big);
    TEST_CASE(64, (xbytes_t{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}), xendian_t::little, xendian_t::little);

#undef TEST_CASE
}

NS_END2
