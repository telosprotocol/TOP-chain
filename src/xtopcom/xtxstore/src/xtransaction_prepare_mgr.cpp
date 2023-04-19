#include "xtxstore/xtransaction_prepare_mgr.h"

#include "xdata/xtransaction_cache.h"
#include "xmbus/xevent_store.h"
#include "xpbase/base/top_utils.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xdata/xblocktool.h"
NS_BEG2(top, txexecutor)

xtransaction_prepare_mgr::xtransaction_prepare_mgr(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<xbase_timer_driver_t> const & timer_driver)
  : m_mbus{mbus}, m_timer_driver{timer_driver}, m_transaction_cache{std::make_shared<data::xtransaction_cache_t>()} {
    xdbg("xtransaction_prepare_mgr init %p, %p, %p", this, m_timer_driver.get(), m_transaction_cache.get());
}

void xtransaction_prepare_mgr::start() {
    if (running()) {
        return;
    }
    if (m_mbus != nullptr)
        m_listener = m_mbus->add_listener(top::mbus::xevent_major_type_store, std::bind(&xtransaction_prepare_mgr::on_block_to_db_event, this, std::placeholders::_1));
    assert(!running());
    running(true);
    assert(running());
    xdbg("xtransaction_prepare_mgr start %p, %p, %p", this, m_timer_driver.get(), m_transaction_cache.get());
    do_check_tx();
    // base::xxtimer_t::start(0, 60*1000);
    return;
}
void xtransaction_prepare_mgr::stop() {
    if (!running()) {
        return;
    }
    xdbg("xtransaction_prepare_mgr stop");
    assert(running());
    running(false);
    assert(!running());
    m_transaction_cache->tx_clear();

    if (m_mbus != nullptr)
        m_mbus->remove_listener(top::mbus::xevent_major_type_store, m_listener);
}

std::shared_ptr<data::xtransaction_cache_t> xtransaction_prepare_mgr::transaction_cache() {
    return m_transaction_cache;
}

void xtransaction_prepare_mgr::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_committed) {
        return;
    }

    mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);

    if (block_event->blk_level != base::enum_xvblock_level_table) {
        return;
    }
    const data::xblock_ptr_t & block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_txpool_db_event_on_block);
    update_prepare_cache(block);
    return;
}

int xtransaction_prepare_mgr::update_prepare_cache(const data::xblock_ptr_t bp) {
    base::xvaccount_t _vaccount(bp->get_account());
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaccount, bp.get(), metrics::blockstore_access_from_txpool_refresh_table)) {
        xerror("xtransaction_prepare_mgr::update_prepare_cache fail-load block input, block=%s", bp->dump().c_str());
        return -1;
    }

    Json::Value jv;
    auto input_actions = data::xblockextract_t::unpack_txactions((base::xvblock_t*)bp.get());
    for(auto & action : input_actions) {
        base::enum_transaction_subtype _subtype = (base::enum_transaction_subtype)action.get_org_tx_action_id();
        
        data::xlightunit_action_ptr_t txaction = std::make_shared<data::xlightunit_action_t>(action);
        xdbg("tran hash: %s", top::HexEncode(txaction->get_tx_hash()).c_str());

        data::xlightunit_action_ptr_t recv_txinfo;
        data::xtransaction_cache_data_t cache_data;
        if (m_transaction_cache->tx_get(txaction->get_tx_hash(), cache_data) == 0) {
            xdbg("not find tran: %s", top::HexEncode(txaction->get_tx_hash()).c_str());
            continue;
        }
        recv_txinfo = cache_data.recv_txinfo;

        data::xtransaction_ptr_t tx_ptr = cache_data.tran;

        jv["height"] = static_cast<Json::UInt64>(bp->get_height());
        auto tx_info = txaction;
        if (tx_info != nullptr) {
            jv["used_gas"] = static_cast<Json::UInt64>(tx_info->get_used_tgas());
            if (tx_info->is_self_tx()) {
                jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(tx_info->get_tx_exec_status());
                jv["used_deposit"] = static_cast<Json::UInt64>(tx_info->get_used_deposit());
            }
            if (tx_info->is_send_tx()) {
                if ((tx_ptr->get_tx_type() == data::xtransaction_type_transfer) && (tx_ptr->get_tx_version() == data::xtransaction_version_2 || tx_info->get_not_need_confirm())) {
                    jv["used_deposit"] = static_cast<Json::UInt64>(tx_info->get_used_deposit());
                } else {
                    jv["used_deposit"] = 0;
                }
            }
            if (tx_info->is_confirm_tx()) {
                jv["used_deposit"] = static_cast<Json::UInt64>(tx_info->get_used_deposit());
                if (recv_txinfo != nullptr) {
                    jv["recv_tx_exec_status"] = data::xtransaction_t::tx_exec_status_to_str(recv_txinfo->get_tx_exec_status());
                    jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(tx_info->get_tx_exec_status() | recv_txinfo->get_tx_exec_status());
                }
            }
        }
        if (_subtype == base::enum_transaction_subtype_self) {
            m_transaction_cache->tx_erase(txaction->get_tx_hash());
            continue;
        } else if (_subtype == base::enum_transaction_subtype_send) {
            auto beacon_tx_fee = txexecutor::xtransaction_fee_t::cal_service_fee(tx_ptr->source_address().to_string(), tx_ptr->target_address().to_string());
            jv["tx_fee"] = static_cast<Json::UInt64>(beacon_tx_fee);
            m_transaction_cache->tx_set_json(txaction->get_tx_hash(), base::enum_transaction_subtype_send, jv);
        } else if (_subtype == base::enum_transaction_subtype_recv) {
            m_transaction_cache->tx_set_recv_txinfo(txaction->get_tx_hash(), txaction);
            m_transaction_cache->tx_set_json(txaction->get_tx_hash(), base::enum_transaction_subtype_recv, jv);
        } else if (_subtype == base::enum_transaction_subtype_confirm) {
            m_transaction_cache->tx_set_json(txaction->get_tx_hash(), base::enum_transaction_subtype_confirm, jv);
        } else
            continue;

        if (_subtype == base::enum_transaction_subtype_confirm || (_subtype == base::enum_transaction_subtype_recv && tx_info->get_not_need_confirm()))
            m_transaction_cache->tx_erase(txaction->get_tx_hash());
    }
    return 0;
}
std::string xtransaction_prepare_mgr::tx_exec_status_to_str(uint8_t exec_status) {
    if (exec_status == data::enum_xunit_tx_exec_status_success) {
        return "success";
    }
    return "failure";
}
void xtransaction_prepare_mgr::do_check_tx() {
    xdbg("do_check_tx1");
    if (!running()) {
        return;
    }

    assert(m_timer_driver != nullptr);

    auto self = shared_from_this();
    m_timer_driver->schedule(std::chrono::minutes{1}, [this, self](std::chrono::milliseconds) {
        if (!running()) {
            return;
        }
        xdbg("do_check_tx2");
        m_transaction_cache->tx_clean();
        do_check_tx();
    });
}
NS_END2
