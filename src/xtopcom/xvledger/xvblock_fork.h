// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"

#include <functional>

namespace top
{
    namespace base
    {
        enum enum_xvblock_fork_version
        {
            enum_xvblock_fork_version_init              = 0x000100,  // 0.1.0 init version
            enum_xvblock_fork_version_table_prop_prove  = 0x010000,  // 1.0.0 table block input action include prop hashs for prove receiptid state
            enum_xvblock_fork_version_unit_opt          = 0x020000,  // 2.0.0 unit block not include txaction but only txhash and state
            enum_xvblock_fork_version_3_0_0             = 0x030000,  // 3.0.0: 1.table-block add second gmtime
            enum_xvblock_fork_version_compatible_eth    = 0x040000,  // 4.0.0: block compatible with eth
            enum_xvblock_fork_version_4_1_0             = 0x040100,  // 4.1.0: v1.6 support relay-chain
            enum_xvblock_fork_version_5_0_0             = 0x050000,  // 5.0.0: v1.7 unit off table
            enum_xvblock_fork_version_6_0_0             = 0x060000,  // 6.0.0: v1.12 block object seperate with input output;simple unit;genesis accounts in mpt;
        };

        // XTODO need change old and new version when block structure changed
        XINLINE_CONSTEXPR uint32_t TOP_BLOCK_FORK_OLD_VERSION = enum_xvblock_fork_version_5_0_0;  // XTODO v1.9.2 v1.10 not released
        XINLINE_CONSTEXPR uint32_t TOP_BLOCK_FORK_NEW_VERSION = enum_xvblock_fork_version_6_0_0;

        typedef std::function<bool(uint64_t clock)> xvblock_fork_check_fun_t;

        class xvblock_fork_t
        {
        public:
            static uint32_t get_block_init_version() {return (uint32_t)enum_xvblock_fork_version_init;}
            static uint32_t get_block_fork_old_version() {return TOP_BLOCK_FORK_OLD_VERSION;}
            static uint32_t get_block_fork_new_version() {return TOP_BLOCK_FORK_NEW_VERSION;}

            static bool is_block_match_version(uint32_t block_version, enum_xvblock_fork_version fork_version) {
                return block_version >= (uint32_t)fork_version;
            }

            static bool is_block_older_version(uint32_t block_version, enum_xvblock_fork_version fork_version) {
                return block_version < (uint32_t)fork_version;
            }

        public:
            static xvblock_fork_t & instance() {
                static xvblock_fork_t _instance;
                return _instance;
            }
            void init(const xvblock_fork_check_fun_t & _fun) {
                m_check_fun = _fun;
            }
            uint32_t get_expect_block_version(uint64_t current_clock) const {
                if (is_forked(current_clock)) {
                    return get_block_fork_new_version();
                }
                return get_block_fork_old_version();
            }
        protected:
            bool     is_forked(uint64_t current_clock) const {
                if (m_check_fun != nullptr) {
                    return m_check_fun(current_clock);
                }
                return true;  // default always forked
            }
        private:
            xvblock_fork_check_fun_t m_check_fun;
        };

    }//end of namespace of base

}//end of namespace top
