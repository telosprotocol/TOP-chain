// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xverifier/xwhitelist_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

namespace top {
    namespace xverifier {

        bool xwhitelist_utl::is_white_address_limit(std::string const& source_addr) {
            // 1. if whitelist not enable, it should not be limited
            if (false == is_whitelist_enable()) {
                return false;
            }
            // 2. if address in whitelist, it should not be limited
            auto write_addrs = whitelist_config();
            if (!write_addrs.empty()) {
                if (std::find(write_addrs.begin(), write_addrs.end(), source_addr) != std::end(write_addrs)) {
                    return false;
                }
            }
            // 3. otherwise, it should be limited
            xdbg("xwhitelist_utl::is_white_address_limit not match white addrs. %zu, addr=%s", write_addrs.size(), source_addr.c_str());
            return true;
        }

        bool xwhitelist_utl::is_whitelist_enable() {
            if (XGET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist)) {
                xdbg("xwhitelist_utl::is_whitelist_enable toggle_whitelist enable");
                return true;
            }
            if (XGET_CONFIG(local_toggle_whitelist)) {
                xdbg("xwhitelist_utl::is_whitelist_enable local_toggle_whitelist enable");
                return true;
            }
            return false;
        }

        std::set<std::string> xwhitelist_utl::whitelist_config() {
            auto offchain_config = XGET_CONFIG(local_whitelist);
            auto onchain_config = XGET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);

            std::set<std::string> local_wl;
            xtx_utl::parse_bwlist_config_data(offchain_config, local_wl);
            xtx_utl::parse_bwlist_config_data(onchain_config, local_wl);

            return local_wl;
        }


    }  // namespace xverifier
}  // namespace top
