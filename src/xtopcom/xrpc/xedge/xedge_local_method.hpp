// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "xedge_local_method.h"
#include "xrpc/xuint_format.h"
#include "xrpc/xrpc_method.h"

NS_BEG2(top, xrpc)
#define MAX_EDGE_POOL_SIZE 10
#define EDGE_REGISTER_V1_LOCAL_METHOD(T, func_name) m_edge_local_method_map.emplace(pair<pair<string, string>, local_method_handler> \
{pair<string, string>{"1.0", #func_name}, std::bind(&xedge_local_method<T>::func_name##_method, this, _1)} )

template<class T>
unordered_map<string, vector<string>> xedge_local_method<T>::m_sub_account_map;
template<class T>
std::mutex xedge_local_method<T>::m_mutex;

template<class T>
xedge_local_method<T>::xedge_local_method(observer_ptr<elect::ElectMain> elect_main, common::xip2_t xip2):
m_elect_main(elect_main),
m_xip2(xip2) {
    // EDGE_REGISTER_V1_LOCAL_METHOD(T, account);
    // EDGE_REGISTER_V1_LOCAL_METHOD(T, sign_transaction);
    // EDGE_REGISTER_V1_LOCAL_METHOD(T, import_private_key);
    // EDGE_REGISTER_V1_LOCAL_METHOD(T, get_private_keys);
    EDGE_REGISTER_V1_LOCAL_METHOD(T, get_edge_neighbors);
}

template<class T>
void xedge_local_method<T>::reset_edge_local_method(common::xip2_t xip2){
    std::lock_guard<std::mutex> lock(m_mutex);
    m_xip2 = xip2;
}

template<class T>
bool xedge_local_method<T>::do_local_method(pair<string, string>& version_method, xjson_proc_t &json_proc) {
    auto iter = m_edge_local_method_map.find(version_method);
    if (iter == m_edge_local_method_map.end()) {
        return false;
    }
    try {
        iter->second(json_proc);
        json_proc.m_response_json[RPC_ERRNO] = RPC_OK_CODE;
        json_proc.m_response_json[RPC_ERRMSG] = RPC_OK_MSG;
    } catch (const xrpc_error &e) {
        throw e;
    } catch (const std::exception &e) {
        throw xrpc_error{enum_xrpc_error_code::rpc_param_param_error, "rpc param error"};
    }
    return true;
}

// template<class T>
// void xedge_local_method<T>::sign_transaction_method(xjson_proc_t &json_proc) {
//     json_proc.m_tx_ptr = make_object_ptr<data::xtransaction_t>();
//     auto& tx = json_proc.m_tx_ptr;
//     auto& request = json_proc.m_request_json["params"];
//     tx->set_tx_type(request["type"].asUInt());
//     tx->set_fire_timestamp(request["timestamp"].asUInt64());
//     tx->set_expire_duration(request["expire"].asUInt());
//     tx->set_last_hash(request["last_hash"].asUInt64());
//     tx->set_last_nonce(request["nonce"].asUInt64());
//     // vector<uint8_t> signature = hex_to_uint(request["signature"].asString());
//     // string signature_str((char*)signature.data(), signature.size()); // hex_to_uint client send xstream_t data 0xaaaa => string
//     // tx->set_authorization(std::move(signature_str));
//     auto& source_action = tx->get_source_action();
//     source_action.set_action_type(request["sender_action"]["type"].asUInt());
//     source_action.set_account_addr(request["sender_action"]["addr"].asString());
//     source_action.set_action_name(request["sender_action"]["name"].asString());
//     source_action.set_action_param(request["sender_action"]["param"].asString());
//     auto& target_action = tx->get_target_action();
//     target_action.set_action_type(request["receiver_action"]["type"].asUInt());
//     target_action.set_account_addr(request["receiver_action"]["addr"].asString());
//     target_action.set_action_name(request["receiver_action"]["name"].asString());
//     target_action.set_action_param(request["receiver_action"]["param"].asString());

//     tx->set_digest();
//     tx->set_len();
//     //find private key
//     std::lock_guard<std::mutex> lock(xedge_local_method<T>::m_mutex);
//     auto iter = m_account_key_map.find(source_action.get_account_addr());
//     if (iter == m_account_key_map.end()) {
//         throw xrpc_error{enum_xrpc_error_code::rpc_param_param_lack, "no private_key find for " + source_action.get_account_addr()};
//     }
//     utl::xecdsasig_t signature = iter->second.sign(tx->digest());
//     xJson::Value dataJson;
//     dataJson["hash"] = uint_to_str(tx->digest().data(), tx->digest().size());
//     dataJson["signature"] = uint_to_str(signature.get_compact_signature(), signature.get_compact_signature_size());
//     json_proc.m_response_json["data"] = dataJson;
// }

// template<class T>
// void xedge_local_method<T>::account_method(xjson_proc_t &json_proc) {
//     auto& request = json_proc.m_request_json["params"];
//     uint16_t network_id = static_cast<uint16_t>(request["network_id"].asUInt());
//     uint8_t addr_type   = static_cast<uint8_t>(request["address_type"].asUInt());
//     utl::xecprikey_t pri_key_obj;
//     utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
//     xJson::Value dataJson;
//     std::string account_addr;
//     dataJson["private_key"] = uint_to_str(pri_key_obj.data(), pri_key_obj.size());
//     if (request.isMember("parent_account")) {
//         const auto& parent_account = request["parent_account"].asString();
//         account_addr = pub_key_obj.to_address(parent_account, addr_type, network_id);
//         std::lock_guard<std::mutex> lock(xedge_local_method<T>::m_mutex);
//         auto iter = m_sub_account_map.find(account_addr);
//         if (iter != m_sub_account_map.end()) {
//             iter->second.emplace_back(account_addr);
//         } else {
//             vector<string> account_list;
//             account_list.emplace_back(account_addr);
//             m_sub_account_map.emplace(parent_account, account_list);
//         }
//         m_account_key_map.emplace(account_addr, pri_key_obj);
//     } else {
//         account_addr = pub_key_obj.to_address(addr_type, network_id);
//         std::lock_guard<std::mutex> lock(xedge_local_method<T>::m_mutex);
//         m_account_key_map.emplace(account_addr, pri_key_obj);
//     }
//     dataJson["account"]     = account_addr;
//     json_proc.m_response_json["data"] = dataJson;

// }

// template<class T>
// void xedge_local_method<T>::import_private_key_method(xjson_proc_t &json_proc) {
//     auto& request = json_proc.m_request_json["params"];
//     uint256_t private_key = hex_to_uint256(request["private_key"].asString());
//     uint16_t network_id = static_cast<uint16_t>(request["network_id"].asUInt());
//     uint8_t addr_type   = static_cast<uint8_t>(request["address_type"].asUInt());
//     utl::xecprikey_t pri_key_obj(private_key);
//     std::lock_guard<std::mutex> lock(xedge_local_method<T>::m_mutex);
//     m_account_key_map[pri_key_obj.get_public_key().to_address(addr_type, network_id)] = pri_key_obj;
// }

// template<class T>
// void xedge_local_method<T>::get_private_keys_method(xjson_proc_t &json_proc) {
//     std::lock_guard<std::mutex> lock(xedge_local_method<T>::m_mutex);
//     xJson::Value dataJson;
//     for (auto& kv: m_account_key_map) {
//         dataJson[kv.first] = uint_to_str(kv.second.data(), kv.second.size());
//     }
//     json_proc.m_response_json["data"] = dataJson;
// }

template<class T>
void xedge_local_method<T>::get_edge_neighbors_method(xjson_proc_t &json_proc) {
    xJson::Value dataJson;
    xJson::Value::Members ips;
    {
        std::lock_guard<std::mutex> lock(xedge_local_method<T>::m_mutex);
        ips = m_elect_main->GetServiceNeighbours(m_xip2);
    }
    // ips maximum size is 255, random_shuffle is good enough
    std::random_shuffle(ips.begin(), ips.end());
    auto pool_size = (ips.size() < MAX_EDGE_POOL_SIZE) ? ips.size() : MAX_EDGE_POOL_SIZE;
    for(size_t i = 0; i < pool_size; ++i){
        dataJson.append(ips[i]);
    }
    xdbg("edge_neighbors size: %d, pool size: %d ", ips.size(), pool_size);
    json_proc.m_response_json["data"] = dataJson;
}

NS_END2
