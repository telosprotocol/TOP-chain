// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcodec/xcodec_error.h"

#include <cstring>
#include <type_traits>

#ifdef __LINUX__
#include <endian.h>  // TODO(bluecl): linux only?
#elif defined(__APPLE__)
#include <machine/endian.h>

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif

NS_BEG3(top, codec, decorators)

template <typename SizeLengthT>
struct xtop_prepend_size_decorator final
{
    XSTATIC_ASSERT(std::is_integral<SizeLengthT>::value);

    xtop_prepend_size_decorator()                                                = delete;
    xtop_prepend_size_decorator(xtop_prepend_size_decorator const &)             = delete;
    xtop_prepend_size_decorator & operator=(xtop_prepend_size_decorator const &) = delete;
    xtop_prepend_size_decorator(xtop_prepend_size_decorator &&)                  = delete;
    xtop_prepend_size_decorator & operator=(xtop_prepend_size_decorator &&)      = delete;
    ~xtop_prepend_size_decorator()                                               = delete;

    static
    xbyte_buffer_t
    encode(xbyte_buffer_t const & in, std::size_t const header_size_adjustment = 0) {
        xbyte_buffer_t buffer(in.size() + size_field_length());
        auto * buffer_addr = buffer.data();

        auto const buffer_size = convert_to_little_ending<SizeLengthT>(in.size() + size_field_length() + header_size_adjustment);
        std::memcpy(buffer_addr, &buffer_size, size_field_length());
        std::memcpy(buffer_addr + size_field_length(), in.data(), in.size());

        return buffer;
    }

    static
    xbyte_buffer_t
    decode(xbyte_buffer_t const & in,
           std::size_t const header_size_adjustment = 0,
           std::size_t * decoded_size = nullptr) {
        if (in.size() < size_field_length()) {
            throw xcodec_error_t{ xcodec_errc_t::decode_input_buffer_not_enough, __LINE__, __FILE__ };
        }

        if (in.size() - size_field_length() < header_size_adjustment) {
            throw xcodec_error_t{ xcodec_errc_t::decode_input_buffer_not_enough, __LINE__, __FILE__ };
        }

        SizeLengthT little_ending_size;
        std::memcpy(&little_ending_size, in.data(), size_field_length());
        auto buffer_size = convert_from_little_ending(little_ending_size) - header_size_adjustment;

        if (buffer_size > in.size()) {
            throw xcodec_error_t{ xcodec_errc_t::decode_input_buffer_not_enough, __LINE__, __FILE__ };
        }

        auto effective_buffer_size = buffer_size - size_field_length();

        xbyte_buffer_t buffer(effective_buffer_size);
        std::memcpy(buffer.data(), in.data() + size_field_length(), effective_buffer_size);
        if (decoded_size) {
            *decoded_size = buffer_size;
        }

        return buffer;
    }

    static
    constexpr
    std::size_t
    size_field_length() noexcept {
        return sizeof(SizeLengthT);
    }

private:
    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 1, T>::type
    convert_to_little_ending(std::size_t const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return static_cast<T>(static_cast<T>(size));
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 2, T>::type
    convert_to_little_ending(std::size_t const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return htole16(static_cast<T>(size));
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 4, T>::type
    convert_to_little_ending(std::size_t const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return htole32(static_cast<T>(size));
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 8, T>::type
    convert_to_little_ending(std::size_t const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return htole64(static_cast<T>(size));
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 1, std::size_t>::type
    convert_from_little_ending(T const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return static_cast<std::size_t>(size);
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 2, std::size_t>::type
    convert_from_little_ending(T const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return static_cast<std::size_t>(le16toh(size));
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 4, std::size_t>::type
    convert_from_little_ending(T const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return static_cast<std::size_t>(le32toh(size));
    }

    template <typename T>
    static
    typename std::enable_if<sizeof(T) == 8, std::size_t>::type
    convert_from_little_ending(T const size) {
        XSTATIC_ASSERT(std::is_integral<T>::value);
        return static_cast<std::size_t>(le64toh(size));
    }
};

template <typename SizeLengthT>
using xprepend_size_decorator_t = xtop_prepend_size_decorator<SizeLengthT>;

NS_END3
