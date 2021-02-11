// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <functional>

NS_BEG1(top)

class xtop_scope_executer final
{
    std::function<void()> m_execution_definition;

public:
    xtop_scope_executer(xtop_scope_executer const &) = delete;
    xtop_scope_executer & operator=(xtop_scope_executer const &) = delete;
    xtop_scope_executer(xtop_scope_executer &&) = default;
    xtop_scope_executer& operator=(xtop_scope_executer &&) = default;
    ~xtop_scope_executer() noexcept;

    explicit
    xtop_scope_executer(std::function<void()> && execution_body) noexcept;
};
using xscope_executer_t = xtop_scope_executer;

NS_END1
