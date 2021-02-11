// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte.h"
#include "xbasic/xhash.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

NS_BEG1(top)

/**
 * @brief Big-Endian large unsigned integer.
 * @tparam Bits Integer bits.
 */
template <std::size_t Bits>
class xtop_uint final {
    XSTATIC_ASSERT(Bits % 8 == 0);
    XSTATIC_ASSERT(Bits > 64);

    template <std::size_t OtherBits>
    friend class xtop_uint;

public:
    static constexpr std::size_t bits_size = Bits;
    static constexpr std::size_t bytes_size = Bits / 8;
    // static constexpr std::size_t uint64_count = (bytes_size + (sizeof(store_t) >> 1)) / sizeof(store_t);

private:
    std::array<xbyte_t, bytes_size> m_data;

public:
    xtop_uint(xtop_uint const &) = default;
    xtop_uint & operator=(xtop_uint const &) = default;
    xtop_uint(xtop_uint &&) = default;
    xtop_uint & operator=(xtop_uint &&) = default;
    ~xtop_uint() = default;

    xtop_uint() {
        m_data.fill(0);
    }

    template <typename T>
    explicit xtop_uint(T value) noexcept : xtop_uint{} {
        XSTATIC_ASSERT(std::is_integral<T>::value);

        if (value < 0) {
            m_data.fill(0xff);
        }

        for (auto i = 0u; i < sizeof(T); ++i, value >>= 8) {
            auto v = static_cast<xbyte_t>(value & 0xFF);
            m_data[bytes_size - 1 - i] = v;
        }
    }

    template <std::size_t Bytes>
    xtop_uint(xhash_t<Bytes> const & hash) : xtop_uint{} {
        auto constexpr uint_bytes = bytes_size;
        auto constexpr hash_bytes = Bytes;
        auto const min_bytes = std::min(uint_bytes, hash_bytes);

        for (auto i = 0u; i < min_bytes; ++i) {
            m_data[uint_bytes - 1 - i] = hash.as_array()[hash_bytes - 1 - i];
        }
    }

    operator bool() const noexcept {
        for (auto const & uch : m_data) {
            if (uch != 0) {
                return true;
            }
        }

        return false;
    }

    bool operator!() const noexcept {
        return !static_cast<bool>(*this);
    }

    bool operator==(xtop_uint const & other) const noexcept {
        return m_data == other.m_data;
    }

    bool operator<(xtop_uint const & other) const noexcept {
        for (auto i = 0u; i < bytes_size; ++i) {
            if (m_data[i] < other.m_data[i]) {
                return true;
            }

            if (m_data[i] > other.m_data[i]) {
                return false;
            }
        }

        return false;
    }

    template <std::size_t OtherBits, typename = typename std::enable_if<Bits != OtherBits>::type>
    bool operator==(xtop_uint<OtherBits> const & other) const noexcept {
        auto constexpr bytes_count_this = bytes_size;
        auto constexpr bytes_count_other = xtop_uint<OtherBits>::bytes_size;

        auto min_bytes_count = std::min(bytes_count_this, bytes_count_other);
        for (auto i = 0; i < min_bytes_count; ++i) {
            if (m_data[bytes_count_this - 1 - i] != other.m_data[bytes_count_other - 1 - i]) {
                return false;
            }
        }

        if (bytes_count_this > bytes_count_other) {
            for (auto i = min_bytes_count; i < bytes_count_this; ++i) {
                if (m_data[bytes_count_this - 1 - i] != 0) {
                    return false;
                }
            }
        } else if (bytes_count_this < bytes_count_other) {
            for (auto i = min_bytes_count; i < bytes_count_other; ++i) {
                if (other.m_data[bytes_count_other - 1 - i] != 0) {
                    return false;
                }
            }
        } else {
            assert(false);
        }

        return true;
    }

    template <std::size_t OtherBits, typename = typename std::enable_if<Bits != OtherBits>::type>
    bool operator<(xtop_uint<OtherBits> const & other) const noexcept {
        auto constexpr bytes_count_this = bytes_size;
        auto constexpr bytes_count_other = xtop_uint<OtherBits>::bytes_size;
        auto constexpr min_bytes_count = std::min(bytes_count_this, bytes_count_other);
        auto constexpr index_boundary_this = bytes_count_this - min_bytes_count;
        auto constexpr index_boundary_other = bytes_count_other - min_bytes_count;

        if (bytes_count_this > bytes_count_other) {
            for (auto i = 0u; i < index_boundary_this; ++i) {
                if (m_data[i] > 0) {
                    return false;
                }
            }
        } else if (bytes_count_this < bytes_count_other) {
            for (auto i = 0u; i < index_boundary_other; ++i) {
                if (other.m_data[i] > 0) {
                    return true;
                }
            }
        } else {
            assert(false);
        }

        for (auto i = 0u; i < min_bytes_count; ++i) {
            if (m_data[index_boundary_this + i] != other.m_data[index_boundary_other + i]) {
                return m_data[index_boundary_this + i] < other.m_data[index_boundary_other + i];
            }
        }

        return false;
    }

    xtop_uint operator>>(int const shift) const {
        auto result = *this;
        return result >>= shift;
    }

    xtop_uint & operator>>=(int const shift) {
        if (shift < 0) {
            throw std::invalid_argument{"xtop_uint::operator>>=(negative)"};
        }

        auto const dv = std::div(shift, 8);  // calc how many bytes and bits to shift.
        assert(dv.quot >= 0);
        if (static_cast<std::size_t>(dv.quot) >= bytes_size) {
            // if shifted bytes more than the unit bytes, the result is zero.
            m_data.fill(0);
        } else if (dv.quot > 0) {
            std::memmove(&m_data[dv.quot], &m_data[0], bytes_size - dv.quot);
            std::memset(&m_data[0], 0, dv.quot);
        }

        assert(dv.rem < 8);
        if (dv.rem > 0) {
            for (auto i = 0u; i < bytes_size; ++i) {
                auto index = bytes_size - 1 - i;
                m_data[index] = static_cast<xbyte_t>(m_data[index] >> dv.rem);
                if (index > 0) {
                    m_data[index] = m_data[index] | static_cast<xbyte_t>(m_data[index - 1] << (8 - dv.rem));
                }
            }
        }

        return *this;
    }

    xtop_uint operator<<(int const shift) const {
        xtop_uint result = *this;
        return result <<= shift;
    }

    xtop_uint & operator<<=(int const shift) {
        if (shift < 0) {
            throw std::invalid_argument{"xtop_uint::operator<<(negative)"};
        }

        auto const dv = std::div(shift, 8);  // calc how many bytes and bits to shift.
        assert(dv.quot >= 0 && dv.rem >= 0 && dv.rem < 8);
        if (static_cast<std::size_t>(dv.quot) >= bytes_size) {
            m_data.fill(0);
        } else if (dv.quot > 0) {
            std::memmove(&m_data[0], &m_data[dv.quot], bytes_size - dv.quot);
            std::memset(&m_data[bytes_size - dv.quot], 0, dv.quot);
        }

        if (dv.rem > 0) {
            for (auto i = 0u; i < bytes_size; ++i) {
                m_data[i] = static_cast<xbyte_t>(m_data[i] << dv.rem);
                if (i + 1 < bytes_size) {
                    m_data[i] = static_cast<xbyte_t>(m_data[i] | static_cast<xbyte_t>(m_data[i + 1] >> (8 - dv.rem)));
                }
            }
        }

        return *this;
    }

    xtop_uint & operator^=(xtop_uint const & other) noexcept {
        for (auto i = 0u; i < bytes_size; ++i) {
            m_data[i] ^= other.m_data[i];
        }

        return *this;
    }

    xtop_uint operator^(xtop_uint const & other) const noexcept {
        auto ret{*this};
        return ret ^= other;
    }

    xtop_uint & operator|=(xtop_uint const & other) noexcept {
        for (auto i = 0u; i < bytes_size; ++i) {
            m_data[i] |= other.m_data[i];
        }

        return *this;
    }

    xtop_uint operator|(xtop_uint const & other) const noexcept {
        auto ret{*this};
        return ret |= other;
    }

    xtop_uint & operator&=(xtop_uint const & other) noexcept {
        for (auto i = 0u; i < bytes_size; ++i) {
            m_data[i] &= other.m_data[i];
        }

        return *this;
    }

    xtop_uint operator&(xtop_uint const & other) const noexcept {
        auto ret{*this};
        return ret &= other;
    }

    xtop_uint operator~() const noexcept {
        xtop_uint ret;
        for (auto i = 0u; i < bytes_size; ++i) {
            ret.m_data[i] = ~m_data[i];
        }

        return ret;
    }

    xtop_uint & operator++() {
        auto i = bytes_size;
        do {
            ++m_data[--i];
        } while (i > 0 && m_data[i] == 0);

        return *this;
    }

    xtop_uint operator++(int) {
        auto ret{*this};

        auto i = bytes_size;
        do {
            ++m_data[--i];
        } while (i > 0 && m_data[i] == 0);

        return ret;
    }

    xtop_uint & operator--() {
        auto i = bytes_size;
        do {
            --m_data[--i];
        } while (i > 0 && m_data[i] == std::numeric_limits<xbyte_t>::max());

        return *this;
    }

    xtop_uint operator--(int) {
        auto ret{*this};

        auto i = bytes_size;
        do {
            ++m_data[--i];
        } while (i > 0 && m_data[i] == std::numeric_limits<xbyte_t>::max());

        return ret;
    }

    template <typename T>
    xtop_uint & operator=(T value) {
        XSTATIC_ASSERT(std::is_integral<T>::value);

        if (value < 0) {
            m_data.fill(0xff);
        } else {
            m_data.fill(0);
        }

        for (auto i = 0u; i < sizeof(T); ++i, value >>= 8) {
            auto v = static_cast<xbyte_t>(value & 0xFF);
            m_data[bytes_size - 1 - i] = v;
        }

        return *this;
    }
};

template <>
class xtop_uint<8> final {
private:
    std::uint8_t m_data{0};
};

template <>
class xtop_uint<16> final {
private:
    std::uint16_t m_data{0};
};

template <>
class xtop_uint<32> final {
private:
    std::uint32_t m_data{0};
};

template <>
class xtop_uint<64> final {
private:
    std::uint64_t m_data{0};
};

template <std::size_t Bits>
using xuint_t = xtop_uint<Bits>;

using xuint8_t = xuint_t<8>;

using xuint16_t = xuint_t<16>;
using xuint32_t = xuint_t<32>;
using xuint64_t = xuint_t<64>;

// using xuint128_t = xuint_t<128>;
using xuint256_t = xuint_t<256>;
using xuint512_t = xuint_t<512>;

NS_END1
