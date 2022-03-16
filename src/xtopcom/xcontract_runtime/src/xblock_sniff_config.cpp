// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xblock_sniff_config.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xcontract_runtime/xerror/xerror.h"

#include <cassert>
#include <type_traits>

NS_BEG2(top, contract_runtime)

xtop_sniff_broadcast_config::xtop_sniff_broadcast_config(xsniff_broadcast_type_t zone, xsniff_block_type_t type) : zone{zone}, type{type} {
}

xtop_sniff_timer_config::xtop_sniff_timer_config(uint32_t interval, std::string action, xtimer_strategy_type_t strategy) : timer_config_data{interval}, action{std::move(action)}, strategy{strategy} {
}

xtop_sniff_timer_config::xtop_sniff_timer_config(std::string tcc_config_name, std::string action, xtimer_strategy_type_t strategy) : timer_config_data{tcc_config_name}, action{std::move(action)}, strategy{strategy} {
}

xtop_sniff_block_config::xtop_sniff_block_config(common::xaccount_address_t const & sniff_address,
                                                 common::xaccount_address_t const & action_address,
                                                 std::string action,
                                                 xsniff_block_type_t type)
  : sniff_address{sniff_address}, action_address{action_address}, action{std::move(action)}, type(type) {
}

xsniff_type_t & operator&=(xsniff_type_t & lhs, xsniff_type_t const rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

xsniff_type_t & operator|=(xsniff_type_t & lhs, xsniff_type_t const rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

xtop_timer_config_data::xtop_timer_config_data(uint32_t const interval) noexcept : m_interval{interval} {
}

xtop_timer_config_data::xtop_timer_config_data(std::string tcc_config_name) noexcept : m_tcc_config_name{std::move(tcc_config_name)} {
}

uint32_t xtop_timer_config_data::get_timer_interval(std::error_code & ec) const {
    assert(!ec);
    if (m_tcc_config_name.empty()) {
        if (m_interval == 0) {
            ec = error::xerrc_t::invalid_timer_interval;
        }
        return m_interval;
    }

    try {
        config::xinterval_t interval{0};
        if (config::xconfig_register_t::get_instance().get(m_tcc_config_name, interval)) {
            xwarn("xtimer_config_data_t::get_timer_interval] key (%s) %d", m_tcc_config_name.c_str(), interval);
        } else {
            ec = error::xerrc_t::invalid_timer_interval;
        }

        return interval;
    } catch (top::error::xtop_error_t const & eh) {
        ec = eh.code();
    } catch (std::exception const & eh) {
        ec = error::xerrc_t::unknown_error;
    }

    return 0;
}

uint32_t xtop_timer_config_data::get_timer_interval() const {
    std::error_code ec;
    auto const r = get_timer_interval(ec);
    top::error::throw_error(ec);
    return r;
}

NS_END2
