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

#include "xvledger/xvaccount.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xcommon/xaddress.h"
#include "xcommon/xstring_id.hpp"

#include <string>

NS_BEG2(top, common)

class xtop_legacy_account_address final : public xstring_id_t<xtop_legacy_account_address> {
private:
    using id_base_t = xstring_id_t<xtop_legacy_account_address>;

    mutable base::enum_vaccount_addr_type m_type{base::enum_vaccount_addr_type_invalid};

public:
    xtop_legacy_account_address() = default;
    xtop_legacy_account_address(xtop_legacy_account_address const &) = default;
    xtop_legacy_account_address & operator=(xtop_legacy_account_address const &) = default;
    xtop_legacy_account_address(xtop_legacy_account_address &&) = default;
    xtop_legacy_account_address & operator=(xtop_legacy_account_address &&) = default;
    ~xtop_legacy_account_address() override = default;

    explicit xtop_legacy_account_address(std::string value);
    explicit xtop_legacy_account_address(char const * value);
    explicit xtop_legacy_account_address(xaccount_address_t const & address);

    using id_base_t::empty;
    using id_base_t::has_value;
    using id_base_t::hash;
    using id_base_t::value;

    using id_base_t::operator bool;

    void swap(xtop_legacy_account_address & other) noexcept;

    bool operator<(xtop_legacy_account_address const & other) const noexcept;

    bool operator==(xtop_legacy_account_address const & other) const noexcept;

    bool operator!=(xtop_legacy_account_address const & other) const noexcept;

    std::size_t length() const noexcept;

    std::size_t size() const noexcept;

    char const * c_str() const noexcept;

    base::enum_vaccount_addr_type type() const noexcept;
};

using xlegacy_account_address_t = xtop_legacy_account_address;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xlegacy_account_address_t> final {
    std::size_t operator()(top::common::xlegacy_account_address_t const & id) const noexcept;
};

NS_END1
