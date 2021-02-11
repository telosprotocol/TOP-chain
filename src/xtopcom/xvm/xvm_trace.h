// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <chrono>
#include "xbasic/xns_macro.h"
#include "xvm_define.h"
#include "xerror/xvm_error_code.h"

NS_BEG2(top, xvm)
// using std::chrono::time_point;
// using std::chrono::steady_clock;
using std::chrono::microseconds;

struct xtransaction_trace
{
    enum_xvm_error_code             m_errno{enum_xvm_error_code::ok};
    string                          m_errmsg{"ok"};
    //uint32_t                        m_tgas_usage{0};
    //uint32_t                        m_gas_limit{0};
    //time_point<steady_clock>        m_start;
    uint32_t                        m_instruction_usage{0};
    microseconds::rep               m_duration_us{0};
    uint32_t                        m_tgas_usage{0};
    uint32_t                        m_disk_usage{0};
};

using xtransaction_trace_ptr = std::shared_ptr<xtransaction_trace>;

NS_END2
