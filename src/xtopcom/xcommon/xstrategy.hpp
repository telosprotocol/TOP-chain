// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "assert.h"
#include "xbasic/xutility.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xuser_option.h"

#include <functional>
#include <vector>

NS_BEG2(top, common)

class xtop_bool_strategy_t {
private:
    std::vector<std::pair<common::xnode_type_t, bool>> m_node_type_strategy;
    bool m_default_setting{false};
    std::size_t m_user_option_location{0};
    common::xuser_option_ptr_t m_user_option{nullptr};

public:
    xtop_bool_strategy_t() = default;
    xtop_bool_strategy_t(xtop_bool_strategy_t const &) = default;
    xtop_bool_strategy_t & operator=(xtop_bool_strategy_t const &) = default;
    xtop_bool_strategy_t(xtop_bool_strategy_t &&) = default;
    xtop_bool_strategy_t & operator=(xtop_bool_strategy_t &&) = default;
    ~xtop_bool_strategy_t() = default;

    xtop_bool_strategy_t(bool default_strategy) : m_default_setting{default_strategy} {
    }

public:
    void deal(common::xnode_type_t node_type, bool if_open) {
        m_node_type_strategy.push_back({node_type, if_open});
    }

    // relay on user_option(which might changed by user)
    void deal(common::xuser_option_ptr_t user_option, std::_Placeholder<1> const &) {
        assert(m_user_option == nullptr);
        m_user_option = user_option;
        m_user_option_location = m_node_type_strategy.size();  // put user_option priority
    }

    // use node_type info at this moment to decide whether should use this api/function.
    bool if_open(common::xnode_type_t const & c_node_type) const {
        bool result{m_default_setting};

        std::size_t index;
        for (index = 0; index < m_user_option_location; ++index) {
            common::xnode_type_t define_type = top::get<common::xnode_type_t>(m_node_type_strategy[index]);
            if ((define_type & c_node_type) == define_type) {
                result = top::get<bool>(m_node_type_strategy[index]);
            }
        }
        if (m_user_option) {
            // result = m_user_option->xxx();
        }
        for (; index < m_user_option_location; ++index) {
            common::xnode_type_t define_type = top::get<common::xnode_type_t>(m_node_type_strategy[index]);
            if ((define_type & c_node_type) == define_type) {
                result = top::get<bool>(m_node_type_strategy[index]);
            }
        }

        return result;
    }
};
using xbool_strategy_t = xtop_bool_strategy_t;

template <class S, class U, class V>
S inner_define_startegy_impl(S & s, U u, V v) {
    s.deal(u, v);
    return s;
}

template <class S, class U, class V, class... Args>
S inner_define_startegy_impl(S & s, U u, V v, Args... rest) {
    static_assert(sizeof...(rest) % 2 == 0, "inner_define_startegy_impl U,V should be in pair");
    s.deal(u, v);
    return inner_define_startegy_impl(s, rest...);
}

xbool_strategy_t define_bool_strategy(bool dft);

/**
 * define_bool_strategy:
 * usage:
 * define_bool_strategy(defaultly_value, <node_type, (bool)strategy> or <user_option_ptr, std::_Placeholder<1>>... )
 * you can define `pairs` strategies as much as you want.
 * there will affect one by one and determined the final result.
 * e.g. define_bool_strategy(false, xnode_type_t::consensus, true, some_user_option_ptr, std::placeholders::_1, archive, false)
 **/
template <class U, class V, class... Args>
xbool_strategy_t define_bool_strategy(bool dft, U u, V v, Args... rest) {
    xbool_strategy_t s{dft};
    return inner_define_startegy_impl<xbool_strategy_t, U, V, Args...>(s, u, v, rest...);
}

NS_END2