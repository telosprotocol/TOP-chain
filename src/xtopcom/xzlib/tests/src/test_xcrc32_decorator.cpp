#include "xbasic/xbyte_buffer.h"
#include "xcodec/xbuffer_codec.hpp"
#include "xcodec/xcodec_error.h"
#include "xzlib/xdecorators/xcrc32_decorator.h"

#include <gtest/gtest.h>

#include <array>
#include <random>

static constexpr std::size_t char_size{ 62 };
static constexpr std::array<top::xbyte_t, char_size> bytes
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

TEST(xzlib, crc32_decorator)
{
    std::random_device rd;
    std::mt19937_64 random_engine{ rd() };
    std::uniform_int_distribution<std::size_t> distribution{ 0, char_size };

    auto const size = distribution(random_engine);
    top::xbyte_buffer_t in(size);
    for (auto i = 0u; i < size; ++i)
    {
        in[i] = bytes[distribution(random_engine)];
    }

    auto const encoded_result = top::codec::xbuffer_codec_t<top::zlib::decorators::xcrc32_decorator_t>::encode(in);
    auto const decoded_result = top::codec::xbuffer_codec_t<top::zlib::decorators::xcrc32_decorator_t>::decode(encoded_result);

    EXPECT_EQ(in, decoded_result);
}

TEST(xzlib, crc32_decorator_decode_empty)
{
    EXPECT_THROW(top::codec::xbuffer_codec_t<top::zlib::decorators::xcrc32_decorator_t>::decode(top::xbyte_buffer_t{}),
                 top::codec::xcodec_error_t);
}

TEST(xzlib, crc32_decorator_decode_small)
{
    EXPECT_THROW(top::codec::xbuffer_codec_t<top::zlib::decorators::xcrc32_decorator_t>::decode(top::xbyte_buffer_t{'a'}),
                 top::codec::xcodec_error_t);
}
