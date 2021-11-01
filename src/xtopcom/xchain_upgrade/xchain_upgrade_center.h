// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>
#include "xbasic/xoptional.hpp"
#include "xchain_upgrade_type.h"

namespace top {
    namespace chain_upgrade {

        extern xchain_fork_config_t mainnet_chain_config;
        extern xchain_fork_config_t testnet_chain_config;
        extern xchain_fork_config_t default_chain_config;

        /**
         * @brief chain fork config center
         *
         */
        class xtop_chain_fork_config_center {
        public:
            static xtop_chain_fork_config_center & instance() {
                static xtop_chain_fork_config_center _instance;
                return _instance;
            }

            static xchain_fork_config_t const & chain_fork_config() noexcept;
            static bool is_forked(top::optional<xfork_point_t> const& fork_point, uint64_t target) noexcept;
            static bool is_block_forked(uint64_t target) noexcept;

        public:
            void    init();
            xchain_fork_config_t const & get_chain_fork_config() noexcept {return m_fork_config;}

        private:
            xchain_fork_config_t    m_fork_config;
        };
        using xchain_fork_config_center_t = xtop_chain_fork_config_center;
    }
}
