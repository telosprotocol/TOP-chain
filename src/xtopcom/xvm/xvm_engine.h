// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include "xcommon/xaddress.h"
NS_BEG2(top, xvm)
class xvm_context;
class xengine
{
    public:
        virtual ~xengine() {}
        //virtual void init() = 0;
        virtual void load_script(const std::string& code, xvm_context& ctx) = 0;
        virtual void publish_script(const std::string& code, xvm_context& ctx) = 0;
        // virtual int32_t add_abi(const string &code, const string& abi, xvm_context &ctx) = 0;
        virtual void process(common::xaccount_address_t const & contract_account, const std::string& code, xvm_context& ctx) = 0;
};
NS_END2
