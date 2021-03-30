// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte.h"
#include "xbasic/xbyte_buffer.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>
#include <type_traits>

NS_BEG1(top)

// store data in big-ending order

template <std::size_t Bytes>
class xtop_hash final
{
public:
    static constexpr std::size_t bytes_size{ Bytes };
    static constexpr std::size_t bits_size{ Bytes * 8 };

    struct xtop_construction_option final
    {
        enum xenum_option : std::uint8_t
        {
            align_right = 0x01,
            hex         = 0x02,
            size_match  = 0x04
        };

        xenum_option value;
    };
    using xconstruction_option_t = xtop_construction_option;

private:
    std::array<xbyte_t, bytes_size> m_data{};

public:
    xtop_hash(xtop_hash const &)             = default;
    xtop_hash & operator=(xtop_hash const &) = default;
    xtop_hash(xtop_hash &&)                  = default;
    xtop_hash & operator=(xtop_hash &&)      = default;
    ~xtop_hash()                             = default;

    xtop_hash() {
        m_data.fill(0);
    }

    template <typename T>
    explicit
    xtop_hash(T const value) : xtop_hash{}
    {
        XSTATIC_ASSERT(std::is_unsigned<T>::value && !std::is_floating_point<T>::value);

        for (auto i = m_data.size(); i != 0; --i, value >>= 8)
        {
            auto v = static_cast<xbyte_t>(value & 0xFF);
            m_data[i - 1] = v;
        }
    }

    explicit
    xtop_hash(std::array<xbyte_t, Bytes> const & src_data)
        : m_data(src_data)
    {
    }

    explicit
    xtop_hash(xbyte_buffer_t const & bytes,
              xconstruction_option_t const & option = { xconstruction_option_t::size_match }) {
        if (bytes.size() == bytes_size) {
            std::memcpy(m_data.data(), bytes.data(), std::min(bytes.size(), bytes_size));
        } else {
            if (option.value & xconstruction_option_t::size_match) {
                return;
            }

            m_data.fill(0);

            auto const s = std::min(bytes_size, bytes.size());
            auto const align_right = static_cast<bool>(option.value & xconstruction_option_t::align_right);

            if (align_right) {
                for (auto i = 0u; i < s; ++i) {
                    m_data[bytes_size - 1 - i] = bytes[bytes.size() - 1 - i];
                }
            } else {
                for (auto i = 0u; i < s; ++i) {
                    m_data[i] = bytes[i];
                }
            }
        }
    }

    bool
    operator==(xtop_hash const & other) const noexcept {
        return m_data == other.m_data;
    }

    bool
    operator!=(xtop_hash const & other) const noexcept {
        return !(*this == other);
    }

    bool
    operator<(xtop_hash const & other) const noexcept {
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

    xtop_hash &
    operator^=(xtop_hash const & other) noexcept {
        for (auto i = 0u; i < bytes_size; ++i) {
            m_data[i] = m_data[i] ^ other.m_data[i];
        }

        return *this;
    }

    xtop_hash
    operator^(xtop_hash const & other) const noexcept {
        auto ret = *this;
        return ret ^= other;
    }

    xtop_hash &
    operator|=(xtop_hash const & other) noexcept {
        for (auto i = 0u; i < bytes_size; ++i)
        {
            m_data[i] = m_data[i] | other.m_data[i];
        }

        return *this;
    }

    xtop_hash
    operator|(xtop_hash const & other) const noexcept {
        auto ret = *this;
        return ret |= other;
    }

    xtop_hash &
    operator&=(xtop_hash const & other) noexcept {
        for (auto i = 0u; i < bytes_size; ++i) {
            m_data[i] = m_data[i] & other.m_data[i];
        }

        return *this;
    }

    xtop_hash
    operator&(xtop_hash const & other) const noexcept {
        auto ret = *this;
        return ret &= other;
    }

    xtop_hash
    operator~() const noexcept {
        xtop_hash ret;
        for (auto i = 0u; i < bytes_size; ++i) {
            ret.m_data[i] = static_cast<xbyte_t>(~m_data[i]);
        }

        return ret;
    }

    xtop_hash &
    operator++() {
        auto i = bytes_size;
        do { ++m_data[--i]; } while (i > 0 && m_data[i] == 0);

        return *this;
    }

    xtop_hash
    operator++(int) {
        auto ret = *this;

        auto i = bytes_size;
        do { ++m_data[--i]; } while (i > 0 && m_data[i] == 0);

        return ret;
    }

    xtop_hash &
    operator--() {
        auto i = bytes_size;
        do { --m_data[--i]; } while (i > 0 && m_data[i] == std::numeric_limits<xbyte_t>::max());

        return *this;
    }

    xtop_hash
    operator--(int) {
        auto ret = *this;

        auto i = bytes_size;
        do { ++m_data[--i]; } while (i > 0 && m_data[i] == std::numeric_limits<xbyte_t>::max());

        return ret;
    }

    xbyte_t *
    data() noexcept {
        return m_data.data();
    }

    xbyte_t const *
    data() const noexcept {
        return m_data.data();
    }

    std::array<xbyte_t, Bytes> &
    as_array() noexcept {
        return m_data;
    }

    std::array<xbyte_t, Bytes> const &
    as_array() const noexcept {
        return m_data;
    }

    auto
    begin() const -> typename std::array<xbyte_t, Bytes>::const_iterator {
        return m_data.begin();
    }

    auto
    end() const -> typename std::array<xbyte_t, Bytes>::const_iterator {
        return m_data.end();
    }

    std::size_t
    size() const noexcept {
        return m_data.size();
    }

    void
    clear() {
        m_data.fill(0);
    }

    static
    xtop_hash
    random() {
        std::uniform_int_distribution<std::uint16_t> distribution
        {
            std::numeric_limits<xbyte_t>::min(),
            std::numeric_limits<xbyte_t>::max()
        };

        std::random_device rd{};

        xtop_hash ret;
        for (auto & byte : ret.m_data) {
            byte = static_cast<xbyte_t>(distribution(rd));
        }

        return ret;
    }
};

template <std::size_t Bytes>
using xhash_t = xtop_hash<Bytes>;

extern
template
class xtop_hash<32>;

using xhash256_t = xhash_t<32>;

NS_END1
