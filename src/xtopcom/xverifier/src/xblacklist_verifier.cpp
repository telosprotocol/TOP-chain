// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xverifier/xblacklist_verifier.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xverifier/xverifier_errors.h"
#include "xverifier/xverifier_utl.h"

NS_BEG2(top, xverifier)

bool xtop_blacklist_utl::is_black_address(std::string const& addr) {
    auto black_addrs = black_config();
    if (!black_addrs.empty()) {
        if (std::find(black_addrs.begin(), black_addrs.end(), addr) != std::end(black_addrs)) {
            return true;
        }
    }
    return false;
}
bool xtop_blacklist_utl::is_black_address(std::string const& tx_source_addr, std::string const& tx_target_addr) {
    auto black_addrs = black_config();
    if (!black_addrs.empty()) {
        if (std::find(black_addrs.begin(), black_addrs.end(), tx_source_addr) != std::end(black_addrs)) {
            return true;
        }
        if (std::find(black_addrs.begin(), black_addrs.end(), tx_target_addr) != std::end(black_addrs)) {
            return true;
        }
    }
    return false;
}

std::set<std::string>  xtop_blacklist_utl::black_config() {
    std::string offchain_config = XGET_CONFIG(local_blacklist);
    std::string onchain_config = XGET_ONCHAIN_GOVERNANCE_PARAMETER(blacklist);

    std::set<std::string> res;
    xtx_utl::parse_bwlist_config_data(offchain_config, res);
    xtx_utl::parse_bwlist_config_data(onchain_config, res);

    return res;
}

std::vector<std::string> xtop_blacklist_utl::refresh_and_get_new_addrs() {
    std::vector<std::string> new_addrs;
    std::set<std::string> config_addrs = black_config();
    if (config_addrs != m_last_black_addrs) {
        for (auto & v : config_addrs) {
            if (std::find(m_last_black_addrs.begin(), m_last_black_addrs.end(), v) == std::end(m_last_black_addrs)) {
                new_addrs.push_back(v);
            }
        }

        m_last_black_addrs = config_addrs;
    }
    return new_addrs;
}


NS_END2

