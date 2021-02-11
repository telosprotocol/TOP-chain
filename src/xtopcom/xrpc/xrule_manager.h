// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xjson_proc.h"
#include "xrpc_define.h"
NS_BEG2(top, xrpc)

// class filter_base
// {
//     public:
//     virtual void execute(string request) = 0;
// };

// class ip_filter:public filter_base
// {
//     public:
//     void execute(string request);
// };
// class account_filter:public filter_base
// {
//     public:
//     void execute(string request);
// };

// class filter_chain_t
// {
//     void register_filter();
//     void exec_filter();
//     public:
//     vector<unique_ptr<filter_base>> m_filter_chain;
// };

using filter_handler = std::function<void(xjson_proc_t &)>;

#define ADDRESS_LENGTH 35

class xfilter_manager {
public:
    xfilter_manager();
    void getAccount_filter(xjson_proc_t & json_proc);
    void getTransaction_filter(xjson_proc_t & json_proc);
    // void create_account_filter(xjson_proc_t& json_proc);
    // void transfer_filter(xjson_proc_t& json_proc);
    void filter(xjson_proc_t & json_proc);
    // void create_contract_account_filter(xjson_proc_t& json_proc);
    // void deploy_contract_filter(xjson_proc_t& json_proc);
    // void exec_contract_filter(xjson_proc_t& json_proc);
    // void global_op_filter(xjson_proc_t& json_proc);
    void get_property_filter(xjson_proc_t & json_proc);
    void sendTransaction_filter(xjson_proc_t & json_proc);

private:
    unordered_map<pair<string, string>, filter_handler> m_filter_map;
};

NS_END2
