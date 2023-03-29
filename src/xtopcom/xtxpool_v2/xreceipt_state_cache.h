// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_resources_face.h"
#include "xvledger/xreceiptid.h"
#include "xvledger/xvaccount.h"
#include "xcommon/xtable_address.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

#undef __MODULE__
#define __MODULE__ "xtxpool_v2"

class xtable_state_cache_t {
public:
    xtable_state_cache_t(xtxpool_resources_face * para, const std::string & table_addr) : m_para(para), m_table_address(common::xtable_address_t::build_from(table_addr)) {
    }
    uint64_t get_tx_corresponding_latest_receipt_id(const std::shared_ptr<xtx_entry> & tx) const;
    uint64_t get_confirmid_max(base::xtable_shortid_t peer_table_sid) const;
    uint64_t get_recvid_max(base::xtable_shortid_t peer_table_sid) const;
    uint64_t last_update_time() const;
    bool is_unconfirmed_num_reach_limit(base::xtable_shortid_t peer_table_sid) const;
    bool get_account_index(const std::string & account, base::xaccount_index_t & account_index) const;
    uint64_t get_state_height() const;

private:
    xtxpool_resources_face * m_para;
    common::xtable_address_t m_table_address;
};

NS_END2
