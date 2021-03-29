#include "xtxpool_v2/xtx_account_filter.h"
#include "xtxpool_v2/xtxpool_log.h"

#include "xbase/xbase.h"
#include "xbase/xlog.h"
#include "xdata/xblocktool.h"

#include <unordered_set>

NS_BEG2(top, xtxpool_v2)

#define xtxpool_more_clock_height (20)

xaccount_addr_t xaccount_filter::get_account_addr() {
    return m_account_addr;
}

base::xvblockstore_t * xaccount_filter::get_blockstore() {
    return m_blockstore;
}

enum_xtxpool_error_type xaccount_recvtx_filter::reject(const xcons_transaction_ptr_t & tx, bool & deny) {
    deny = false;
    if (m_height_of_unitblocks.empty()) {
        auto unit_block = get_blockstore()->get_latest_committed_block(get_account_addr());
        xassert(unit_block != nullptr);

        return sync_reject_rules_and_reject(tx, unit_block->get_height(), deny);
    }

    uint256_t hash = tx->get_tx_info()->get_tx_hash_256();
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());

    if (m_reject_rules.find(hash_str) != m_reject_rules.end()) {
        deny = true;
        return xtxpool_success;
    }

    return xtxpool_success;
}

enum_xtxpool_error_type xaccount_recvtx_filter::reject(const xcons_transaction_ptr_t & tx, uint64_t pre_unitblock_height, bool & deny) {
    enum_xtxpool_error_type result = sync_reject_rules_and_reject(tx, pre_unitblock_height, deny);
    return result;
}

// void del_filter_rule(const data::xblock_t* unit_block) override;
enum_xtxpool_error_type xaccount_recvtx_filter::update_reject_rule(const data::xblock_t * unit_block) {
    insert(unit_block);
    evict();
    return xtxpool_success;
};

xcons_transaction_ptr_t xaccount_recvtx_filter::get_tx(const xtx_hash_t hash) {
    return nullptr;
}

uint32_t xaccount_recvtx_filter::size() {
    return m_reject_rules.size();
}

const std::vector<xcons_transaction_ptr_t> xaccount_recvtx_filter::get_resend_txs(uint64_t now) {
    return {};
}

enum_xtxpool_error_type xaccount_recvtx_filter::initialize() {
    return xtxpool_success;
}

enum_xtxpool_error_type xaccount_recvtx_filter::sync_reject_rules(uint64_t unitblock_height) {
    return xtxpool_success;
}

enum_xtxpool_error_type xaccount_recvtx_filter::sync_reject_rules_and_reject(const xcons_transaction_ptr_t & tx, uint64_t unitblock_height, bool & deny) {
    deny = false;

    uint256_t hash = tx->get_tx_info()->get_tx_hash_256();
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    if (m_reject_rules.find(hash_str) != m_reject_rules.end()) {
        deny = true;
        return xtxpool_success;
    }

    // if miss cache,then find tx in the blockstore
    for (uint64_t cur_height = unitblock_height; cur_height > 0; cur_height--) {
        if (m_height_of_unitblocks.find(cur_height) != m_height_of_unitblocks.end()) {
            continue;
        }

        auto unit_block = get_blockstore()->load_block_object(get_account_addr(), cur_height);
        if (unit_block == nullptr) {
            xtxpool_dbg_info("lack unitblock, the height is %u ", cur_height);
            return xtxpool_error_unitblock_lack;
        }

        auto xvunit_block = dynamic_cast<xblock_t *>(unit_block.get());
        uint64_t height_of_clockblock = xvunit_block->get_timerblock_height();

        if ((xvunit_block->get_timerblock_height() + xtxpool_more_clock_height) < tx->get_clock()) {
            xtxpool_dbg_info("the clockblock height of send tx is %llu, older than %llu", tx->get_clock(), xvunit_block->get_timerblock_height() + xtxpool_more_clock_height);
            break;
        }

        if (unit_block->get_block_class() != base::enum_xvblock_class_light) {
            continue;
        }

        insert(xvunit_block);
        auto & txs = xvunit_block->get_txs();
        for (auto & tx1 : txs) {
            if (!tx1->is_recv_tx()) {
                continue;
            }

            bool eq1 = (tx1->get_raw_tx()->get_digest_str() == tx->get_transaction()->get_digest_str());
            bool eq2 = (tx1->get_raw_tx()->digest() == tx->get_transaction()->digest());
            if (eq1 != eq2) {
                xtxpool_error("sync_reject_rules_and_reject:tx1:%s tx:%s equal judge fail",
                       base::xstring_utl::to_hex(tx1->get_raw_tx()->get_digest_str()).c_str(),
                       base::xstring_utl::to_hex(tx->get_transaction()->get_digest_str()).c_str());
            }
            if (tx1->get_raw_tx()->get_digest_str() == tx->get_transaction()->get_digest_str()) {
                deny = true;
                evict();
                return xtxpool_success;
            }
        }
    }

    evict();
    return xtxpool_success;
}

void xaccount_recvtx_filter::insert(const data::xblock_t * unit_block) {
    if (unit_block->get_block_class() != base::enum_xvblock_class_light) {
        return;
    }

    uint64_t height = unit_block->get_height();
    if ((m_height_of_unitblocks.size() < m_cap_of_heights) || (height > m_height_of_unitblocks.begin()->first)) {
        uint64_t height_of_clockblock = unit_block->get_timerblock_height();
        m_height_of_unitblocks.insert({height, {}});
        auto const it = m_height_of_unitblocks.find(height);
        auto & txs = unit_block->get_txs();
        for (auto & tx1 : txs) {
            if (!tx1->is_recv_tx()) {
                continue;
            }
            uint256_t hash = tx1->get_raw_tx()->digest();
            std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
            xtx_recvtx_entry entry = xtx_recvtx_entry(height, hash_str, height_of_clockblock);
            m_reject_rules[hash_str] = entry;
            it->second.insert(hash_str);
        }
    }
}

void xaccount_recvtx_filter::evict() {
    for (auto iter = m_height_of_unitblocks.begin(); (iter != m_height_of_unitblocks.end() && m_height_of_unitblocks.size() > m_cap_of_heights);) {
        for (xtx_hash_t tx : iter->second) {
            m_reject_rules.erase(tx);
        }
        iter = m_height_of_unitblocks.erase(iter);
    }
}

xcons_transaction_ptr_t xaccount_confirmtx_filter::get_tx(const xtx_hash_t hash) {
    auto it = m_permit_rules.find(hash);
    if (it == m_permit_rules.end()) {
        return nullptr;
    }
    return (*(it->second)).get_raw_tx();
}

enum_xtxpool_error_type xaccount_confirmtx_filter::reject(const xcons_transaction_ptr_t & tx, bool & deny) {
    if (m_highest_height == 0) {
        initialize();
    }

    uint256_t hash = tx->get_transaction()->digest();
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());

    if (m_permit_rules.find(hash_str) != m_permit_rules.end()) {
        deny = false;
        return xtxpool_success;
    }

    deny = true;
    return xtxpool_success;
};

enum_xtxpool_error_type xaccount_confirmtx_filter::reject(const xcons_transaction_ptr_t & tx, uint64_t unitblock_height_view, bool & deny) {
    enum_xtxpool_error_type result = sync_reject_rules(unitblock_height_view);
    if (result != xtxpool_success) {
        return result;
    }

    result = reject(tx, deny);
    return result;
}

enum_xtxpool_error_type xaccount_confirmtx_filter::update_reject_rule(const data::xblock_t * unit_block) {
    enum_xtxpool_error_type result = sync_reject_rules(unit_block->get_height());
    return result;
};

uint32_t xaccount_confirmtx_filter::size() {
    return m_permit_rules.size();
}

const std::vector<xcons_transaction_ptr_t> xaccount_confirmtx_filter::get_resend_txs(uint64_t now) {
    std::vector<xtx_unconfirm_tx_entry> resend_tx_ents;
    for (auto it = m_unconfirm_txs.begin(); it != m_unconfirm_txs.end();) {
        if (it->get_resend_time() <= now) {
            resend_tx_ents.push_back(*it);
            m_permit_rules.erase(it->get_raw_tx()->get_transaction()->get_digest_str());
            it = m_unconfirm_txs.erase(it);
        } else {
            break;
        }
    }

    std::vector<xcons_transaction_ptr_t> resend_txs;
    for (auto & it : resend_tx_ents) {
        resend_txs.push_back(it.get_raw_tx());
        it.update_resend_time(now);
        auto it_unconfirm_txs = m_unconfirm_txs.insert(it);
        m_permit_rules[it.get_raw_tx()->get_transaction()->get_digest_str()] = it_unconfirm_txs;
    }
    return resend_txs;
}

enum_xtxpool_error_type xaccount_confirmtx_filter::initialize() {
    auto unit_block = get_blockstore()->get_latest_committed_block(get_account_addr());
    xassert(unit_block != nullptr);
    enum_xtxpool_error_type result = sync_reject_rules(unit_block->get_height());
    return result;
}

enum_xtxpool_error_type xaccount_confirmtx_filter::sync_reject_rules(uint64_t unitblock_height) {
    std::unordered_set<xtx_hash_t> confirm_txs;
    std::map<xtx_hash_t, xtx_unconfirm_tx_entry> send_txs;

    for (uint64_t cur_height = unitblock_height; cur_height > m_highest_height; cur_height--) {
        auto unit_block = get_blockstore()->load_block_object(get_account_addr(), cur_height);
        if (unit_block == nullptr) {
            xtxpool_dbg_info("lack unitblock, the height is %u", cur_height);
            return xtxpool_error_unitblock_lack;
        }

        if (unit_block->is_genesis_block() || !data::xblocktool_t::is_connect_and_executed_block(unit_block.get())) {
            xtxpool_warn("[unconfirm cache]account state behind, can't update.account=%s,block=%s", get_account_addr().c_str(), unit_block->dump().c_str());
            return xtxpool_error_unitblock_lack;
        }

        if (unit_block->get_block_class() == base::enum_xvblock_class_full) {
            XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_tx", m_unconfirm_txs.size());
            confirm_txs.clear();
            m_permit_rules.clear();
            m_unconfirm_txs.clear();
            xtxpool_dbg_info("meet the full unitblock, means all send tx have confirmed before the height %llu", unit_block->get_height());
            break;
        }

        if (unit_block->get_block_class() != base::enum_xvblock_class_light) {
            continue;
        }

        xblock_t * xblock = dynamic_cast<xblock_t *>(unit_block.get());
        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(xblock);

        if (lightunit->get_unconfirm_sendtx_num() == 0) {
            XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_tx", m_unconfirm_txs.size());
            confirm_txs.clear();
            m_permit_rules.clear();
            m_unconfirm_txs.clear();
            xtxpool_dbg_info("meet the unconfirm send tx zero light unitblock, means all send tx have confirmed before the height %llu", unit_block->get_height());
            break;
        }

        for (auto & tx : lightunit->get_txs()) {
            uint256_t hash = tx->get_tx_hash_256();
            std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
            if (tx->is_send_tx()) {
                auto result = confirm_txs.find(hash_str);
                if (result != confirm_txs.end()) {
                    confirm_txs.erase(result);
                } else {
                    xtx_unconfirm_tx_entry entry =
                        xtx_unconfirm_tx_entry(cur_height, hash_str, xblock->get_timerblock_height(), lightunit->create_one_txreceipt(tx->get_raw_tx().get()));
                    send_txs[hash_str] = entry;
                }
                continue;
            }

            if (tx->is_confirm_tx()) {
                confirm_txs.insert(hash_str);
                continue;
            }
        }
    }

    for (auto & hash : confirm_txs) {
        auto it = m_permit_rules.find(hash);
        if (it != m_permit_rules.end()) {
            m_unconfirm_txs.erase(it->second);
            m_permit_rules.erase(it);
        }
    }

    if (unitblock_height > m_highest_height) {
        m_highest_height = unitblock_height;
    }

    for (auto it_s : send_txs) {
        auto it_unconfirm_txs = m_unconfirm_txs.insert(it_s.second);
        m_permit_rules[it_s.first] = it_unconfirm_txs;
    }
    if (send_txs.size() > confirm_txs.size()) {
        XMETRICS_COUNTER_INCREMENT("txpool_unconfirm_tx", send_txs.size() - confirm_txs.size());
    } else if (send_txs.size() < confirm_txs.size()) {
        XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_tx", confirm_txs.size() - send_txs.size());
    }
    
    return xtxpool_success;
}

xtx_commited_meta_entry::xtx_commited_meta_entry(uint64_t unitblock_height, std::string hash, uint64_t clockblock_height)
  : m_unitblock_height(unitblock_height), m_hash(hash), m_clockblock_height(clockblock_height) {
}

uint64_t xtx_commited_meta_entry::get_unitblock_height() {
    return m_unitblock_height;
}

uint64_t xtx_commited_meta_entry::get_clockblock_height() {
    return m_clockblock_height;
}

const xcons_transaction_ptr_t & xtx_unconfirm_tx_entry::get_raw_tx() const {
    return m_tx;
}

NS_END2