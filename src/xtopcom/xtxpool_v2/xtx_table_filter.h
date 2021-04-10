#pragma once

#include "xtxpool_v2/xtx_account_filter.h"
#include "xbasic/xlru_cache.h"

NS_BEG2(top, xtxpool_v2)
using namespace top::data;
using namespace top::basic;
using data::xblock_t;
using data::xcons_transaction_ptr_t;
using data::enum_transaction_subtype;
using basic::xlru_cache;

using xaccount_addr_t = std::string;

class xtx_table_filter {
public:
    xtx_table_filter() = default;
    xtx_table_filter(base::xvblockstore_t * blockstore):m_send_filters_cache(100), m_recv_filters_cache(100), m_blockstore(blockstore){
    };

    enum_xtxpool_error_type reject(const std::string account, const xcons_transaction_ptr_t &tx, bool &deny);

    enum_xtxpool_error_type reject(const std::string &account, const xcons_transaction_ptr_t &tx, uint64_t pre_unitblock_height, bool &deny);
    enum_xtxpool_error_type update_reject_rule(const std::string &account, const data::xblock_t *unit_block);
    xcons_transaction_ptr_t get_tx(const std::string &account, const std::string hash);
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint64_t now);
    uint32_t get_unconfirm_txs_num() const;
private:
    xaccount_filter_ptr_t get(const std::string &account, uint8_t subtype);
    xlru_cache<xaccount_addr_t,xaccount_filter_ptr_t> m_send_filters_cache;
    xlru_cache<xaccount_addr_t,xaccount_filter_ptr_t> m_recv_filters_cache;
    std::unordered_map<xaccount_addr_t, xaccount_filter_ptr_t> m_confirm_filters_cache;
    base::xvblockstore_t * m_blockstore;
};
NS_END2
