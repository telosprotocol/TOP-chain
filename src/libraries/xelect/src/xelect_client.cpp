// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelect/client/xelect_client.h"

#include "generated/version.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xchain_param.h"
#include "xdata/xtx_factory.h"
#include "xhttp/xhttp_client_base.h"
#include "xmbus/xevent_store.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xrpc/xerror/xrpc_error_code.h"
#include "xrpc/xrpc_method.h"
#include "xrpc/xuint_format.h"
#include "xsafebox/safebox_proxy.h"


using namespace top::data;

NS_BEG2(top, elect)

class BootstrapClient : public xhttp::xhttp_client_base_t {
private:
    using base_t = xhttp::xhttp_client_base_t;

public:
    BootstrapClient(std::string const & seed_edge_host) : base_t{seed_edge_host} {};

    std::string Request(std::string const & request) {
        return request_post_string("/", request);
    }
};

void xelect_client_imp::bootstrap_node_join() {
    // auto & config_register = top::config::xconfig_register_t::get_instance();
    xuser_params & user_params = xuser_params::get_instance();

    auto const http_port_ui = XGET_CONFIG(http_port);
    std::string http_port = std::to_string(http_port_ui);

    std::string p2p_endpoints = XGET_CONFIG(p2p_endpoints);
    xinfo("get p2p_endpoints: %s", p2p_endpoints.c_str());
    std::vector<std::string> endpoints_vec;
    std::set<std::string> ip_set;
    top::base::xstring_utl::split_string(p2p_endpoints, ',', endpoints_vec);
    for (auto ip_port : endpoints_vec) {
        std::vector<std::string> ip_port_vec;
        top::base::xstring_utl::split_string(ip_port, ':', ip_port_vec);
        if (ip_port_vec.size() != 2) {
            continue;
        }
        ip_set.insert(ip_port_vec[0]);
    }

    // bool send_success{false};
    size_t try_count{60};
    // size_t loop_count{0};
    size_t success_count{0};
    xinfo("enter bootstrap_node_join");
    for (auto i = 0u; i < try_count; ++i) {
        for (auto & item : ip_set) {
            std::string seed_edge_host = item + ":" + http_port;
            BootstrapClient client{seed_edge_host};
            xinfo("bootstrap_node_join try_count %zu bootstrap to %s", i, seed_edge_host.c_str());

            try {
                std::string token_request = "version=1.0&target_account_addr=" + user_params.account.to_string() + "&method=requestToken&sequence_id=1";
                xdbg("token_request:%s", token_request.c_str());
                auto token_response_str = client.Request(token_request);
                xinfo("bootstrap_node_join token_response:%s", token_response_str.c_str());
                Json::Reader reader;
                Json::Value token_response_json;
                xdbg("json parse: %d", reader.parse(token_response_str, token_response_json));
                if (!reader.parse(token_response_str, token_response_json) || token_response_json[xrpc::RPC_ERRNO].asUInt() != xrpc::RPC_OK_CODE) {
                    xerror("bootstrap_node_join token_response error");
                    continue;
                }
                std::string token = token_response_json["data"][xrpc::RPC_TOKEN].asString();

                // get standbys to check if node is aready in standby list
                std::string getstandbys_request = "version=1.0&target_account_addr=" + user_params.account.to_string() + "&method=getStandbys&sequence_id=2&identity_token=" + token +
                                                "&body=" + client.percent_encode("{\"params\": {\"node_account_addr\": \"" + user_params.account.to_string() + "\"}}");
                xdbg("getstandbys_request:%s", getstandbys_request.c_str());
                auto getstandbys_response = client.Request(getstandbys_request);
                xinfo("bootstrap_node_join getstandbys_response:%s", getstandbys_response.c_str());

                Json::Value getstandbys_response_json;
                if (!reader.parse(getstandbys_response, getstandbys_response_json) || getstandbys_response_json[xrpc::RPC_ERRNO].asInt() != xrpc::RPC_OK_CODE) {
                    xwarn("bootstrap_node_join getstandbys_response error");
                } else {
                    std::string target_node_id = getstandbys_response_json["data"]["node_id"].asString();
                    if (user_params.account.to_string() == target_node_id) {
                        xinfo("bootstrap_node_join node is in standby list, no need to join. node_id:%s", target_node_id.c_str());
                        return;
                    }
                    xinfo("bootstrap_node_join node is not in standby list, need to join. node_id:%s", target_node_id.c_str());
                }

                // get last hash and nonce
                std::string account_info_request = "version=1.0&target_account_addr=" + user_params.account.to_string() + "&method=getAccount&sequence_id=3&identity_token=" + token +
                                                   "&body=" + client.percent_encode("{\"params\": {\"account_addr\": \"" + user_params.account.to_string() + "\"}}");
                xdbg("account_info_request:%s", account_info_request.c_str());
                auto account_info_response_str = client.Request(account_info_request);
                xinfo("bootstrap_node_join account_info_response:%s", account_info_response_str.c_str());

                top::base::xstream_t param_stream(base::xcontext_t::instance());
                param_stream << user_params.account;
                param_stream << common::xnetwork_id_t{static_cast<common::xnetwork_id_t::value_type>(top::config::to_chainid(XGET_CONFIG(chain_name)))};
#if defined XENABLE_MOCK_ZEC_STAKE
                ENUM_SERIALIZE(param_stream, user_params.node_role_type);
                param_stream << user_params.publickey;
                param_stream << static_cast<uint64_t>(top::config::to_chainid(XGET_CONFIG(chain_name)));
#endif
                param_stream << PROGRAM_VERSION;
                std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());

                uint64_t nonce = 0;
                // uint64_t last_hash = 0;
                Json::Value account_info_response_json;
                if (!reader.parse(account_info_response_str, account_info_response_json) || account_info_response_json[xrpc::RPC_ERRNO].asInt() != xrpc::RPC_OK_CODE) {
                    xwarn("bootstrap_node_join account_info_response_json error");
                } else {
                    nonce = account_info_response_json["data"]["nonce"].asUInt64();
                    // std::string last_trans_hash = account_info_response_json["data"]["latest_tx_hash_xxhash64"].asString();
                    // last_hash = data::hex_to_uint64(last_trans_hash);
                }

                uint32_t deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
                xtransaction_ptr_t tx = xtx_factory::create_nodejoin_tx(user_params.account.to_string(), nonce, param, deposit);

                tx->set_authorization(safebox::xsafebox_proxy::get_instance().get_proxy_secp256_signature(base::xstring_utl::base64_decode(user_params.publickey), tx->digest()));
                tx->set_len();

                std::string send_tx_request = "version=1.0&target_account_addr=" + user_params.account.to_string() + "&method=sendTransaction&sequence_id=3&token=" + token;
                Json::FastWriter writer;
                Json::Value tx_json;
                if (tx->get_tx_version() == xtransaction_version_2) {
                    tx->parse_to_json(tx_json["params"], data::RPC_VERSION_V2);
                } else {
                    tx->parse_to_json(tx_json["params"], data::RPC_VERSION_V1);
                }

                tx_json["params"]["authorization"] = data::uint_to_str(tx->get_authorization().data(), tx->get_authorization().size());
                xdbg("tx_json: %s", writer.write(tx_json).c_str());
                send_tx_request += "&body=" + client.percent_encode(writer.write(tx_json));
                xdbg("send_tx_request: %s", send_tx_request.c_str());

                auto send_tx_response_str = client.Request(send_tx_request);
                xkinfo("bootstrap_node_join send_tx_response: %s", send_tx_response_str.c_str());
                if (++success_count >= 3) {
                    break;
                }
            } catch (const std::exception & e) {
                xerror("bootstrap_node_join Client exception error: %s", e.what());
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
