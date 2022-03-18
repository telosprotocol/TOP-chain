// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xchain_fork/xchain_upgrade_center.h"

#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xchain_names.h"
#include "xvledger/xvblock.h"

#include <cinttypes>

namespace top {
    namespace chain_fork {

        xchain_fork_config_t xtop_chain_fork_config_center::m_fork_config;

#if defined(XCHAIN_FORKED_BY_DEFAULT)
#   if defined(XBUILD_CI)
        const uint32_t BLOCK_FORK_POINT = 6000000;
#   else
        const uint32_t BLOCK_FORK_POINT = 0;
#   endif

            xchain_fork_config_t  mainnet_chain_config{
                xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "blacklist function fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "node initial credit fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, BLOCK_FORK_POINT, "v3 block fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "enable fullnode election"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "enable fullnode related func"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fee fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "election contract store miner type & genesis flag"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "partly remove confirm"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"},
            };

            // !!!change!!! fork time for galileo
            xchain_fork_config_t  testnet_chain_config{
                xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "blacklist function fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "node initial credit fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, BLOCK_FORK_POINT, "v3 block fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "enable fullnode election"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "enable fullnode related func"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fee fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "election contract store miner type & genesis flag"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "partly remove confirm"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"},
           };

            xchain_fork_config_t default_chain_config {
                xfork_point_t{xfork_point_type_t::logic_time, 0, "block fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "blacklist function fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "node initial credit fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, BLOCK_FORK_POINT, "v3 block fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "enable fullnode election"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "enable fullnode related func"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "tx v2 fee fork point"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "election contract store miner type & genesis flag"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "partly remove confirm"},
                xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"},
            };
#else   // #if defined(XCHAIN_FORKED_BY_DEFAULT)
        xchain_fork_config_t  mainnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "block fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "blacklist function fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "node initial credit fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7126740, "v3 block fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7126740, "enable fullnode election"},
            xfork_point_t{xfork_point_type_t::logic_time, 7129260, "enable fullnode related func"},
            xfork_point_t{xfork_point_type_t::logic_time, 7221960, "tx v2 fee fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7481160, "election contract store miner type & genesis flag"},
            xfork_point_t{xfork_point_type_t::logic_time, 7482600, "partly remove confirm"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"},
        };

        // !!!change!!! fork time for galileo
        xchain_fork_config_t  testnet_chain_config{
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "block fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "blacklist function fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "node initial credit fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7126740, "v3 block fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7126740, "enable fullnode election"},
            xfork_point_t{xfork_point_type_t::logic_time, 7129260, "enable fullnode related func"},
            xfork_point_t{xfork_point_type_t::logic_time, 7221960, "tx v2 fee fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7481160, "election contract store miner type & genesis flag"},
            xfork_point_t{xfork_point_type_t::logic_time, 7482600, "partly remove confirm"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"},
        };

        // !!!change!!! fork time for local develop net
        xchain_fork_config_t default_chain_config {
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "block fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "blacklist function fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 6859080, "node initial credit fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7126740, "v3 block fork point"},
            xfork_point_t{xfork_point_type_t::logic_time, 7126740, "enable fullnode election"},
            xfork_point_t{xfork_point_type_t::logic_time, 7129260, "enable fullnode related func"},
            xfork_point_t{xfork_point_type_t::logic_time, 7221960, "tx v2 fee fork point"},//2022-2-21 10:00:00
            xfork_point_t{xfork_point_type_t::logic_time, 7481160, "election contract store miner type & genesis flag"},
            xfork_point_t{xfork_point_type_t::logic_time, 7482600, "partly remove confirm"},
            xfork_point_t{xfork_point_type_t::logic_time, 0, "add rsp id"},
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
            if (!fork_point.has_value()) {
                return false;
            }

            xdbg("xtop_chain_fork_config_center::is_forked target:%llu, fork point:%llu", target, fork_point.value().point);
            return  target >= fork_point.value().point;
        }

        bool xtop_chain_fork_config_center::is_forked(top::optional<xfork_point_t> const & fork_point, uint64_t fork_point_offset, uint64_t target) noexcept {
            auto const fork_point_value = (fork_point.has_value() ? fork_point.value().point : static_cast<uint64_t>(0)) + fork_point_offset;

            xdbg("xtop_chain_fork_config_center::is_forked target:%" PRIu64 ", fork point:%" PRIu64, target, fork_point_value);
            return target >= fork_point_value;
        }

        bool xtop_chain_fork_config_center::is_block_forked(uint64_t target) noexcept {
            xchain_fork_config_t const & _fork_config = xtop_chain_fork_config_center::get_chain_fork_config();
            return  xtop_chain_fork_config_center::is_forked(_fork_config.V3_0_0_0_block_fork_point, target);
        }

        bool xtop_chain_fork_config_center::is_tx_forked_by_timestamp(uint64_t fire_timestamp) noexcept {
            xchain_fork_config_t const & _fork_config = xtop_chain_fork_config_center::get_chain_fork_config();
            auto clock = _fork_config.block_fork_point.value().point;
            auto clock_time_stamp = clock * 10 + base::TOP_BEGIN_GMTIME;
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

        void xtop_chain_fork_config_center::update(uint64_t cur_time, std::map<std::string, uint64_t> const & new_config) {
            for (auto config : new_config) {
                auto name = config.first;
                auto new_point = config.second;
                top::optional<xfork_point_t> * ptr = nullptr;

                if (name == "block_fork_point") {
                    ptr = &m_fork_config.block_fork_point;
                } else if (name == "blacklist_function_fork_point") {
                    ptr = &m_fork_config.blacklist_function_fork_point;
                } else if (name == "node_initial_credit_fork_point") {
                    ptr = &m_fork_config.node_initial_credit_fork_point;
                } else if (name == "V3_0_0_0_block_fork_point") {
                    ptr = &m_fork_config.V3_0_0_0_block_fork_point;
                } else if (name == "enable_fullnode_election_fork_point") {
                    ptr = &m_fork_config.enable_fullnode_election_fork_point;
                } else if (name == "enable_fullnode_related_func_fork_point") {
                    ptr = &m_fork_config.enable_fullnode_related_func_fork_point;
                } else if (name == "tx_v2_fee_fork_point") {
                    ptr = &m_fork_config.tx_v2_fee_fork_point;
                } else if (name == "election_contract_stores_miner_type_and_genesis_fork_point") {
                    ptr = &m_fork_config.election_contract_stores_miner_type_and_genesis_fork_point;
                } else if (name == "partly_remove_confirm") {
                    ptr = &m_fork_config.partly_remove_confirm;
                } else {
                    xwarn("xtop_chain_fork_config_center::update invalid fork point (%s) not found!", name.c_str());
                    continue;
                }

                if (!ptr->has_value()) {
                    xwarn("xtop_chain_fork_config_center::update invalid fork point (%s) empty!", name.c_str());
                    continue;
                }
                if (new_point < cur_time || ptr->value().point < cur_time) {
                    xwarn("xtop_chain_fork_config_center::update invalid fork time convertion (%lu->%lu), cur time (%lu)!", ptr->value().point, new_point, cur_time);
                    continue;
                }
                ptr->value().point = new_point;
                xinfo("xtop_chain_fork_config_center::update fork (%s) with new value (%lu)", name.c_str(), ptr->value().point);
            }
        }
    }
}
