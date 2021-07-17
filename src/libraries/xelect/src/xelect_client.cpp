// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#define USE_STANDALONE_ASIO
#endif

#include "xelect/client/xelect_client.h"

#include "generated/version.h"
#include "simplewebserver/client_http.hpp"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xchain_param.h"
#include "xdata/xgenesis_data.h"
#include "xelect_common/elect_option.h"
#include "xmbus/xevent_store.h"
#include "xpbase/base/top_log.h"
#include "xrpc/xerror/xrpc_error_code.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"
#include "xpbase/base/top_utils.h"
#include "xtopcl/include/api_method.h"

#include <cinttypes>

using namespace top::data;

NS_BEG2(top, elect)

using namespace base;           // NOLINT
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

void xelect_client_imp::bootstrap_node_join() {
    auto & config_register = top::config::xconfig_register_t::get_instance();
    xplatform_params & platform_params = xplatform_params::get_instance();

    xuser_params & user_params = xuser_params::get_instance();

    auto const http_port_ui = XGET_CONFIG(http_port);

    std::string http_port = std::to_string(http_port_ui);

    std::set<std::string> seed_edge_host_set;
    platform_params.get_seed_edge_host(seed_edge_host_set);
    if (seed_edge_host_set.empty()) {
        xerror("can't get seed edge host");
        throw std::runtime_error("can't get seed edge host");
    }
    bool send_success{false};
    size_t try_count{60};
    size_t loop_count{0};
    size_t success_count{0};
    xinfo("enter bootstrap_node_join");
    for (auto i = 0u; i < try_count; ++i) {
        for (auto& item : seed_edge_host_set) {
            std::string seed_edge_host = item + ":" + http_port;
            HttpClient client(seed_edge_host);
            xdbg("boostrap to %s", seed_edge_host.c_str());


            try {
                std::string token_request = "version=1.0&target_account_addr=" + user_params.account.value() + "&method=requestToken&sequence_id=1";
                xdbg("token_request:%s", token_request.c_str());
                auto token_response = client.request("POST", "/", token_request);
                const auto& token_response_str = token_response->content.string();
                xdbg("token_response:%s", token_response_str.c_str());
                xJson::Reader reader;
                xJson::Value token_response_json;
                xdbg("json parse: %d", reader.parse(token_response_str, token_response_json));
                if (!reader.parse(token_response_str, token_response_json) || token_response_json[xrpc::RPC_ERRNO].asUInt() != xrpc::RPC_OK_CODE) {
                    xerror("token_response error");
                    continue;
                }
                std::string token = token_response_json["data"][xrpc::RPC_TOKEN].asString();

                //get last hash and nonce
                std::string account_info_request = "version=1.0&target_account_addr=" + user_params.account.value() + "&method=getAccount&sequence_id=2&identity_token=" + token
                    + "&body=" + SimpleWeb::Percent::encode("{\"params\": {\"account_addr\": \"" + user_params.account.value() + "\"}}");
                xdbg("account_info_request:%s", account_info_request.c_str());
                auto account_info_response = client.request("POST", "/", account_info_request);
                const auto& account_info_response_str = account_info_response->content.string();
                xdbg("account_info_response:%s", account_info_response_str.c_str());


                xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
                top::base::xstream_t param_stream(base::xcontext_t::instance());
                param_stream << user_params.account;
                param_stream << common::xnetwork_id_t{ static_cast<common::xnetwork_id_t::value_type>(top::config::to_chainid(XGET_CONFIG(chain_name))) };
#if defined XENABLE_MOCK_ZEC_STAKE
                ENUM_SERIALIZE(param_stream, user_params.node_role_type);
                param_stream << user_params.publickey;
                param_stream << static_cast<uint64_t>(top::config::to_chainid(XGET_CONFIG(chain_name)));
#endif
                param_stream << PROGRAM_VERSION;
                std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
                tx->make_tx_run_contract("nodeJoinNetwork2", param);
                tx->set_different_source_target_address(user_params.account.value(), sys_contract_rec_standby_pool_addr);
                tx->set_fire_and_expire_time(600);
                tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));

                xJson::Value account_info_response_json;
                if (!reader.parse(account_info_response_str, account_info_response_json) || account_info_response_json[xrpc::RPC_ERRNO].asInt() != xrpc::RPC_OK_CODE) {
                    xwarn("account_info_response_json error");
                    tx->set_last_trans_hash_and_nonce({}, 0);
                } else {
                    tx->set_last_nonce(account_info_response_json["data"]["nonce"].asUInt64());
                    std::string last_trans_hash = account_info_response_json["data"]["latest_tx_hash_xxhash64"].asString();
                    tx->set_last_hash(xrpc::hex_to_uint64(last_trans_hash));
                }
                tx->set_digest();

                // get private key and sign
                std::string sign_key;
                // xinfo("xelect_client_imp::bootstrap_node_join,user_params.signkey: %s", user_params.signkey.c_str());
                sign_key = DecodePrivateString(user_params.signkey);    
                utl::xecprikey_t pri_key_obj((uint8_t*)sign_key.data());
                utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
                auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
                tx->set_signature(signature);
                tx->set_len();

                std::string send_tx_request = "version=1.0&target_account_addr=" + user_params.account.value() + "&method=sendTransaction&sequence_id=3&token=" + token;
                xJson::FastWriter writer;
                xJson::Value tx_json;
                tx_to_json(tx_json, tx);
                xdbg("tx_json: %s", writer.write(tx_json).c_str());
                send_tx_request += "&body=" + SimpleWeb::Percent::encode(writer.write(tx_json));
                xdbg("send_tx_request: %s", send_tx_request.c_str());

                auto send_tx_response = client.request("POST", "/", send_tx_request);
                xkinfo("send_tx_response: %s", send_tx_response->content.string().c_str());
                if (++success_count >= 3) {
                    break;
                }
            } catch(const SimpleWeb::system_error &e) {
                xwarn("Client request error: %s", e.what());
            } catch (const std::exception &e) {
                xerror("Client exception error: %s", e.what());
            }
        }
        if (success_count) {
            break;
        }
        sleep(10);
    }
    if (!success_count) {
        std::cout << "send join chain network transaction failed" << std::endl;
        xerror("send node join fail");
        throw std::runtime_error("send node join fail");
    } else {
        std::cout << "join chain network transaction ok" << std::endl;
        xinfo("join chain network transaction ok");
    }

}


void xelect_client_imp::tx_to_json(xJson::Value& tx_json, const xtransaction_ptr_t& tx) {
    if (tx == nullptr) {
        xerror("tx is nullptr");
        return;
    }
    // tx_json[xrpc::RPC_METHOD] = "send_transaction";
    // tx_json["version"] = "1.0";
    xJson::Value& result_json = tx_json["params"];

    result_json["tx_structure_version"] = tx->get_tx_version();
    result_json["tx_deposit"] = tx->get_deposit();
    result_json["to_ledger_id"] = tx->get_to_ledger_id();
    result_json["from_ledger_id"] = tx->get_from_ledger_id();
    result_json["tx_type"] = tx->get_tx_type();
    result_json["tx_len"] = tx->get_tx_len();
    result_json["tx_expire_duration"] = tx->get_expire_duration();
    result_json["send_timestamp"] = static_cast<xJson::UInt64>(tx->get_fire_timestamp());
    result_json["tx_random_nonce"] = tx->get_random_nonce();
    result_json["premium_price"] = tx->get_premium_price();
    result_json["last_tx_nonce"] = static_cast<xJson::UInt64>(tx->get_last_nonce());
    result_json["last_tx_hash"] = xrpc::uint64_to_str(tx->get_last_hash());
    result_json["challenge_proof"] = tx->get_challenge_proof();
    result_json["note"] = tx->get_memo();

    xJson::Value& s_action_json = tx_json["params"]["sender_action"];
    s_action_json["action_hash"] = tx->get_source_action().get_action_hash();
    s_action_json["action_type"] = tx->get_source_action().get_action_type();
    s_action_json["action_size"] = tx->get_source_action().get_action_size();
    s_action_json["tx_sender_account_addr"] = tx->get_source_action().get_account_addr();
    s_action_json["action_name"] = tx->get_source_action().get_action_name();
    s_action_json["action_param"] = xrpc::uint_to_str(tx->get_source_action().get_action_param().data(), tx->get_source_action().get_action_param().size());
    s_action_json["action_ext"] = xrpc::uint_to_str(tx->get_source_action().get_action_ext().data(), tx->get_source_action().get_action_ext().size());
    s_action_json["action_authorization"] = tx->get_source_action().get_action_authorization();

    xJson::Value& t_action_json = tx_json["params"]["receiver_action"];
    t_action_json["action_hash"] = tx->get_target_action().get_action_hash();
    t_action_json["action_type"] = tx->get_target_action().get_action_type();
    t_action_json["action_size"] = tx->get_target_action().get_action_size();
    t_action_json["tx_receiver_account_addr"] = tx->get_target_action().get_account_addr();
    t_action_json["action_name"] = tx->get_target_action().get_action_name();
    t_action_json["action_param"] = xrpc::uint_to_str(tx->get_target_action().get_action_param().data(), tx->get_target_action().get_action_param().size());
    t_action_json["action_ext"] = xrpc::uint_to_str(tx->get_target_action().get_action_ext().data(), tx->get_target_action().get_action_ext().size());
    t_action_json["action_authorization"] = tx->get_target_action().get_action_authorization();

    result_json["ext"] = xrpc::uint_to_str(tx->get_ext().data(), tx->get_ext().size());
    result_json["tx_hash"] = xrpc::uint_to_str(tx->digest().data(), tx->digest().size());
    result_json["authorization"] = "0x" + to_hex_str(tx->get_authorization());
    xdbg("authorization: %s", to_hex_str(tx->get_authorization()).c_str());
}


NS_END2
