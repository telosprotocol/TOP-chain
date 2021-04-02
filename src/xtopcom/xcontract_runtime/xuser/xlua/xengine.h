// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_runtime/xuser/xlua/xengine_context.h"

#include <string>
#include <cstdint>
#include <memory>

NS_BEG3(top, contract_runtime, lua)

class xtop_engine {
public:
    xtop_engine() = default;
    xtop_engine(xtop_engine const &) = delete;
    xtop_engine & operator=(xtop_engine const &) = delete;
    xtop_engine(xtop_engine &&) = default;
    xtop_engine & operator=(xtop_engine &&) = default;
    virtual ~xtop_engine() = default;

    virtual void load_script(const std::string& code, xengine_context_t& ctx) = 0;
    virtual void publish_script(const std::string& code, xengine_context_t& ctx) = 0;
    virtual void process(common::xaccount_address_t const & contract_account, const std::string& code, xengine_context_t& ctx) = 0;
};
using xengine_t = xtop_engine;

NS_END3
