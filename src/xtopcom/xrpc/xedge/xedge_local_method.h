// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <mutex>
#include "xrpc/xrpc_define.h"
#include "xrpc/xjson_proc.h"
#include "xutility/xhash.h"
#include "xcrypto/xckey.h"
#include "xelect_net/include/elect_main.h"

NS_BEG2(top, xrpc)

template<class T>
class xedge_local_method {
    typedef typename T::conn_type conn_type;
    using local_method_handler = std::function<void(xjson_proc_t &json_proc)>;
public:
    xedge_local_method(observer_ptr<elect::ElectMain>, common::xip2_t xip2);
    bool do_local_method(pair<string, string>& version_method, xjson_proc_t &json_proc);
    void sign_transaction_method(xjson_proc_t &json_proc);
    void account_method(xjson_proc_t &json_proc);
    void import_private_key_method(xjson_proc_t &json_proc);
    void get_private_keys_method(xjson_proc_t &json_proc);
    void get_edge_neighbors_method(xjson_proc_t &json_proc);
    void reset_edge_local_method(common::xip2_t xip2);
public:
    static unordered_map<string, vector<string>> m_sub_account_map;
    static std::mutex m_mutex;
private:
    unordered_map<pair<string, string>, local_method_handler> m_edge_local_method_map;
    observer_ptr<elect::ElectMain>                            m_elect_main;
    common::xip2_t                                            m_xip2;
};

NS_END2
