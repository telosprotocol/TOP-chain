// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xchain_param.h"
#include "xpbase/base/line_parser.h"

namespace top {

namespace data {

bool xuser_params::is_valid() {

    if (!top::xverifier::xtx_utl::address_is_valid(account) || !top::xverifier::xtx_utl::privkey_pubkey_is_match(signkey, publickey)) {
        xwarn("[global_trace][xuser_params][fail], xuser params config check fail!");
        return false;
    }

    xdbg("[global_trace][xuser_params][success], xuser params config check success!");
    return true;
}


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
