// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_map>
#include <memory>
#include <functional>
#include <utility>
#include <chrono>
#include "xpre_request_data.h"
#include "xrpc/xerror/xrpc_error.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_signature.h"
#include "xbasic/xlru_cache_specialize.h"


NS_BEG2(top, xrpc)

using pre_request_handler = std::function<void(xpre_request_data_t &)>;
using xChainRPC::xrpc_signature;
using basic::xlru_cache_specialize;
using std::chrono::steady_clock;
struct edge_account_info {
    std::string                     m_token{};
    std::string                     m_secretid{};
    steady_clock::time_point        m_record_time_point;
};

template <typename... Args>
class pre_request_service
{
public:
    pre_request_service() {
    }

    virtual ~pre_request_service() { }
    virtual void init() {}
    virtual void parse_request(xpre_request_data_t& request, const std::string& request_data) { request.parse_request(request_data); }
    virtual void check_request(xpre_request_data_t& request) { request.check_request(); }
    virtual void call_method(xpre_request_data_t& request) {
#if (defined ENABLE_RPC_TOKEN) && (ENABLE_RPC_TOKEN != 0)
        auto account_address = request.get_request_value("account_address");
        if (!account_address.empty()) {
            //std::lock_guard<std::mutex> lock(m_mutex);
            edge_account_info account_info;
            if (!m_account_info_map.get(account_address, account_info)) {
                throw xrpc_error{ enum_xrpc_error_code::rpc_param_param_lack, "need request token or token is not valid" };
            }
            if (account_info.m_token != request.get_request_value("token")) {
                throw xrpc_error{ enum_xrpc_error_code::rpc_param_param_lack, "miss param token or token is not valid" };
            }
            if (!check_signature(request, account_address, account_info.m_secretid)) {
                throw xrpc_error{ enum_xrpc_error_code::rpc_param_signature_error, "signature error" };
            }
        }
#endif
    }

    bool check_signature(xpre_request_data_t& request,
        const std::string& account_address,
        const std::string& secret_key) {
        if (!m_need_check_signature)
            return true;

        xrpc_signature sign;
        for (auto it : request.m_request_map) {
            sign.insert_param(it.first, it.second);
        }
        return sign.check_parsed_params(secret_key);
    }
    //static std::mutex                                              m_mutex;
    static xlru_cache_specialize<std::string, edge_account_info>   m_account_info_map;
    bool                                                           m_need_check_signature{ true };
};


template<typename... Args>
xlru_cache_specialize<std::string, edge_account_info> pre_request_service<Args...>::m_account_info_map{32768};


template <typename T, typename... Args>
class pre_request_service<T, Args...> : public pre_request_service<Args...>
{
public:
    pre_request_service<T, Args...>(T* t, Args*... args): pre_request_service<Args...>(args...), m_interface(t) {
        if (!m_interface)
            return;
        for (auto const& method : m_interface->methods()) {
            m_methods[method.first] = method.second;
        }
    }
    void init() override {
        m_interface->init(this);
        pre_request_service<Args...>::init();
    }

    void call_method(xpre_request_data_t& request) override {
        const string& method = request.get_request_value(RPC_METHOD);
        const string& version = request.get_request_value(RPC_VERSION);

        auto iter = m_methods.find(pair<string, string>(version, method));
        if (iter != m_methods.end()) {
            try {
                (iter->second)(request);
            }
            catch(const xrpc_error& e) {
                throw e;
            }
        } else {
            pre_request_service<Args...>::call_method(request);
        }
    }

private:
    std::unique_ptr<T>                                                              m_interface;
    std::unordered_map<std::pair<std::string, std::string>, pre_request_handler>    m_methods;
};

NS_END2
