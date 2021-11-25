// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xchain_upgrade/xchain_upgrade_center.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xchain_names.h"

namespace top {
    namespace chain_upgrade {

        xchain_fork_config_t      xtop_chain_fork_config_center::m_fork_config;

#if defined(XCHAIN_FORKED_BY_DEFAULT)
        xchain_fork_config_t  mainnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point for table receipt"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "table statistic info fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "table receipt protocol fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fork point"},
        };

        // !!!change!!! fork time for galileo
        xchain_fork_config_t  testnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point for table receipt"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "table statistic info fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "table receipt protocol fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fork point"},
        };

        // !!!change!!! fork time for local develop net
        xchain_fork_config_t default_chain_config {
            xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point for table receipt"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "table statistic info fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "table receipt protocol fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fork point"},
        };
#else   // #if defined(XCHAIN_FORKED_BY_DEFAULT)
        xchain_fork_config_t  mainnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "block fork point for table receipt"},
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "table statistic info fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "table receipt protocol fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6456525, "tx v2 fork point"},
        };

        // !!!change!!! fork time for galileo
        xchain_fork_config_t  testnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "block fork point for table receipt"},
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "table statistic info fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "table receipt protocol fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6456525, "tx v2 fork point"},
        };

        // !!!change!!! fork time for local develop net
        xchain_fork_config_t default_chain_config {
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "block fork point for table receipt"},
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "table statistic info fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 10000000, "table receipt protocol fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6456525, "tx v2 fork point"},
        };
#endif  // #if defined(XCHAIN_FORKED_BY_DEFAULT)

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

        bool xtop_chain_fork_config_center::is_block_forked(uint64_t target) noexcept {
            xchain_fork_config_t const & _fork_config = xtop_chain_fork_config_center::get_chain_fork_config();
            return  xtop_chain_fork_config_center::is_forked(_fork_config.block_fork_point, target);
        }

        bool xtop_chain_fork_config_center::is_tx_forked(uint64_t fire_timestamp) noexcept {
            xchain_fork_config_t const & _fork_config = xtop_chain_fork_config_center::get_chain_fork_config();
            auto clock = _fork_config.tx_v2_fork_point.value().point;
            constexpr uint64_t TOP_BEGIN_GMTIME = 1573189200;
            auto clock_time_stamp = clock * 10 + TOP_BEGIN_GMTIME;
            return fire_timestamp >= clock_time_stamp;
        }

        void xtop_chain_fork_config_center::init() {
            if (top::config::chain_name_mainnet == XGET_CONFIG(chain_name)) {
                m_fork_config = mainnet_chain_config;
                xinfo("xtop_chain_fork_config_center::init mainnet config");
            } else if (top::config::chain_name_testnet == XGET_CONFIG(chain_name)) {
                m_fork_config = testnet_chain_config;
                xinfo("xtop_chain_fork_config_center::init testnet config");
            } else {
                m_fork_config = default_chain_config;
                xinfo("xtop_chain_fork_config_center::init default config");
            }
        }
    }
}
