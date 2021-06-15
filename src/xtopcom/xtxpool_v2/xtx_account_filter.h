#pragma once

#include "xvledger/xvblock.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xlightunit.h"
#include "xdata/xlightunit_info.h"
#include "xrpc/xuint_format.h"
#include "xtxpool_v2/xreceipt_resend.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xverifier/xverifier_utl.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

NS_BEG2(top, xtxpool_v2)

using data::xblock_t;
using data::xcons_transaction_ptr_t;
using data::xcons_transaction_t;
using data::xlightunit_tx_info_ptr_t;

using xaccount_addr_t = std::string;
using xtx_hash_t = std::string;

class xtx_commited_meta_entry {
public:
    xtx_commited_meta_entry() = default;
    xtx_commited_meta_entry(uint64_t unitblock_height, std::string hash, uint64_t clockblock_height);
    uint64_t get_unitblock_height();
    uint64_t get_clockblock_height();

private:
    uint64_t m_unitblock_height{0};
    xtx_hash_t m_hash;
    uint64_t m_clockblock_height{0};
};

class xtx_recvtx_entry : public xtx_commited_meta_entry {
public:
    xtx_recvtx_entry() = default;
    xtx_recvtx_entry(uint64_t unitblock_height, std::string hash, uint64_t clockblock_height) : xtx_commited_meta_entry(unitblock_height, hash, clockblock_height){};
};

class xtx_unconfirm_tx_entry : public xtx_commited_meta_entry {
public:
    xtx_unconfirm_tx_entry() = default;
    xtx_unconfirm_tx_entry(uint64_t unitblock_height, std::string hash, uint64_t clockblock_height, xcons_transaction_ptr_t tx)
      : xtx_commited_meta_entry(unitblock_height, hash, clockblock_height), m_tx(tx), m_resend_time(get_next_resend_time(tx->get_receipt_gmtime(), xverifier::xtx_utl::get_gmttime_s())) {
    }

    const xcons_transaction_ptr_t & get_raw_tx() const;
    void update_resend_time(uint64_t now) {
        m_resend_time = get_next_resend_time(m_tx->get_receipt_gmtime(), now);
    }
    uint64_t get_resend_time() const {
        return m_resend_time;
    }

private:
    xcons_transaction_ptr_t m_tx{nullptr};
    uint64_t m_resend_time{0};
};

class xtx_unconfirm_tx_retry_comp {
public:
    bool operator()(xtx_unconfirm_tx_entry left, xtx_unconfirm_tx_entry right) const {
        return left.get_resend_time() < right.get_resend_time();
    }
};

using xtx_unconfirm_txs_t = std::multiset<xtx_unconfirm_tx_entry, xtx_unconfirm_tx_retry_comp>;

class xaccount_filter : public base::xvaccount_t {
public:
    xaccount_filter(xaccount_addr_t account, base::xvblockstore_t * blockstore) : base::xvaccount_t(account), m_blockstore(blockstore){};
protected:
    ~xaccount_filter() {}
public:
    // base::xvaccount_t get_account_addr();
    base::xvblockstore_t * get_blockstore();
    virtual enum_xtxpool_error_type reject(const xcons_transaction_ptr_t & tx, bool & deny) = 0;
    virtual enum_xtxpool_error_type reject(const xcons_transaction_ptr_t & tx, uint64_t pre_unitblock_height, bool & deny) = 0;
    virtual enum_xtxpool_error_type update_reject_rule(const data::xblock_t * unit_block) = 0;
    virtual xcons_transaction_ptr_t get_tx(const xtx_hash_t hash) = 0;
    virtual uint32_t size() = 0;
    virtual const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint64_t now) = 0;

protected:
    virtual enum_xtxpool_error_type initialize() = 0;
    virtual enum_xtxpool_error_type sync_reject_rules(uint64_t unitblock_height) = 0;

private:
    base::xvblockstore_t * m_blockstore{nullptr};
};

using xaccount_filter_ptr_t = xobject_ptr_t<xaccount_filter>;

class xaccount_recvtx_filter : public xaccount_filter {
public:
    xaccount_recvtx_filter(xaccount_addr_t account, base::xvblockstore_t * blockstore) : xaccount_filter(account, blockstore){};
protected:
    ~xaccount_recvtx_filter() {}
public:
    enum_xtxpool_error_type reject(const xcons_transaction_ptr_t & tx, bool & deny) override;
    enum_xtxpool_error_type reject(const xcons_transaction_ptr_t & tx, uint64_t pre_unitblock_height, bool & deny) override;
    enum_xtxpool_error_type update_reject_rule(const data::xblock_t * unit_block) override;
    xcons_transaction_ptr_t get_tx(const xtx_hash_t hash) override;
    uint32_t size() override;
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint64_t now) override;
private:
    void insert(const data::xblock_t * unit_block);
    void evict();
    enum_xtxpool_error_type initialize() override;
    enum_xtxpool_error_type sync_reject_rules_and_reject(const xcons_transaction_ptr_t & tx, uint64_t unitblock_height, bool & deny);
    enum_xtxpool_error_type sync_reject_rules(uint64_t unitblock_height) override;
    std::unordered_map<xtx_hash_t, xtx_recvtx_entry> m_reject_rules;
    std::map<uint64_t, std::set<xtx_hash_t>> m_height_of_unitblocks;
    const uint32_t m_cap_of_heights = 100;
};

using xaccount_recvtx_filter_ptr_t = xobject_ptr_t<xaccount_recvtx_filter>;

class xaccount_confirmtx_filter : public xaccount_filter {
public:
    xaccount_confirmtx_filter(xaccount_addr_t account, base::xvblockstore_t * blockstore) : xaccount_filter(account, blockstore){};
    enum_xtxpool_error_type reject(const xcons_transaction_ptr_t & tx, bool & deny) override;
    enum_xtxpool_error_type reject(const xcons_transaction_ptr_t & tx, uint64_t unitblock_height_view, bool & deny) override;
    enum_xtxpool_error_type update_reject_rule(const data::xblock_t * unit_block) override;
    xcons_transaction_ptr_t get_tx(const xtx_hash_t hash) override;
    uint32_t size() override;
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint64_t now) override;

private:
    enum_xtxpool_error_type initialize() override;
    enum_xtxpool_error_type sync_reject_rules(uint64_t unitblock_height) override;
    uint64_t m_highest_height{0};
    xtx_unconfirm_txs_t m_unconfirm_txs;
    std::map<xtx_hash_t, xtx_unconfirm_txs_t::iterator> m_permit_rules;
};
NS_END2
