#include "xtxstore/xtransaction_prepare_mgr.h"

#include "xdata/xtransaction_cache.h"
#include "xmbus/xevent_store.h"
#include "xpbase/base/top_utils.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"

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
    // xdbg("block tx size: %d, height: %d", block->get_txs().size(), block->get_height());
    update_prepare_cache(block);
    return;
}

int xtransaction_prepare_mgr::update_prepare_cache(const data::xblock_ptr_t bp) {
    const std::vector<base::xventity_t *> & _table_inentitys = bp->get_input()->get_entitys();
    uint32_t entitys_count = _table_inentitys.size();
    xdbg("update_prepare_cache size: %d", entitys_count);
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t * _table_unit_inentity = dynamic_cast<base::xvinentity_t *>(_table_inentitys[index]);
        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

        const std::vector<base::xvaction_t> & input_actions = _table_unit_inentity->get_actions();
        for (auto & action : input_actions) {
            if (action.get_org_tx_hash().empty()) {  // not txaction
                xdbg("empty hash");
                continue;
            }
            data::xlightunit_action_ptr_t txaction = std::make_shared<data::xlightunit_action_t>(action);
            // data::xtransaction_ptr_t _rawtx = bp->query_raw_transaction(txaction->get_tx_hash());
            xdbg("tran hash: %s", top::HexEncode(txaction->get_tx_hash().c_str()).c_str());

            xJson::Value ji;
            data::xlightunit_action_ptr_t recv_txinfo;
            data::xtransaction_cache_data_t cache_data;
            if (m_transaction_cache->tx_get(txaction->get_tx_hash(), cache_data) == 0) {
                xdbg("not find tran: %s", top::HexEncode(txaction->get_tx_hash().c_str()).c_str());
                continue;
            }
            ji = cache_data.jv;
            recv_txinfo = cache_data.recv_txinfo;

            xJson::Value jv;
            // jv["unit_hash"] = bp->get_block_hash_hex_str();
            // jv["unit_hash"] = _unit_header->get_last_block_hash();
            jv["height"] = static_cast<xJson::UInt64>(_unit_header->get_height());

            jv["used_gas"] = txaction->get_used_tgas();
            jv["used_deposit"] = txaction->get_used_deposit();
            if (txaction->is_self_tx()) {
                jv["exec_status"] = tx_exec_status_to_str(txaction->get_tx_exec_status());
            }
            if (txaction->is_confirm_tx()) {
                // TODO(jimmy) should read recv tx exec status from recv tx unit
                if (recv_txinfo != nullptr) {
                    jv["recv_tx_exec_status"] = tx_exec_status_to_str(recv_txinfo->get_tx_exec_status());
                    jv["exec_status"] = tx_exec_status_to_str(txaction->get_tx_exec_status() | recv_txinfo->get_tx_exec_status());
                }
            }
            if (txaction->is_recv_tx()) {
                // recv_txinfo = tx_info;  // TODO(jimmy) refactor here future
                m_transaction_cache->tx_set_recv_txinfo(txaction->get_tx_hash(), txaction);
            }

            base::enum_transaction_subtype type = txaction->get_tx_subtype();
            // xdbg("type:%d,%s", type, jv.toStyledString().c_str());
            // xdbg("ji:%s", ji.toStyledString().c_str());
            if (type == base::enum_transaction_subtype_self) {
                m_transaction_cache->tx_erase(txaction->get_tx_hash());
                continue;
            } else if (type == base::enum_transaction_subtype_send)
                ji["send_block_info"] = jv;
            else if (type == base::enum_transaction_subtype_recv)
                ji["recv_block_info"] = jv;
            else if (type == base::enum_transaction_subtype_confirm)
                ji["confirm_block_info"] = jv;
            else
                continue;
            m_transaction_cache->tx_set_json(txaction->get_tx_hash(), ji);

            if (type == base::enum_transaction_subtype_confirm)
                m_transaction_cache->tx_erase(txaction->get_tx_hash());
        }
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
