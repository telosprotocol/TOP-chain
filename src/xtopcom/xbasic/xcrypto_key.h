// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xmem.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xserializable_based_on.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>

NS_BEG1(top)

enum class xenum_crypto_key_type : std::uint8_t { invalid = 0, pub_key = 1, pri_key = 2 };
using xcrypto_key_type_t = xenum_crypto_key_type;

XINLINE_CONSTEXPR xcrypto_key_type_t pub = xcrypto_key_type_t::pub_key;
XINLINE_CONSTEXPR xcrypto_key_type_t pri = xcrypto_key_type_t::pri_key;

template <xcrypto_key_type_t KeyT>
class xtop_crypto_key;

template <xcrypto_key_type_t KeyT>
std::int32_t operator<<(base::xstream_t & stream, xtop_crypto_key<KeyT> const & key);

template <xcrypto_key_type_t KeyT>
std::int32_t operator>>(base::xstream_t & stream, xtop_crypto_key<KeyT> & key);

template <xcrypto_key_type_t KeyT>
class xtop_crypto_key final : public xserializable_based_on<void> {
private:
    std::string m_key{};

public:
    xtop_crypto_key() = default;
    xtop_crypto_key(xtop_crypto_key const &) = default;
    xtop_crypto_key & operator=(xtop_crypto_key const &) = default;
    xtop_crypto_key(xtop_crypto_key &&) = default;
    xtop_crypto_key & operator=(xtop_crypto_key &&) = default;
    ~xtop_crypto_key() = default;

    explicit xtop_crypto_key(std::string raw_key) noexcept : m_key{std::move(raw_key)} {}

    void swap(xtop_crypto_key & other) { std::swap(m_key, other.m_key); }

    bool empty() const noexcept { return m_key.empty(); }

    std::string to_string() const { return m_key; }

    bool operator==(xtop_crypto_key const & other) const noexcept { return m_key == other.m_key; }

    bool operator!=(xtop_crypto_key const & other) const noexcept { return !(*this == other); }

    friend std::int32_t operator<<<>(base::xstream_t & stream, xtop_crypto_key const & key);

    friend std::int32_t operator>><>(base::xstream_t & stream, xtop_crypto_key & key);

private:
    std::int32_t do_read(base::xstream_t & stream) override {
        auto const begin_size = stream.size();
        stream >> m_key;
        return begin_size - stream.size();
    }

    std::int32_t do_write(base::xstream_t & stream) const override {
        auto const begin_size = stream.size();
        stream << m_key;
        return stream.size() - begin_size;
    }
};

template <xcrypto_key_type_t KeyT>
std::int32_t operator<<(base::xstream_t & stream, xtop_crypto_key<KeyT> const & key) {
    return key.serialize_to(stream);
}

template <xcrypto_key_type_t KeyT>
std::int32_t operator>>(base::xstream_t & stream, xtop_crypto_key<KeyT> & key) {
    return key.serialize_from(stream);
}

template <xcrypto_key_type_t KeyT>
using xcrypto_key_t = xtop_crypto_key<KeyT>;

using xpublic_key_t = xcrypto_key_t<pub>;   // base64 presentation
using xprivate_key_t = xcrypto_key_t<pri>;

NS_END1
