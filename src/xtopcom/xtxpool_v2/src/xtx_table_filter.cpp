#include "xtxpool_v2/xtx_table_filter.h"
#include "xtxpool_v2/xtxpool_log.h"

NS_BEG2(top, xtxpool_v2)

enum_xtxpool_error_type xtx_table_filter::reject(const std::string account, const xcons_transaction_ptr_t &tx, bool &deny){
    xaccount_filter_ptr_t filter = get(account,tx->get_tx_subtype());
    xassert(filter != nullptr);

    enum_xtxpool_error_type result = filter->reject(tx, deny);
    return result;
}

enum_xtxpool_error_type xtx_table_filter::reject(const std::string &account, const xcons_transaction_ptr_t &tx, uint64_t pre_unitblock_height, bool &deny){
    xaccount_filter_ptr_t filter = get(account,tx->get_tx_subtype());
    xassert(filter != nullptr);

    enum_xtxpool_error_type result = filter->reject(tx, pre_unitblock_height, deny);
    return result;
}

enum_xtxpool_error_type xtx_table_filter::update_reject_rule(const std::string &account, const data::xblock_t *unit_block){
    xaccount_filter_ptr_t filter;
    enum_xtxpool_error_type result;

    enum_transaction_subtype subtypes[] = {enum_transaction_subtype_recv, enum_transaction_subtype_confirm};
    for (uint32_t i = 0; i < sizeof(subtypes)/sizeof(subtypes[0]); i++) {
        filter = get(account, subtypes[i]);
        result = filter->update_reject_rule(unit_block);
        if (result != xtxpool_success){
            return result;
        }
        if (subtypes[i] == enum_transaction_subtype_confirm && filter->size() == 0) {
            m_confirm_filters_cache.erase(account);
        }
    }

    return xtxpool_success;
}

xcons_transaction_ptr_t xtx_table_filter::get_tx(const std::string &account, const std::string hash){
    xaccount_filter_ptr_t filter = get(account, enum_transaction_subtype_confirm);
    return filter->get_tx(hash);
}

const std::vector<xcons_transaction_ptr_t> xtx_table_filter::get_resend_txs(uint64_t now) {
    std::vector<xcons_transaction_ptr_t> resend_txs;
    for (auto filter : m_confirm_filters_cache) {
        auto txs = filter.second->get_resend_txs(now);
        resend_txs.insert(resend_txs.end(), txs.begin(), txs.end());
    }
    return resend_txs;
}

uint32_t xtx_table_filter::get_unconfirm_txs_num() const {
    uint32_t num = 0;
    for (auto filter : m_confirm_filters_cache) {
        num += filter.second->size();
    }
    return num;
}

xaccount_filter_ptr_t xtx_table_filter::get(const std::string &account, uint8_t subtype){
    xaccount_filter_ptr_t filter = nullptr;
    switch (subtype) {
        case enum_transaction_subtype_recv:
        {
            if (!m_recv_filters_cache.get(account, filter)){
                filter = make_object_ptr<xaccount_recvtx_filter>(account, m_blockstore);
                m_recv_filters_cache.put(account,filter);
            }
            break;
        }
        case enum_transaction_subtype_confirm:
        {
            auto it = m_confirm_filters_cache.find(account);
            if (it == m_confirm_filters_cache.end()){
                filter = make_object_ptr<xaccount_confirmtx_filter>(account, m_blockstore);
                m_confirm_filters_cache.insert({account,filter});
            } else {
                filter = it->second;
            }
            break;
        }
        default:
        {
            break;
        }

    }

    return filter;
}
NS_END2
