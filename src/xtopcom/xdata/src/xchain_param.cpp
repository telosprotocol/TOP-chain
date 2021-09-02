// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xchain_param.h"

#include "xconfig/xconfig_register.h"
#include "xpbase/base/line_parser.h"

namespace top {

namespace data {

xuser_params::xuser_params() {
    // xuser_params instance should init after config_register load offchain config.
    // or it can not get the node_id/public_key/signkey...
    auto & config_register = top::config::xconfig_register_t::get_instance();

    std::string node_id;
    if (!config_register.get("node_id", node_id)) {
        assert(0);
    }
    account = common::xnode_id_t{node_id};

    std::string public_key;
    if (!config_register.get("public_key", public_key)) {
        assert(0);
    }
    publickey = public_key;  // publickey = base::xstring_utl::base64_decode(public_key);

    std::string sign_key;
    if (!config_register.get("sign_key", sign_key)) {
        assert(0);
    }
    signkey = sign_key;  // signkey = base::xstring_utl::base64_decode(sign_key);

#if defined XENABLE_MOCK_ZEC_STAKE
    std::string role_type;
    if (!config_register.get("node_type", role_type)) {
        assert(0);
    }
    node_role_type = common::to_role_type(role_type);
    if (node_role_type == common::xrole_type_t::invalid) {
        xerror("[user config] node_type invalid");
        throw std::logic_error{"node_type invalid"};
    }
#endif
}

bool xuser_params::is_valid() {

    if (!top::xverifier::xtx_utl::address_is_valid(account) || !top::xverifier::xtx_utl::privkey_pubkey_is_match(signkey, publickey)) {
        xwarn("[global_trace][xuser_params][fail], xuser params config check fail!");
        return false;
    }

    xdbg("[global_trace][xuser_params][success], xuser params config check success!");
    return true;
}

#if 0
xstaticec_params::xstaticec_params()
        // : total_working_advance_nodes(4)
        // , total_working_consensus_nodes(5)
        // , current_working_consensus_nodes(3)
{
    // total_working_advance_nodes = 4;
    // total_working_consensus_nodes = 6;
    // current_working_consensus_nodes = 3;
    // round_consensus_nodes = 1;
    // total_working_edge_nodes = 10;
}
#endif

xplatform_params::xplatform_params() {
    local_ip = XGET_CONFIG(ip);
    local_port = XGET_CONFIG(platform_business_port);
    first_node = XGET_CONFIG(platform_first_node);
    public_endpoints = XGET_CONFIG(platform_public_endpoints);
    url_endpoints = XGET_CONFIG(platform_url_endpoints);
    show_cmd = XGET_CONFIG(platform_show_cmd);
}

void xplatform_params::get_seed_edge_host(std::set<std::string>& seed_edge_host_set) {
    seed_edge_host_set = edge_endpoints;
    return;
}

void xplatform_params::set_seed_edge_host(const std::string endpoints) {
    base::LineParser line_split(endpoints.c_str(), ',', endpoints.size());
    for (uint32_t i = 0; i < line_split.Count(); ++i) {
        base::LineParser ip_port(line_split[i], ':');
        if (ip_port.Count() != 2) {
            xinfo("<blueshi> invalid endpoint: %s", line_split[i]);
            continue;
        }
        try {
            edge_endpoints.insert(ip_port[0]);
        } catch (...) {
            xwarn("<blueshi> invalid endpoint: %s", line_split[i]);
        }
    }
}


}  // namespace data

}  // namespace top
