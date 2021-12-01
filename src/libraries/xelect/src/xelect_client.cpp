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
#include "xdata/xtx_factory.h"
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
            xdbg("bootstrap to %s", seed_edge_host.c_str());

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

                uint64_t nonce = 0;
                uint64_t last_hash = 0;
                xJson::Value account_info_response_json;
                if (!reader.parse(account_info_response_str, account_info_response_json) || account_info_response_json[xrpc::RPC_ERRNO].asInt() != xrpc::RPC_OK_CODE) {
                    xwarn("account_info_response_json error");
                } else {
                    nonce = account_info_response_json["data"]["nonce"].asUInt64();
                    std::string last_trans_hash = account_info_response_json["data"]["latest_tx_hash_xxhash64"].asString();
                    last_hash = data::hex_to_uint64(last_trans_hash);
                }

                // get private key and sign
                // xinfo("xelect_client_imp::bootstrap_node_join,user_params.signkey: %s", user_params.signkey.c_str());
                std::string sign_key = DecodePrivateString(user_params.signkey);

                uint32_t deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
                xtransaction_ptr_t tx = xtx_factory::create_nodejoin_tx(user_params.account.value(), nonce, last_hash, param, deposit, sign_key);

                std::string send_tx_request = "version=1.0&target_account_addr=" + user_params.account.value() + "&method=sendTransaction&sequence_id=3&token=" + token;
                xJson::FastWriter writer;
                xJson::Value tx_json;
                if (tx->get_tx_version() == xtransaction_version_2) {
                    tx->parse_to_json(tx_json["params"], data::RPC_VERSION_V2);
                } else {
                    tx->parse_to_json(tx_json["params"], data::RPC_VERSION_V1);
                }

                tx_json["params"]["authorization"] = data::uint_to_str(tx->get_authorization().data(), tx->get_authorization().size());
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

NS_END2
