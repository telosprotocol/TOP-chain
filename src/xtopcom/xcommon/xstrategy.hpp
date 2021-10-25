// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "assert.h"
#include "xbasic/xutility.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xuser_option.h"

#include <functional>
#include <map>
#include <vector>

NS_BEG2(top, common)

enum class xstrategy_priority_enum_t : size_t {
    invalid = 0,
    lowest = 1,
    low = 2,
    normal = 3,
    high = 4,
    highest = 5,
};

enum class xstrategy_value_enum_t : size_t {
    invalid = 0,
    enable = 1,
    disable = 2,
};

using xdefault_strategy_t = std::pair<xstrategy_value_enum_t, xstrategy_priority_enum_t>;
using xnode_type_strategy_t = std::tuple<xnode_type_t, xstrategy_value_enum_t, xstrategy_priority_enum_t>;
using xuser_strategy_t = std::pair<common::xuser_option_ptr_t, xstrategy_priority_enum_t>;

class xtop_bool_strategy_t {
private:
    enum xbool_strategy_type_enum : size_t {
        invalid = 0,
        defaulty = 1,
        user = 2,
        node_type = 3,
    };

private:
    xdefault_strategy_t m_default_strategy{xstrategy_value_enum_t::invalid, xstrategy_priority_enum_t::invalid};
    std::map<common::xstrategy_priority_enum_t, xnode_type_strategy_t> m_node_type_strategy;
    xuser_strategy_t m_user_strategy{nullptr, xstrategy_priority_enum_t::invalid};

    std::map<xstrategy_priority_enum_t, xtop_bool_strategy_t::xbool_strategy_type_enum> m_strategy_type_map;

public:
    xtop_bool_strategy_t() = default;
    xtop_bool_strategy_t(xtop_bool_strategy_t const &) = default;
    xtop_bool_strategy_t & operator=(xtop_bool_strategy_t const &) = default;
    xtop_bool_strategy_t(xtop_bool_strategy_t &&) = default;
    xtop_bool_strategy_t & operator=(xtop_bool_strategy_t &&) = default;
    ~xtop_bool_strategy_t() = default;

public:
    void add_strategy(xdefault_strategy_t const & default_strategy) {
        auto const & priority_level = top::get<xstrategy_priority_enum_t>(default_strategy);
        // not set this level before
        if (m_strategy_type_map.find(priority_level) != m_strategy_type_map.end()) {
            xassert(false);
        }
        xassert(top::get<xstrategy_value_enum_t>(default_strategy) != xstrategy_value_enum_t::invalid);

        // not set default_strategy before
        if (top::get<xstrategy_value_enum_t>(m_default_strategy) != xstrategy_value_enum_t::invalid || top::get<xstrategy_priority_enum_t>(m_default_strategy) != xstrategy_priority_enum_t::invalid) {
            xassert(false);
        }
        m_default_strategy = default_strategy;
        m_strategy_type_map.insert({priority_level, xtop_bool_strategy_t::xbool_strategy_type_enum::defaulty});
    }

    void add_strategy(xnode_type_strategy_t const & node_type_strategy) {
        auto const & priority_level = std::get<2>(node_type_strategy);
        if (m_strategy_type_map.find(priority_level) != m_strategy_type_map.end()) {
            xassert(false);
        }
        xassert(std::get<1>(node_type_strategy) != xstrategy_value_enum_t::invalid);

        m_node_type_strategy[priority_level] = node_type_strategy;
        m_strategy_type_map.insert({priority_level, xtop_bool_strategy_t::xbool_strategy_type_enum::node_type});
    }

    void add_strategy(xuser_strategy_t const & user_strategy) {
        auto const & priority_level = top::get<xstrategy_priority_enum_t>(m_user_strategy);
        if (m_strategy_type_map.find(priority_level) != m_strategy_type_map.end()) {
            xassert(false);
        }

        if (top::get<common::xuser_option_ptr_t>(m_user_strategy) == nullptr || priority_level != xstrategy_priority_enum_t::invalid) {
            xassert(false);
        }
        m_user_strategy = user_strategy;
        m_strategy_type_map.insert({priority_level, xtop_bool_strategy_t::xbool_strategy_type_enum::user});
    }

    // use node_type info at this moment to decide whether should use this api/function.
    bool allow(common::xnode_type_t const & c_node_type) const {
        for (auto riter = m_strategy_type_map.crbegin(); riter != m_strategy_type_map.crend(); ++riter) {
            switch (riter->second) {
            case xbool_strategy_type_enum::defaulty: {
                xassert(top::get<xstrategy_value_enum_t>(m_default_strategy) != xstrategy_value_enum_t::invalid);
                return top::get<xstrategy_value_enum_t>(m_default_strategy) == xstrategy_value_enum_t::enable ? true : false;
            }
            case xbool_strategy_type_enum::user: {
                // return m_user_strategy.first->xxx();
                xassert(false);
                return false;
            }
            case xbool_strategy_type_enum::node_type: {
                auto const priority_level = top::get<const xstrategy_priority_enum_t>(*riter);
                auto const & node_type_strategy = m_node_type_strategy.at(priority_level);
                common::xnode_type_t strategy_type = std::get<0>(node_type_strategy);
                if ((c_node_type & strategy_type) == strategy_type) {
                    xassert(std::get<1>(node_type_strategy) != xstrategy_value_enum_t::invalid);
                    return std::get<1>(node_type_strategy) == xstrategy_value_enum_t::enable ? true : false;
                }
                break;
            }
            case xbool_strategy_type_enum::invalid:
            default:
                xassert(false);
            }
        }
        xassert(false);
        return false;
    }
};
using xbool_strategy_t = xtop_bool_strategy_t;

template <class SC, class U>
SC inner_define_startegy_impl(SC & s, U u) {
    s.add_strategy(u);
    return s;
}

template <class SC, class U, class... Args>
SC inner_define_startegy_impl(SC & sc, U u, Args... rest) {
    sc.add_strategy(u);
    return inner_define_startegy_impl(sc, rest...);
}

template <class T>
xbool_strategy_t define_bool_strategy(T t) {
    xbool_strategy_t s;
    s.add_strategy(t);
    return s;
}

template <class T, class... Args>
xbool_strategy_t define_bool_strategy(T t, Args... rest) {
    xbool_strategy_t s;
    s.add_strategy(t);
    return inner_define_startegy_impl<xbool_strategy_t>(s, rest...);
}

NS_END2