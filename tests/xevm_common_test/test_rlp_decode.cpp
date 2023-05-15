#include <gtest/gtest.h>

#include "xbasic/xbytes.h"
#include "xevm_common/xcrosschain/xeth2.h"
#include "xcommon/rlp.h"

#include <random>
#include <cstring>

#include <endian.h>

NS_BEG3(top, evm_common, tests)

TEST(xinit_input_t, fuzzy) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    {
        eth2::xinit_input_t input;
        ASSERT_FALSE(input.decode_rlp(xbytes_t{}));
    }

    for (auto i = 1u; i < 10000u; ++i) {
        xbytes_t bytes;
        bytes.reserve(i);

        for (auto j = 0u; j < i; ++j) {
            bytes.emplace_back(byte_distrib(gen));
        }

        try {
            eth2::xinit_input_t input;
            auto const ret = input.decode_rlp(bytes);
            ASSERT_FALSE(ret);
        } catch (std::invalid_argument const &) {
        } catch (std::exception const &) {
            ASSERT_FALSE(true);
        } catch (...) {
            ASSERT_FALSE(true);
        }
    }
}

TEST(xlight_client_update_t, fuzzy) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    {
        eth2::xlight_client_update_t update;
        ASSERT_FALSE(update.decode_rlp(xbytes_t{}));
    }

    for (auto i = 1u; i < 10000u; ++i) {
        xbytes_t bytes;
        bytes.reserve(i);

        for (auto j = 0u; j < i; ++j) {
            bytes.emplace_back(byte_distrib(gen));
        }

        try {
            eth2::xlight_client_update_t update;
            auto const ret = update.decode_rlp(bytes);
            ASSERT_FALSE(ret);
        } catch (std::invalid_argument const &) {
        } catch (std::exception const &) {
            ASSERT_FALSE(true);
        } catch (...) {
            ASSERT_FALSE(true);
        }
    }
}

TEST(xeth_header_t, fuzzy) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(4, 100000);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    {
        xeth_header_t header;
        ASSERT_THROW(header.decode_rlp(xbytes_t{}), top::error::xtop_error_t);
    }

    for (auto i = 1u; i < 100000u; ++i) {
        xbytes_t bytes;
        bytes.reserve(i);

        for (auto j = 0u; j < i; ++j) {
            bytes.emplace_back(byte_distrib(gen));
        }

        try {
            xeth_header_t header;
            std::error_code ec;
            header.decode_rlp(bytes, ec);
            ASSERT_FALSE(!ec);
        } catch (std::invalid_argument const &) {
        } catch (std::exception const &) {
            ASSERT_FALSE(true);
        } catch (...) {
            ASSERT_FALSE(true);
        }
    }
}

TEST(rlp, u64_codec) {
    constexpr uint64_t u64 = 0x123456789abcdef0ull;

    auto const bytes = RLP::encode(u64);
    uint64_t const u64_decoded = evm_common::fromBigEndian<uint64_t>(bytes);

    ASSERT_EQ(u64, u64_decoded);

    evm_common::h64 const h64{"0x123456789abcdef0"};
    auto const bytes_2 = RLP::encode(h64.asBytes());
    uint64_t const u64_decoded_2 = evm_common::fromBigEndian<uint64_t>(bytes_2);

    ASSERT_EQ(u64, u64_decoded_2);
}

NS_END3
