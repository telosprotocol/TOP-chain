#if defined _WIN32 && !defined NOMINMAX
# define NOMINMAX
#endif

#include "xcodec/xcodec_error.h"
#include "xzlib/xdecorators/xcrc32_decorator.h"

#include <cstring>
#include <limits>
#include <cstdint>
#include <stdexcept>

#include <zlib.h>

#include <endian.h>

NS_BEG3(top, zlib, decorators)

static constexpr std::size_t crc_length{ 4 };

static
std::uint32_t
calc_crc(xbyte_t const * data, std::size_t const size)
{
    return static_cast<std::uint32_t>(crc32(crc32(0, nullptr, 0),
                                            static_cast<const Bytef*>(data),
                                            static_cast<uInt>(size)));
}

xbyte_buffer_t
xtop_crc32_decorator::encode(xbyte_buffer_t const & in)
{
    auto const length = static_cast<std::uint32_t>(in.size() + crc_length);
    auto buffer = xbyte_buffer_t(length);

    std::size_t idx = 0;
    std::memcpy(&buffer[idx], in.data(), in.size());
    idx += in.size();

    if (std::numeric_limits<uInt>::max() < in.size())
    {
        throw codec::xcodec_error_t{ codec::xcodec_errc_t::encode_input_buffer_too_long, __LINE__, __FILE__ };
    }

    auto crc = calc_crc(in.data(), in.size());
    crc = htole32(crc);
    XSTATIC_ASSERT(sizeof(crc) == crc_length);
    std::memcpy(&buffer[idx], &crc, crc_length);

    return buffer;
}

xbyte_buffer_t
xtop_crc32_decorator::decode(xbyte_buffer_t const & in)
{
    if (in.size() > std::numeric_limits<uInt>::max())
    {
        throw codec::xcodec_error_t{ codec::xcodec_errc_t::decode_input_buffer_too_long, __LINE__, __FILE__ };
    }

    if (in.empty())
    {
        throw codec::xcodec_error_t{ codec::xcodec_errc_t::decode_input_buffer_not_enough, __LINE__, __FILE__ };
    }

    if (in.size() <= crc_length)
    {
        throw codec::xcodec_error_t{ codec::xcodec_errc_t::decode_input_buffer_not_enough, __LINE__, __FILE__ };
    }

    auto const size = static_cast<uInt>(in.size() - crc_length);
    auto const crc = calc_crc(in.data(), size);

    std::uint32_t expected_crc = 0;
    std::memcpy(&expected_crc, &in[size], crc_length);
    expected_crc = le32toh(expected_crc);

    if (crc != expected_crc)
    {
        throw codec::xcodec_error_t{ codec::xcodec_errc_t::decode_wrong_checksum, __LINE__, __FILE__ };
    }

    return { in.begin(), std::next(in.begin(), size) };
}

NS_END3
