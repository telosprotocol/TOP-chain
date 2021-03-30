// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <functional>

NS_BEG1(top)

template <typename RetT, typename ... ArgsT>
class xtop_action final
{
private:
    std::function<RetT(ArgsT...)> m_actor;

public:
    xtop_action(xtop_action const &)             = default;
    xtop_action & operator=(xtop_action const &) = default;
    xtop_action(xtop_action &&)                  = default;
    xtop_action & operator=(xtop_action &&)      = default;
    ~xtop_action()                               = default;

    xtop_action(std::function<RetT(ArgsT...)> && actor)
        : m_actor{ std::move(actor) }
    {
    }

    xtop_action(std::function<RetT(ArgsT...)> const & actor)
        : m_actor{ actor }
    {
    }

    RetT
    operator()(ArgsT ... args) const {
        return m_actor(std::forward<ArgsT>(args)...);
    }
};

template <typename RetT, typename ... ArgsT>
using xaction_t = xtop_action<RetT, ArgsT...>;

NS_END1
