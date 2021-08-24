#pragma once

#include "gtest/gtest.h"
#include "xchain_timer/xchain_timer.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xtxpool_v2/xtxpool_face.h"

#include <string>
#include <vector>

using namespace top::xtxpool_v2;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;

class test_xtxpool_util_t {
public:
    static std::string                          get_account(uint32_t account_idx);
    static xcons_transaction_ptr_t              create_cons_transfer_tx(uint32_t            sender_idx,
                                                                        uint32_t            receiver_idx,
                                                                        uint64_t            nonce,
                                                                        uint64_t            timestamp,
                                                                        uint256_t           last_tx_hash = {},
                                                                        uint16_t            expire_duration = 100,
                                                                        uint64_t            amount = 100,
                                                                        bool                sign = true);
    static xcons_transaction_ptr_t              create_cons_transfer_tx(const std::string & from,
                                                                        const std::string & to,
                                                                        uint64_t            nonce,
                                                                        uint64_t            timestamp,
                                                                        uint256_t           last_tx_hash = {},
                                                                        uint16_t            expire_duration = 100,
                                                                        uint64_t            amount = 100);
    static std::vector<xcons_transaction_ptr_t> create_cons_transfer_txs(uint32_t sender_idx, uint32_t receiver_idx, uint32_t count);
};
