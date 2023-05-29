// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xfixed_hash.h"
#include "xbasic/xspan.h"
#include "xbasic/xstring_view.h"
#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address_fwd.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <system_error>
#include <vector>

NS_BEG2(top, common)

class xtop_eth_address {
private:
    static constexpr size_t SIZE{20};
    using internal_type = std::array<uint8_t, SIZE>;
    internal_type raw_address_{};

    mutable std::string hex_string_;

public:
    xtop_eth_address(xtop_eth_address const &) = default;
    xtop_eth_address & operator=(xtop_eth_address const &) = default;
    xtop_eth_address(xtop_eth_address &&) = default;
    xtop_eth_address & operator=(xtop_eth_address &&) = default;
    ~xtop_eth_address() = default;

    xtop_eth_address();

    explicit xtop_eth_address(std::array<uint8_t, SIZE> const & raw_account_address);

    static xtop_eth_address build_from(xaccount_address_t const & account_address, std::error_code & ec);
    static xtop_eth_address build_from(xaccount_address_t const & account_address);
    static xtop_eth_address build_from(std::array<uint8_t, 20> const & address_data);
    static xtop_eth_address build_from(xspan_t<xbyte_t const> address_data, std::error_code & ec);
    static xtop_eth_address build_from(xspan_t<xbyte_t const> address_data);
    static void build_from(xspan_t<xbyte_t const> address_data, xtop_eth_address & address, std::error_code & ec);
    static void build_from(xspan_t<xbyte_t const> address_data, xtop_eth_address & address);
    static xtop_eth_address build_from(xstring_view_t hex_string, std::error_code & ec);
    static xtop_eth_address build_from(xstring_view_t hex_string);
    template <typename InputIt>
    static xtop_eth_address build_from(InputIt begin, InputIt end, std::error_code & ec) {
        assert(!ec);
        xtop_eth_address address;
        if (std::distance(begin, end) != SIZE) {
            ec = common::error::xerrc_t::invalid_eth_address;
            return address;
        }

        assert(std::distance(begin, end) == SIZE);
        std::copy(begin, end, address.raw_address_.begin());

        return address;
    }

    template <typename InputIt>
    static xtop_eth_address build_from(InputIt begin, InputIt end) {
        std::error_code ec;
        auto address = build_from(begin, end, ec);
        top::error::throw_error(ec);
        return address;
    }

private:
    explicit xtop_eth_address(std::string const & account_string);
    explicit xtop_eth_address(std::string const & account_string, std::error_code & ec);

public:
    using value_type = internal_type::value_type;
    using size_type = internal_type::size_type;
    using difference_type = internal_type::difference_type;
    using reference = internal_type::reference;
    using const_reference = internal_type::const_reference;
    using pointer = internal_type::pointer;
    using const_pointer = internal_type::const_pointer;
    using iterator = internal_type::iterator;
    using const_iterator = internal_type::const_iterator;

    constexpr static size_t size() noexcept {
        return SIZE;
    }

    std::string const & to_hex_string() const;
    std::string to_string() const;
    xbytes_t to_bytes() const;
    xbytes_t to_h160() const;
    void to_h160(xh160_t & h160) const;
    xbytes_t to_h256() const;
    void to_h256(xh256_t & h256) const;
    char const * c_str() const;
    bool is_zero() const;

    xbyte_t const * data() const noexcept;

    static xeth_address_t const & zero();

    bool operator==(xeth_address_t const & rhs) const noexcept;
    bool operator!=(xeth_address_t const & rhs) const noexcept;
    bool operator<(xtop_eth_address const & rhs) const noexcept {
        return raw_address_ < rhs.raw_address_;
    }

    void clear() noexcept;

    size_t get_ex_alloc_size() const;

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    static xtop_eth_address random();
    static void random(xtop_eth_address & address);
};

NS_END2

NS_BEG1(std)

//template <>
//struct hash<top::common::xeth_address_t> final {
//    std::size_t operator()(top::common::xeth_address_t const & eth_address) const noexcept {
//        return std::hash<std::vector<char>>{}(eth_address.to_bytes());
//    }
//};

NS_END1
