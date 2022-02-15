// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xrule_manager.h"

#include "xerror/xrpc_error.h"
#include "xrpc_method.h"
#include "xuint_format.h"
#include "xverifier/xverifier_utl.h"

NS_BEG2(top, xrpc)

#define REGISTER_V1_FILTER(func_name)                                                                                                                                              \
    m_filter_map.emplace(pair<pair<string, string>, filter_handler>{pair<string, string>{"1.0", #func_name}, std::bind(&xfilter_manager::func_name##_filter, this, _1)})

#define CONDTION_FAIL_THROW(condition, error_code, error_msg)                                                                                                                      \
    try {                                                                                                                                                                          \
        if (!(condition)) {                                                                                                                                                        \
            throw xrpc_error{error_code, error_msg};                                                                                                                               \
        }                                                                                                                                                                          \
    } catch (const xrpc_error & e) {                                                                                                                                               \
        throw e;                                                                                                                                                                   \
    } catch (const std::exception & e) {                                                                                                                                           \
        throw xrpc_error{error_code, error_msg};                                                                                                                                   \
    }

#define CONDTION_FAIL_THROW_NO_EXCEPTION(condition, error_code, error_msg)                                                                                                         \
    if (!(condition)) {                                                                                                                                                            \
        throw xrpc_error{error_code, error_msg};                                                                                                                                   \
    }

xfilter_manager::xfilter_manager() {
    REGISTER_V1_FILTER(getAccount);
    REGISTER_V1_FILTER(getTransaction);
    // REGISTER_V1_FILTER(create_account);
    // REGISTER_V1_FILTER(transfer);
    // REGISTER_V1_FILTER(create_contract_account);
    // REGISTER_V1_FILTER(deploy_contract);
    // REGISTER_V1_FILTER(exec_contract);
    REGISTER_V1_FILTER(get_property);
    // REGISTER_V1_FILTER(global_op);
    REGISTER_V1_FILTER(sendTransaction);
}

void xfilter_manager::filter(xjson_proc_t & json_proc) {
    CONDTION_FAIL_THROW(json_proc.m_request_json["method"].isString() && !json_proc.m_request_json["method"].asString().empty(),
                        enum_xrpc_error_code::rpc_param_param_lack,
                        "miss param method or method is empty");
    CONDTION_FAIL_THROW(json_proc.m_request_json["version"].isString(),
                        enum_xrpc_error_code::rpc_param_param_lack,
                        "miss param version");

    auto iter = m_filter_map.find(pair<string, string>{json_proc.m_request_json["version"].asString(), json_proc.m_request_json["method"].asString()});
    if (iter != m_filter_map.end()) {
        iter->second(json_proc);
    }
}

void xfilter_manager::getAccount_filter(xjson_proc_t & json_proc) {
    auto account = json_proc.m_request_json["params"]["account_addr"];
    auto is_valid_account =
        account.isString() && !account.asString().empty() && (xverifier::xtx_utl::address_is_valid(account.asString()) == xverifier::xverifier_error::xverifier_success);
    CONDTION_FAIL_THROW(is_valid_account, enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");
}

void xfilter_manager::getTransaction_filter(xjson_proc_t & json_proc) {
    auto account = json_proc.m_request_json["params"]["account_addr"];
    auto is_valid_account =
        account.isString() && !account.asString().empty() && (xverifier::xtx_utl::address_is_valid(account.asString()) == xverifier::xverifier_error::xverifier_success);
    CONDTION_FAIL_THROW(is_valid_account, enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");
}

// void xfilter_manager::create_account_filter(xjson_proc_t& json_proc)
// {
//     auto account = json_proc.m_request_json["params"]["account"];
//     CONDTION_FAIL_THROW(account.isString() && !account.asString().empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not
//     valid");
// }

// void xfilter_manager::transfer_filter(xjson_proc_t& json_proc)
// {
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["from"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params from or from is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["to"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params to or to is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["amount"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params amount or amount is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["last_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_hash or last_hash is not
//     valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["nonce"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params nonce or nonce is not
//     valid");

//     const string& from = json_proc.m_request_json["params"]["from"].asString();
//     const string& to = json_proc.m_request_json["params"]["to"].asString();
//     CONDTION_FAIL_THROW(!from.empty() && !to.empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");

//     CONDTION_FAIL_THROW_NO_EXCEPTION(from != to, enum_xrpc_error_code::rpc_param_param_error, "from to can't the same");
//     CONDTION_FAIL_THROW_NO_EXCEPTION(json_proc.m_request_json["params"]["amount"].asUInt64() != 0, enum_xrpc_error_code::rpc_param_param_error, "amount can't be zero");
//     CONDTION_FAIL_THROW_NO_EXCEPTION(is_valid_hex(json_proc.m_request_json["params"]["last_hash"].asString()), enum_xrpc_error_code::rpc_param_param_error, "last_hash is not
//     valid");
// }

// void xfilter_manager::create_contract_account_filter(xjson_proc_t& json_proc)
// {
//     auto account = json_proc.m_request_json["params"]["account"];
//     CONDTION_FAIL_THROW(account.isString() && !account.asString().empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not
//     valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["last_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_hash or
//     last_hash is not valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["nonce"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params nonce or
//     nonce is not valid");

//     CONDTION_FAIL_THROW_NO_EXCEPTION(is_valid_hex(json_proc.m_request_json["params"]["last_hash"].asString()), enum_xrpc_error_code::rpc_param_param_error, "last_hash is not
//     valid");
// }

// void xfilter_manager::deploy_contract_filter(xjson_proc_t& json_proc)
// {
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["from"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params from or from is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["to"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params to or to is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["data"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params data or data is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["last_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_hash or last_hash is not
//     valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["nonce"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params nonce or nonce is not
//     valid");

//     const string& from = json_proc.m_request_json["params"]["from"].asString();
//     const string& to = json_proc.m_request_json["params"]["to"].asString();
//     CONDTION_FAIL_THROW(!from.empty() && !to.empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");

//     CONDTION_FAIL_THROW_NO_EXCEPTION(from != to, enum_xrpc_error_code::rpc_param_param_error, "from to can't the same");
//     CONDTION_FAIL_THROW_NO_EXCEPTION(is_valid_hex(json_proc.m_request_json["params"]["last_hash"].asString()), enum_xrpc_error_code::rpc_param_param_error, "last_hash is not
//     valid");
// }

// void xfilter_manager::exec_contract_filter(xjson_proc_t& json_proc)
// {
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["from"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params from or from is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["to"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params to or to is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["func"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params func or func is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["amount"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params amount or amount is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["data"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params data or data is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["last_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_hash or last_hash is not
//     valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["nonce"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params nonce or nonce is not
//     valid");

//     const string& from = json_proc.m_request_json["params"]["from"].asString();
//     const string& to = json_proc.m_request_json["params"]["to"].asString();
//     CONDTION_FAIL_THROW(!from.empty() && !to.empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");

//     CONDTION_FAIL_THROW_NO_EXCEPTION(from != to, enum_xrpc_error_code::rpc_param_param_error, "from to can't the same");
//     CONDTION_FAIL_THROW_NO_EXCEPTION(is_valid_hex(json_proc.m_request_json["params"]["last_hash"].asString()), enum_xrpc_error_code::rpc_param_param_error, "last_hash is not
//     valid");
// }

void xfilter_manager::get_property_filter(xjson_proc_t & json_proc) {
    auto account = json_proc.m_request_json["params"]["account_addr"];
    CONDTION_FAIL_THROW(
        account.isString() && !account.asString().empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");
    CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["type"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_hash or last_hash is not valid");
    const string & type = json_proc.m_request_json["params"]["type"].asString();
    CONDTION_FAIL_THROW(((json_proc.m_request_json["params"]["data"].isString() && (type == RPC_PROPERTY_STRING || type == RPC_PROPERTY_LIST)) ||
                         (json_proc.m_request_json["params"]["data"].isArray() && type == RPC_PROPERTY_MAP && json_proc.m_request_json["params"]["data"].size() == 2)),
                        enum_xrpc_error_code::rpc_param_param_lack,
                        "miss param params data or data is not valid");
}

// void xfilter_manager::global_op_filter(xjson_proc_t& json_proc) {
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["from"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params from or from is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["to"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params to or to is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["op_key"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params op_key or op_key is not valid");
//     CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["op_value"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params op_value or op_value is not
//     valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["last_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_hash or
//     last_hash is not valid"); CONDTION_FAIL_THROW(json_proc.m_request_json["params"]["nonce"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params nonce or
//     nonce is not valid");

//     const string& from = json_proc.m_request_json["params"]["from"].asString();
//     const string& to = json_proc.m_request_json["params"]["to"].asString();
//     CONDTION_FAIL_THROW(!from.empty() && !to.empty(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params account_addr or account_addr is not valid");

//     CONDTION_FAIL_THROW_NO_EXCEPTION(from != to, enum_xrpc_error_code::rpc_param_param_error, "from to can't the same");
//     CONDTION_FAIL_THROW_NO_EXCEPTION(is_valid_hex(json_proc.m_request_json["params"]["last_hash"].asString()), enum_xrpc_error_code::rpc_param_param_error, "last_hash is not
//     valid");
// }

void xfilter_manager::sendTransaction_filter(xjson_proc_t & json_proc) {
    CONDTION_FAIL_THROW(json_proc.m_request_json["params"].isObject(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params");
    auto & params = json_proc.m_request_json["params"];

    CONDTION_FAIL_THROW(
        params["tx_structure_version"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_structure_version or tx_structure_version is not valid");
    auto version = params["tx_structure_version"].asUInt();
    CONDTION_FAIL_THROW((version == data::xtransaction_version_1) || (version == data::xtransaction_version_2), enum_xrpc_error_code::rpc_param_param_error, "tx_structure_version invalid");
    if (version == data::xtransaction_version_2) {
        CONDTION_FAIL_THROW(params["amount"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params amount or amount is not valid");
        CONDTION_FAIL_THROW(params["sender_account"].isString() && !params["sender_account"].asString().empty(),
                            enum_xrpc_error_code::rpc_param_param_lack,
                            "miss param sender_account");
        CONDTION_FAIL_THROW(params["sender_action_name"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action_name");
        CONDTION_FAIL_THROW(params["sender_action_param"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action_param");
        CONDTION_FAIL_THROW(params["receiver_account"].isString() && !params["receiver_account"].asString().empty(),
                            enum_xrpc_error_code::rpc_param_param_lack,
                            "miss param receiver_account");
        CONDTION_FAIL_THROW(params["receiver_action_name"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action_name");
        CONDTION_FAIL_THROW(params["receiver_action_param"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action_param");

        std::string src_addr = params["sender_account"].asString();
        std::string dst_addr = params["receiver_account"].asString();
        CONDTION_FAIL_THROW(top::xverifier::xverifier_error::xverifier_success == xverifier::xtx_utl::address_is_valid(src_addr, true), enum_xrpc_error_code::rpc_param_param_error, "sender_account invalid");
        CONDTION_FAIL_THROW(top::xverifier::xverifier_error::xverifier_success == xverifier::xtx_utl::address_is_valid(dst_addr, true), enum_xrpc_error_code::rpc_param_param_error, "receiver_account invalid");
    
        CONDTION_FAIL_THROW(params["edge_nodeid"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params edge_nodeid or edge_nodeid is not valid");
        CONDTION_FAIL_THROW(params["token_name"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params token_name or token_name is not valid");
    } else {
        CONDTION_FAIL_THROW(params["to_ledger_id"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params to_ledger_id or to_ledger_id is not valid");
        CONDTION_FAIL_THROW(params["from_ledger_id"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params from_ledger_id or from_ledger_id is not valid");
        CONDTION_FAIL_THROW(params["tx_len"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_len or tx_len is not valid");
        CONDTION_FAIL_THROW(params["tx_random_nonce"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_random_nonce or tx_random_nonce is not valid");
        CONDTION_FAIL_THROW(params["last_tx_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_tx_hash or last_tx_hash is not valid");
        CONDTION_FAIL_THROW(params["challenge_proof"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params challenge_proof or challenge_proof is not valid");
        CONDTION_FAIL_THROW(params["tx_hash"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_hash");

        CONDTION_FAIL_THROW(params["sender_action"].isObject(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action");
        CONDTION_FAIL_THROW(params["receiver_action"].isObject(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action");
        auto & source_action = params["sender_action"];
        auto & target_action = params["receiver_action"];

        CONDTION_FAIL_THROW(source_action["action_hash"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_hash");
        CONDTION_FAIL_THROW(source_action["action_type"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_type");
        CONDTION_FAIL_THROW(source_action["action_size"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_size");
        CONDTION_FAIL_THROW(source_action["tx_sender_account_addr"].isString() && !source_action["tx_sender_account_addr"].asString().empty(),
                            enum_xrpc_error_code::rpc_param_param_lack,
                            "miss param sender_action tx_sender_account_addr");
        CONDTION_FAIL_THROW(source_action["action_name"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_name");
        CONDTION_FAIL_THROW(source_action["action_param"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_param");
        CONDTION_FAIL_THROW(source_action["action_authorization"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_authorization");
        CONDTION_FAIL_THROW(source_action["action_ext"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param sender_action action_ext");

        CONDTION_FAIL_THROW(target_action["action_hash"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_hash");
        CONDTION_FAIL_THROW(target_action["action_type"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_type");
        CONDTION_FAIL_THROW(target_action["action_size"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_size");
        CONDTION_FAIL_THROW(target_action["tx_receiver_account_addr"].isString() && !target_action["tx_receiver_account_addr"].asString().empty(),
                            enum_xrpc_error_code::rpc_param_param_lack,
                            "miss param receiver_action tx_receiver_account_addr");
        CONDTION_FAIL_THROW(target_action["action_name"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_name");
        CONDTION_FAIL_THROW(target_action["action_param"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_param");
        CONDTION_FAIL_THROW(target_action["action_authorization"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_authorization");
        CONDTION_FAIL_THROW(target_action["action_ext"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param receiver_action action_ext");

        std::string src_addr = source_action["tx_sender_account_addr"].asString();
        std::string dst_addr = target_action["tx_receiver_account_addr"].asString();
        CONDTION_FAIL_THROW(top::xverifier::xverifier_error::xverifier_success == xverifier::xtx_utl::address_is_valid(src_addr), enum_xrpc_error_code::rpc_param_param_error, "tx_sender_account_addr invalid");
        CONDTION_FAIL_THROW(top::xverifier::xverifier_error::xverifier_success == xverifier::xtx_utl::address_is_valid(dst_addr), enum_xrpc_error_code::rpc_param_param_error, "tx_receiver_account_addr invalid");    
    }

    CONDTION_FAIL_THROW(params["premium_price"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params premium_price or premium_price is not valid");
    CONDTION_FAIL_THROW(params["tx_deposit"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_deposit or tx_deposit is not valid");
    CONDTION_FAIL_THROW(params["tx_type"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_type or tx_type is not valid");
    CONDTION_FAIL_THROW(
        params["tx_expire_duration"].isUInt(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params tx_expire_duration or tx_expire_duration is not valid");
    CONDTION_FAIL_THROW(params["send_timestamp"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params send_timestamp or send_timestamp is not valid");
    CONDTION_FAIL_THROW(params["last_tx_nonce"].isUInt64(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params last_tx_nonce or last_tx_nonce is not valid");
    CONDTION_FAIL_THROW(params["note"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params note or note is not valid");
    CONDTION_FAIL_THROW(params["ext"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params ext or ext is not valid");

    CONDTION_FAIL_THROW(params["authorization"].isString(), enum_xrpc_error_code::rpc_param_param_lack, "miss param params authorization");
}

NS_END2
