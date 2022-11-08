// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xverifier/xwhitelist_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

namespace top {
    namespace xverifier {

        bool xwhitelist_utl::is_white_address(std::string const& source_addr) {
            if (false == is_whitelist_enable()) {
                return false;
            }

            auto write_addrs = whitelist_config();

            if (!write_addrs.empty()) {
                if (std::find(write_addrs.begin(), write_addrs.end(), source_addr) != std::end(write_addrs)) {
                    return true;
                }
            }
            return false;
        }

        bool xwhitelist_utl::is_whitelist_enable() {
            if (XGET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist)) {
                return true;
            }
            if (XGET_CONFIG(local_toggle_whitelist)) {
                return true;
            }
            return false;
        }

        void xwhitelist_utl::parse_config_data(std::string const& data, std::set<std::string> & ret_addrs) {
            std::vector<std::string> vec;
            if (!data.empty()) {
                base::xstring_utl::split_string(data, ',', vec);
                for (auto const& v: vec) {
                    ret_addrs.insert(v);
                }
            }
        }

        std::set<std::string> xwhitelist_utl::whitelist_config() {
            // TODO(jimmy) local config should 
            auto offchain_config = XGET_CONFIG(local_whitelist);
            auto onchain_config = XGET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);

            std::set<std::string> local_wl;
            parse_config_data(offchain_config, local_wl);
            parse_config_data(onchain_config, local_wl);

            return local_wl;
        }


    }  // namespace xverifier
}  // namespace top
