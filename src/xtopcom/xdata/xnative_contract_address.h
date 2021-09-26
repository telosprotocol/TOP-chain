// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcxx_config.h"

#include <cstdint>

namespace top {

XINLINE_CONSTEXPR uint32_t MAIN_CHAIN_REC_TABLE_USED_NUM{1};
XINLINE_CONSTEXPR uint32_t MAIN_CHAIN_ZEC_TABLE_USED_NUM{3};

XINLINE_CONSTEXPR char const * black_hole_addr{"T!000131R4UAjgF6ZBWnwZESMWx4nCnqL1GhM3nT3"};
XINLINE_CONSTEXPR char const * genesis_root_addr_main_chain{"T$000132i21FyYZvjTKiEwvBjshUbfQx6xoNT68v5"};
XINLINE_CONSTEXPR char const * sys_contract_beacon_timer_addr{"Tt00013axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx"};
XINLINE_CONSTEXPR char const * sys_drand_addr{"Tr00013aFJ3pTJ56d7Nrc3VtwUQPwkXRL1vozEvCh"};

XINLINE_CONSTEXPR char const * sys_contract_rec_registration_addr{ "T2000138NZjvNJjRNG5iEqVKydpqAqoeNjBuFmNbj@0" };
XINLINE_CONSTEXPR char const * sys_contract_rec_elect_edge_addr{ "T2000138NpRxYCFQxMHvedTxRpgkb8B7oHt235N2W@0" };
XINLINE_CONSTEXPR char const * sys_contract_rec_elect_archive_addr{ "T2000138NXb36GkofBUMqxCAZqdERi63htDVC8Yzt@0" };
XINLINE_CONSTEXPR char const * sys_contract_rec_elect_rec_addr{ "T2000138JQPo5TcurZsVLFUMd5vHJRBLenLWjLhk6@0" };
XINLINE_CONSTEXPR char const * sys_contract_rec_elect_zec_addr{ "T2000138Kc9WynduqxJvX3VCU7XjHCR9YyKuBL1fx@0" };
XINLINE_CONSTEXPR char const * sys_contract_rec_tcc_addr{ "T2000138Ao4jjYtrXoNwfzb6gdpD2XNBpqUv46p8B@0" };
XINLINE_CONSTEXPR char const * sys_contract_rec_standby_pool_addr{ "T2000138CQwyzFxbWZ59mNjkq3eZ3eH41t7b5midm@0" };

XINLINE_CONSTEXPR char const * sys_contract_zec_workload_addr{ "T200024uMvLFmyttx6Nccv4jKP3VfRq9NJ2mxcNxh@0" };
XINLINE_CONSTEXPR char const * sys_contract_zec_vote_addr{ "T200024uPV1k6XjTm9No5yB2mxBPK9o2XqJKyhDUn@0" };
XINLINE_CONSTEXPR char const * sys_contract_zec_reward_addr{ "T200024uV5yB1ZCnXe7SbViA86ufhouFjpDKNRd3X@0" };
XINLINE_CONSTEXPR char const * sys_contract_zec_slash_info_addr{ "T200024uDhihoPJ24LQL4znxrugPM4eWk8rY42ceS@1" };
XINLINE_CONSTEXPR char const * sys_contract_zec_elect_consensus_addr{ "T200024uHxGKRST3hk5tKFjVpuQbGNDihMJR6qeeQ@2" };
XINLINE_CONSTEXPR char const * sys_contract_zec_standby_pool_addr{ "T200024uCQ5Di2vZmPURNYVUuvWm5p7EaFQrRLs76@2" };
XINLINE_CONSTEXPR char const * sys_contract_zec_group_assoc_addr{ "T200024uN3e6AujFyvDXY4h5t6or3DgKpu5rTKELD@2" };

XINLINE_CONSTEXPR char const * sys_contract_sharding_vote_addr{ "T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa" };
XINLINE_CONSTEXPR char const * sys_contract_sharding_reward_claiming_addr{ "T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg" };
XINLINE_CONSTEXPR char const * sys_contract_sharding_statistic_info_addr{ "T20000ML7oBZbitBCcXhrJwqBhha2MUimd6SM9Z6" };

XINLINE_CONSTEXPR char const * sys_contract_beacon_table_block_addr{ "Ta0001" };
XINLINE_CONSTEXPR char const * sys_contract_zec_table_block_addr{ "Ta0002" };
XINLINE_CONSTEXPR char const * sys_contract_sharding_table_block_addr{ "Ta0000" };

}  // namespace top
