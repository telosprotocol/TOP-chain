// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include "xdata/xtx_factory.h"

namespace xChainSDK {

    const char* const ParamAccount = "account_addr";
    const char* const ParamGetBlockType = "type"; // "hash","height","last"
    const char* const ParamBlockHash = "block_hash";
    const char* const ParamBlockHeight = "height";

    class task_info {
    public:
        task_info();
        virtual ~task_info() {
        //    std::cout << "destory task_info" << std::endl;
        }
        uint32_t task_id;
        std::string host;
        std::string uri;
        std::string content;
        std::string method;
        std::map<std::string, std::string> params;
        top::data::xtransaction_ptr_t trans_action;
        bool use_transaction;
    };

    template <typename T>
    class task_info_callback : public task_info {
    public:
        using callback_t = std::function<void(T*)>;

        callback_t callback_;
    };
}

