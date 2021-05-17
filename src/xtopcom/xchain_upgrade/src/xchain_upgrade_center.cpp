// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xchain_upgrade/xchain_upgrade_center.h"

#include <string>

#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xchain_names.h"
#include "xchain_upgrade/xchain_upgrade_type.h"

namespace top {
    namespace chain_upgrade {

        xchain_fork_config_t  mainnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 3705480, "reward contract fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 3774608, "split reward transactions fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 3904920, "reward contract save reward detail"},
            xfork_point_t{xfork_point_type_t::logic_time, 3913560, "vote contract trx split fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 4000000, "rec standby update programe version"},
            xfork_point_t{xfork_point_type_t::logic_time, 5000000, "slash and workload contract upgrade"}
        };

        // !!!change!!! fork time for galileo
        xchain_fork_config_t  testnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 3705480, "reward contract fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 3774608, "split reward transactions fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 3904920, "reward contract save reward detail"},
            xfork_point_t{xfork_point_type_t::logic_time, 3913560, "vote contract trx split fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 4000000, "rec standby update programe version"},
            xfork_point_t{xfork_point_type_t::logic_time, 5000000, "slash and workload contract upgrade"}
        };

        // !!!change!!! fork time for local develop net
        xchain_fork_config_t default_chain_config {
            xfork_point_t{xfork_point_type_t::logic_time, 3705480, "reward contract fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 3774608, "split reward transactions fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 3904920, "reward contract save reward detail"},
            xfork_point_t{xfork_point_type_t::logic_time, 3913560, "vote contract trx split fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 4000000, "rec standby update programe version"},
            xfork_point_t{xfork_point_type_t::logic_time, 5000000, "slash and workload contract upgrade"}
        };

        xchain_fork_config_t const & xtop_chain_fork_config_center::chain_fork_config() noexcept {
            if (top::config::chain_name_mainnet == XGET_CONFIG(chain_name)) {
                return mainnet_chain_config;
            } else if (top::config::chain_name_testnet == XGET_CONFIG(chain_name)) {
                return testnet_chain_config;
            }

            return default_chain_config;
        }

        bool xtop_chain_fork_config_center::is_forked(top::optional<xfork_point_t> const& fork_point, uint64_t target) noexcept {
            if (!fork_point.has_value()) return false;
            return  target >= fork_point.value().point;
        }
    }
}
