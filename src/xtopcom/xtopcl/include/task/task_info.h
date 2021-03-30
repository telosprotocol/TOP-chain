#pragma once
#include <string>
#include <map>
#include <functional>
#include <memory>
#include "xdata/xtransaction.h"
#include <iostream>

namespace xChainSDK {

    const char* const ParamAccount = "account_addr";
    // const char* const ParamBlockOwner = "block_owner";
    const char* const ParamGetBlockType = "type"; // "hash","height","last"
    const char* const ParamBlockHash = "block_hash";
    const char* const ParamBlockHeight = "height";


    class task_info {
    public:
        task_info() : use_transaction(false) {
            trans_action = top::make_object_ptr<top::data::xtransaction_t>();
        }
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

