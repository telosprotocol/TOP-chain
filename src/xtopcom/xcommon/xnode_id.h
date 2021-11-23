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
#include "xvledger/xvaccount.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xaccount_base_address.h"
#include "xcommon/xaccount_id.h"
#include "xcommon/xledger_id.h"
#include "xcommon/xtable_id.h"

#include <cstdint>
#include <string>

NS_BEG2(top, common)

std::int32_t operator <<(top::base::xstream_t & stream, xtop_node_id const & node_id);
std::int32_t operator >>(top::base::xstream_t & stream, xtop_node_id & node_id);
std::int32_t operator<<(top::base::xbuffer_t & stream, xtop_node_id const & node_id);
std::int32_t operator>>(top::base::xbuffer_t & stream, xtop_node_id & node_id);

class xtop_node_id final {
private:
    std::string m_account_string;
    xaccount_base_address_t m_account_base_address;
    xtable_id_t m_assigned_table_id;

    xaccount_id_t m_account_id{};

public:
    xtop_node_id()                                 = default;
    xtop_node_id(xtop_node_id const &)             = default;
    xtop_node_id & operator=(xtop_node_id const &) = default;
    xtop_node_id(xtop_node_id &&)                  = default;
    xtop_node_id & operator=(xtop_node_id &&)      = default;
    ~xtop_node_id()                                = default;

    explicit xtop_node_id(std::string const & value);
    explicit xtop_node_id(xaccount_base_address_t base_address);
    explicit xtop_node_id(xaccount_base_address_t base_address, uint16_t const table_id_value);
    explicit xtop_node_id(xaccount_base_address_t base_address, xtable_id_t table_id);

    bool empty() const noexcept;
    bool has_value() const noexcept;
    std::string const & value() const noexcept;
    xaccount_base_address_t const & base_address() const noexcept;
    uint64_t hash() const;
    std::string const & to_string() const noexcept;
    void clear();

    explicit operator bool() const noexcept;

    void
    swap(xtop_node_id & other) noexcept;

    bool operator==(xtop_node_id const & other) const noexcept;
    bool operator<(xtop_node_id const & other) const noexcept;
    bool operator>(xtop_node_id const & other) const noexcept;
    bool operator!=(xtop_node_id const & other) const noexcept;
    bool operator>=(xtop_node_id const & other) const noexcept;
    bool operator<=(xtop_node_id const & other) const noexcept;

    void
    random();

    std::size_t
    length() const noexcept;

    std::size_t
    size() const noexcept;

    char const *
    c_str() const noexcept;

    base::enum_vaccount_addr_type type(std::error_code & ec) const;
    base::enum_vaccount_addr_type type() const;

    xaccount_id_t const & account_id() const noexcept;

    xledger_id_t const & ledger_id() const noexcept;
    xtable_id_t const & table_id() const noexcept;

    friend std::int32_t operator<<(base::xstream_t & stream, xtop_node_id const & node_id);
    friend std::int32_t operator>>(base::xstream_t & stream, xtop_node_id & node_id);
    friend std::int32_t operator<<(base::xbuffer_t & stream, xtop_node_id const & node_id);
    friend std::int32_t operator>>(base::xbuffer_t & stream, xtop_node_id & node_id);

    std::int32_t serialize_to(base::xstream_t & stream) const;
    std::int32_t serialize_from(base::xstream_t & stream);
    std::int32_t serialize_to(base::xbuffer_t & buffer) const;
    std::int32_t serialize_from(base::xbuffer_t & buffer);

private:
    void parse();

    std::int32_t
    do_read(base::xstream_t & stream);

    std::int32_t
    do_write(base::xstream_t & stream) const;
};

using xnode_id_t = xtop_node_id;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xnode_id_t> final {
    std::size_t
    operator()(top::common::xnode_id_t const & id) const noexcept;
};

NS_END1
