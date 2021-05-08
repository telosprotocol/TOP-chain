// Copyright (c) 2017-2021 Telos Foundation & contributors
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

#include "xvledger/xvstate.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_common/xproperties/xaccess_control_data.h"
#include "xcontract_common/xproperties/xaccess_control_fwd.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"

#include <system_error>

NS_BEG3(top, contract_common, properties)

class xtop_access_control {
    observer_ptr<base::xvbstate_t> m_state;
    xproperty_access_control_data_t m_ac_data;

public:
    xtop_access_control() = default;
    xtop_access_control(xtop_access_control const &) = delete;
    xtop_access_control & operator=(xtop_access_control const &) = delete;
    xtop_access_control(xtop_access_control &&) = default;
    xtop_access_control & operator=(xtop_access_control &&) = default;
    ~xtop_access_control() = default;

    explicit xtop_access_control(observer_ptr<base::xvbstate_t> state) noexcept;
    //xtop_access_control(observer_ptr<base::xvbstate_t> state, std::string const & json);
    xtop_access_control(observer_ptr<base::xvbstate_t> state, xproperty_access_control_data_t data) noexcept;

    void load_access_control_data(std::string const & json, std::error_code & ec);
    void load_access_control_data(xproperty_access_control_data_t data);

    void rebind(observer_ptr<base::xvbstate_t> state) noexcept;

    bool read_permitted(common::xaccount_address_t const & reader,
                        xproperty_identifier_t const & property_id) const noexcept;

    bool write_permitted(common::xaccount_address_t const & writer,
                         xproperty_identifier_t const & property_id) const noexcept;

    void create_property(common::xaccount_address_t const & writer, xproperty_identifier_t const & peroperty_id, std::error_code & ec);
    bool has_property(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id, std::error_code & ec) const noexcept;
    bool has_property(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id) const noexcept;

    std::string src_code(xproperty_identifier_t const & property_id, std::error_code & ec) const;
    void src_code(xproperty_identifier_t const & property_id, std::string code, std::error_code & ec);
    xbyte_buffer_t bin_code(common::xaccount_address_t const & reader, xproperty_identifier_t const & property_id, std::error_code & ec) const;

    template <xproperty_type_t PropertyT, typename std::enable_if<PropertyT == xproperty_type_t::token>::type * = nullptr>
    void increase(xproperty_identifier_t const & property_id,
                  common::xaccount_address_t const & writer,
                  common::xaccount_address_t const & to,
                  typename xtype_of_t<PropertyT>::type const value,
                  std::error_code & ec);

    template <xproperty_type_t PropertyT, typename std::enable_if<PropertyT == xproperty_type_t::token>::type * = nullptr>
    void decrease(xproperty_identifier_t const & property_id,
                  common::xaccount_address_t const & writer,
                  common::xaccount_address_t const & to,
                  typename xtype_of_t<PropertyT>::type const value,
                  std::error_code & ec);
};

NS_END3

#include "xcontract_common/xerror/xerror.h"

NS_BEG3(top, contract_common, properties)

template <xproperty_type_t PropertyT, typename std::enable_if<PropertyT == xproperty_type_t::token>::type *>
void xtop_access_control::increase(xproperty_identifier_t const & property_id,
                                   common::xaccount_address_t const & writer,
                                   common::xaccount_address_t const & to,
                                   typename xtype_of_t<PropertyT>::type const value,
                                   std::error_code & ec) {
    if (!write_permitted(writer, property_id)) {
        ec = error::xerrc_t::permission_not_allowed;
        assert(false);
        return;
    }

    if (common::xaccount_address_t{m_state->get_account_addr()} != to) {
        ec = error::xerrc_t::account_not_matched;
        assert(false);
        return;
    }

    auto token = m_state->load_token_var(property_id.name());
    token->deposit(value);
}

template <xproperty_type_t PropertyT, typename std::enable_if<PropertyT == xproperty_type_t::token>::type *>
void xtop_access_control::decrease(xproperty_identifier_t const & property_id,
                                   common::xaccount_address_t const & writer,
                                   common::xaccount_address_t const & to,
                                   typename xtype_of_t<PropertyT>::type const value,
                                   std::error_code & ec) {
    if (!write_permitted(writer, property_id)) {
        ec = error::xerrc_t::permission_not_allowed;
        assert(false);
        return;
    }

    if (common::xaccount_address_t{m_state->get_account_addr()} != to) {
        ec = error::xerrc_t::account_not_matched;
        assert(false);
        return;
    }

    auto const & token = m_state->load_token_var(property_id.name());
    auto const balance = token->get_balance();
    if (balance < value) {
        ec = error::xerrc_t::token_not_enough;
        assert(false);
        return;
    }
    token->withdraw(value);
}

NS_END3
