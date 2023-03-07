// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xevent_state_sync.h"
#include "xverifier/xverifier_utl.h"
#include "xvledger/xvledger.h"
#include "xvledger/xaccountindex.h"
#include "xstatestore/xstatestore_impl.h"
#include "xstatestore/xerror.h"
#include "xdata/xblocktool.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xevm_common/trie/xtrie_iterator.h"
#include "xevm_common/trie/xtrie_kv_db.h"

NS_BEG2(top, statestore)

xstatestore_face_t * xstatestore_hub_t::_static_statestore = nullptr;

#define statestore_mailbox_limit (8192)

xstatestore_face_t* xstatestore_hub_t::instance() {
    if(_static_statestore)
        return _static_statestore;

    _static_statestore = new xstatestore_impl_t();
    return _static_statestore;
}

xstatestore_face_t* xstatestore_hub_t::reset_instance() {
    if(_static_statestore)
        delete _static_statestore;
    _static_statestore = new xstatestore_impl_t();
    return _static_statestore;
}

xstatestore_impl_t::xstatestore_impl_t() {
    xdbg("xstatestore_impl_t::xstatestore_impl_t this=%p",this);
    m_para = std::make_shared<xstatestore_resources_t>();
    init_all_tablestate();
}

void xstatestore_impl_t::init_all_tablestate() {
    std::vector<std::string> table_addrs = data::xblocktool_t::make_all_table_addresses();
    for (auto & v : table_addrs) {
        xstatestore_table_ptr_t tablestore = std::make_shared<xstatestore_table_t>(common::xtable_address_t::build_from(v), m_para);
        m_table_statestore[v] = tablestore;
    }
}

bool xstatestore_impl_t::start(const xobject_ptr_t<base::xiothread_t> & iothread, const xobject_ptr_t<base::xiothread_t> & iothread_for_prune) {
    if (m_started) {
        xerror("xstatestore_impl_t::start already started.");
        return false;
    }
    m_started = true;
    m_prune_dispather = make_object_ptr<statestore_prune_dispatcher_t>(top::base::xcontext_t::instance(), iothread_for_prune->get_thread_id());
    m_para->set_prune_dispatcher(make_observer(m_prune_dispather.get()));

    // todo(nathan):use timer to update mpt and table state.
    m_timer = new xstatestore_timer_t(top::base::xcontext_t::instance(), iothread->get_thread_id(), this);
    m_timer->start(0, 1000);
    m_store_block_listen_id = get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&xstatestore_impl_t::on_block_to_db_event, this, std::placeholders::_1));
    m_state_sync_listen_id = get_mbus()->add_listener(top::mbus::xevent_major_type_state_sync, std::bind(&xstatestore_impl_t::on_state_sync_event, this, std::placeholders::_1));
    return false;
}

void xstatestore_impl_t::on_table_block_committed(const mbus::xevent_ptr_t & event) const {
    mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(event);
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(block_event->owner);

    if (tablestore->on_table_block_committed_by_height(block_event->blk_height, block_event->blk_hash)) {
        return;
    }
    const data::xblock_ptr_t & block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_txpool_db_event_on_block);
    tablestore->on_table_block_committed(block.get());
}

void xstatestore_impl_t::on_table_block_committed(base::xvblock_t* block) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(block->get_account());
    tablestore->on_table_block_committed(block);
}

xtablestate_ext_ptr_t xstatestore_impl_t::do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(current_block->get_account());
    return tablestore->do_commit_table_all_states(current_block, tablestate_store, ec);
}

void xstatestore_impl_t::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_committed) {
        return;
    }

    mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(e);

    if (block_event->blk_level != base::enum_xvblock_level_table) {
        return;
    }

    auto event_handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        mbus::xevent_object_t* _event_obj = dynamic_cast<mbus::xevent_object_t*>(call.get_param1().get_object());
        xassert(_event_obj != nullptr);
        on_table_block_committed(_event_obj->event);
        return true;
    };

    int64_t in, out;
    int32_t queue_size = m_timer->count_calls(in, out);
    XMETRICS_GAUGE_SET_VALUE(metrics::mailbox_statestore_cur, queue_size);
    bool discard = queue_size >= statestore_mailbox_limit;
    if (discard) {
        XMETRICS_GAUGE(metrics::mailbox_statestore_total, 0);
        return;
    }
    XMETRICS_GAUGE(metrics::mailbox_statestore_total, 1);

    base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
    base::xcall_t asyn_call(event_handler, event_obj.get());
    m_timer->send_call(asyn_call);
}

uint64_t xstatestore_impl_t::get_latest_executed_block_height(common::xtable_address_t const & table_address) const {
    xdbg("xstatestore_impl_t::get_latest_executed_block_height table:%s,this=%p", table_address.to_string().c_str(), this);
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_address.to_string());
    return tablestore->get_latest_executed_block_height();
}
uint64_t xstatestore_impl_t::get_need_sync_state_block_height(common::xtable_address_t const & table_address) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_address.to_string());
    return tablestore->get_need_sync_state_block_height();    
}

void xstatestore_impl_t::on_state_sync_result(mbus::xevent_state_sync_ptr_t state_sync_event) {
    auto const & table_addr = state_sync_event->table_addr.to_string();
    if (state_sync_event->ec) {
        xwarn("xstatestore_impl_t::on_state_sync_result fail-notify:table:%s,height:%llu,root:%s,ec=%s", 
            table_addr.c_str(),state_sync_event->height,state_sync_event->root_hash.hex().c_str(), state_sync_event->ec.message().c_str());
        XMETRICS_GAUGE(metrics::statestore_sync_succ, 0);
        return;
    }
    XMETRICS_GAUGE(metrics::statestore_sync_succ, 1);

    xstate_sync_info_t sync_info(state_sync_event->height,state_sync_event->root_hash,state_sync_event->table_state_hash, top::to_string(state_sync_event->table_block_hash.to_bytes()));
    xinfo("xstatestore_impl_t::on_state_sync_result succ-notify:table:%s,height:%llu,root:%s", 
        table_addr.c_str(),state_sync_event->height,state_sync_event->root_hash.hex().c_str());

    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_addr);
    tablestore->raise_execute_height(sync_info);
}

void xstatestore_impl_t::on_state_sync_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_state_sync_t::none) {
        xerror("xstatestore_impl_t::on_state_sync_event event minor type is invalid.");
        return;
    }

    // auto event_handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
    //     mbus::xevent_object_t* _event_obj = dynamic_cast<mbus::xevent_object_t*>(call.get_param1().get_object());
    //     xassert(_event_obj != nullptr);
    //     mbus::xevent_state_sync_ptr_t state_sync_event = dynamic_xobject_ptr_cast<mbus::xevent_state_sync_t>(_event_obj->event);
    //     on_state_sync_result(state_sync_event);
    //     return true;
    // };

    // base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
    // base::xcall_t asyn_call(event_handler, event_obj.get());
    // m_timer->send_call(asyn_call);

    // XTODO use sync mode to update execute height 
    mbus::xevent_state_sync_ptr_t state_sync_event = dynamic_xobject_ptr_cast<mbus::xevent_state_sync_t>(e);
    on_state_sync_result(state_sync_event);
}

xstatestore_table_ptr_t xstatestore_impl_t::get_table_statestore_from_unit_addr(common::xaccount_address_t const & account_address) const {
    base::xvaccount_t vaccount(account_address.vaccount());
    if (!vaccount.has_valid_table_addr()) {
        xwarn("xstatestore_impl_t::get_table_statestore_from_unit_addr fail. %s", account_address.to_string().c_str());
        return nullptr;
    }
    std::string table_address = base::xvaccount_t::make_table_account_address(account_address.vaccount());
    auto iter = m_table_statestore.find(table_address);
    if (iter != m_table_statestore.end()) {
        return iter->second;
    }
    xassert(false);
    return nullptr;
}

xstatestore_table_ptr_t xstatestore_impl_t::get_table_statestore_from_table_addr(std::string const & table_addr) const {
    auto iter = m_table_statestore.find(table_addr);
    if (iter != m_table_statestore.end()) {
        return iter->second;
    }
    xassert(false);
    return nullptr;
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_state_from_block(common::xaccount_address_t const & account_address, base::xvblock_t * target_block) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_unit_addr(account_address);
    auto unitstate = tablestore->get_unit_state_from_block(account_address, target_block);
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_succ, unitstate != nullptr ? 1 : 0);
    return unitstate;
}

xtablestate_ext_ptr_t xstatestore_impl_t::get_tablestate_ext_from_block_inner(base::xvblock_t* target_block, bool bstate_must) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(target_block->get_account());
    auto tablestate = tablestore->get_tablestate_ext_from_block(target_block, bstate_must);
    XMETRICS_GAUGE(metrics::statestore_get_table_state_succ, tablestate != nullptr ? 1 : 0);
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_impl_t::get_tablestate_ext_from_block(base::xvblock_t* target_block) const {
    return get_tablestate_ext_from_block_inner(target_block, true);
}

data::xtablestate_ptr_t xstatestore_impl_t::get_table_state_by_block(base::xvblock_t * target_block) const {
    xtablestate_ext_ptr_t tablestate_ext = get_tablestate_ext_from_block_inner(target_block, true);
    if (nullptr != tablestate_ext) {
        return tablestate_ext->get_table_state();
    }
    return nullptr;
}

bool xstatestore_impl_t::get_accountindex_from_latest_connected_table(common::xaccount_address_t const & table_address, common::xaccount_address_t const & account_address, base::xaccount_index_t& account_index) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_address.to_string());

    auto tablestate = tablestore->get_latest_connectted_table_state();
    if (tablestate == nullptr) {
        xerror("xstatestore_impl_t::get_accountindex_from_latest_connected_table fail-load latest state. table=%s,account=%s",
               table_address.to_string().c_str(),
               account_address.to_string().c_str());
        return false;
    }    
    std::error_code ec;
    tablestate->get_accountindex(account_address.to_string(), account_index, ec);
    if (ec) {
        xerror("xstatestore_impl_t::get_accountindex_from_latest_connected_table fail-load accountindex. table=%s,account=%s",
               table_address.to_string().c_str(),
               account_address.to_string().c_str());
        return false;
    }
    return true;
}

bool xstatestore_impl_t::get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const {
    XMETRICS_TIME_RECORD("statestore_getindex_cost");
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_block->get_account());
    if (tablestore != nullptr) {
        return tablestore->get_accountindex_from_table_block(account_address, table_block, account_index);
    }
    xwarn("xstatestore_impl_t::get_accountindex_from_table_block fail.block=%s", table_block->dump().c_str());
    return false;
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_latest_connectted_change_state(common::xaccount_address_t const & account_address) const {
    auto _block = get_latest_connectted_state_changed_block(get_blockstore(), account_address.vaccount());
    if (nullptr == _block) {
        xerror("xstatestore_impl_t::get_unit_latest_connectted_change_state fail-get block");
        return nullptr;
    }

    return get_unit_state_from_block(account_address, _block.get());
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unitstate(xblock_number_t number, common::xaccount_address_t const & account_address) const {
    data::xaccountstate_ptr_t accountstate = get_accountstate(number, account_address);
    if (nullptr != accountstate) {
        return accountstate->get_unitstate();
    }
    return nullptr;
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_latest_connectted_state(common::xaccount_address_t const & account_address) const {
    // auto _block = get_blockstore()->get_latest_connected_block(account_address.vaccount());
    // if (_block == nullptr) {
    //     xerror("xstatestore_impl_t::get_unit_latest_connectted_state fail-load latest connectted block. account=%s", account_address.to_string().c_str());
    //     return nullptr;
    // }

    // if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
    //     xwarn("xstatestore_impl_t::get_unit_latest_connectted_state fail-invalid state for empty genesis block. account=%s", account_address.to_string().c_str());
    //     return nullptr;
    // }
    // return get_unit_state_from_block(account_address, _block.get());

    base::xvaccount_t vaccount(account_address.vaccount());
    if (!vaccount.has_valid_table_addr()) {
        return nullptr;
    }
    std::string table_address = base::xvaccount_t::make_table_account_address(vaccount);
    common::xaccount_address_t _table_addr(table_address);

    base::xaccount_index_t account_index;
    auto ret = get_accountindex_from_latest_connected_table(_table_addr, account_address, account_index);
    if (!ret) {
        return nullptr;
    }

    return get_unit_state_by_accountindex(account_address, account_index);
}

data::xunitstate_ptr_t  xstatestore_impl_t::get_unit_committed_changed_state(common::xaccount_address_t const & account_address, uint64_t max_height) const {
    auto _block = get_committed_state_changed_block(get_blockstore(), account_address.vaccount(), max_height);
    if (nullptr == _block) {
        xwarn("xstatestore_impl_t::get_unit_committed_changed_state fail-get block");
        return nullptr;
    }
    return get_unit_state_from_block(account_address, _block.get());
}

data::xunitstate_ptr_t  xstatestore_impl_t::get_unit_committed_state(common::xaccount_address_t const & account_address, uint64_t height) const {
    auto _block = get_blockstore()->load_block_object(account_address.vaccount(), height, base::enum_xvblock_flag_committed, false);
    if (nullptr == _block) {
        xwarn("xstatestore_impl_t::get_unit_committed_state fail-get block.%s,height=%ld", account_address.to_string().c_str(), height);
        return nullptr;
    }
    if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
        xwarn("xstatestore_impl_t::get_unit_committed_state fail-invalid state for empty genesis block. account=%s", account_address.to_string().c_str());
        return nullptr;
    }    
    return get_unit_state_from_block(account_address, _block.get());
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_state_by_accountindex(common::xaccount_address_t const & account_address, base::xaccount_index_t const& account_index) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_unit_addr(account_address);
    return tablestore->get_unit_state_from_accountindex(account_address, account_index);
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_state_by_unit_block(base::xvblock_t * target_block) const {
    common::xaccount_address_t account_address(target_block->get_account());
    return get_unit_state_from_block(account_address, target_block);
}

bool xstatestore_impl_t::get_accountindex(xblock_number_t number, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const {
    base::xvaccount_t vaccount(account_address.vaccount());
    if (!vaccount.has_valid_table_addr()) {
        xwarn("xstatestore_impl_t::get_accountindex fail-invalid addr.%s",account_address.to_string().c_str());
        return false;
    }
    std::string table_addr = base::xvaccount_t::make_table_account_address(account_address.vaccount());
    common::xaccount_address_t table_address(table_addr);

    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_addr);

    if (number == LatestConnectBlock) {
        auto tablestate = tablestore->get_latest_connectted_table_state();
        if (tablestate == nullptr) {
            xerror("xstatestore_impl_t::get_accountindex fail-load state. table=%s,account=%s",table_address.to_string().c_str(),account_address.to_string().c_str());
            return false;
        }
        std::error_code ec;
        tablestate->get_accountindex(account_address.to_string(), account_index, ec);
        if (ec) {
            xerror("xstatestore_impl_t::get_accountindex fail-load accountindex. table=%s,account=%s",table_address.to_string().c_str(),account_address.to_string().c_str());
            return false;
        }
        return true;
    }

    // TODO(jimmy) get latest commit and cert executed state
    xobject_ptr_t<base::xvblock_t> _block = nullptr;
    if (number == LatestBlock || number == PendingBlock) {
        // _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(table_address.vaccount());
        return get_accountindex_from_latest_connected_table(table_address, account_address, account_index);
    } else if (number == 0) {
        _block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(table_address.vaccount());
    } else {
        uint64_t commit_height = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block_height(table_address.vaccount());
        if (number > commit_height) {
            _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(table_address.vaccount(), number, base::enum_xvblock_flag_authenticated, false);
        } else {
            _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(table_address.vaccount(), number, base::enum_xvblock_flag_committed, false);
        }
    }
    if (nullptr == _block) {
        xwarn("xstatestore_impl_t::get_accountindex fail-get block.table=%s,account=%sheight=%ld", table_address.to_string().c_str(),account_address.to_string().c_str(), number);
        return false;
    }
    return tablestore->get_accountindex_from_table_block(account_address, _block.get(), account_index);
}

std::vector<std::pair<common::xaccount_address_t, base::xaccount_index_t>> xstatestore_impl_t::get_all_accountindex(base::xvblock_t * table_block, std::error_code & ec) const {
    auto tablestate_ext = get_tablestate_ext_from_block_inner(table_block, false);// not need table bstate
    if (nullptr == tablestate_ext) {
        ec = error::xerrc_t::statestore_load_tablestate_err;
        return {};
    }

    auto state_root = tablestate_ext->get_state_mpt()->get_original_root_hash();
    if (state_root.empty()) {
        if (tablestate_ext->get_table_state() == nullptr) {
            ec = error::xerrc_t::statestore_load_tablestate_err;
            return {};
        }
        return tablestate_ext->get_table_state()->all_accounts();
    } else {
        common::xtable_address_t table_addr = common::xtable_address_t::build_from(table_block->get_account());
        std::vector<std::pair<common::xaccount_address_t, base::xaccount_index_t>> accounts_index;
        auto const kv_db = std::make_shared<evm_common::trie::xkv_db_t>(base::xvchain_t::instance().get_xdbstore(), table_addr);
        auto xtrie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
        auto const & leafs = top::evm_common::trie::xtrie_simple_iterator_t::trie_leafs(state_root, make_observer(xtrie_db));
        for (auto & leaf : leafs) {
            state_mpt::xaccount_info_t info;
            info.decode({leaf.begin(), leaf.end()});
            accounts_index.emplace_back(info.m_account, info.m_index);
        }
        return accounts_index;
    }
}

data::xaccountstate_ptr_t xstatestore_impl_t::get_accountstate(xblock_number_t number, common::xaccount_address_t const & account_address) const {
    base::xaccount_index_t account_index;
    bool ret = get_accountindex(number, account_address, account_index);
    if (false == ret) {
        xwarn("xstatestore_impl_t::get_accountstate fail-get accountindex.number=%ld,account=%s", number,account_address.to_string().c_str());
        return nullptr;
    }
    data::xunitstate_ptr_t unitstate = get_unit_state_by_accountindex(account_address, account_index);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::get_accountstate fail-get unitstate.number=%ld,account=%s,index=%s", number,account_address.to_string().c_str(),account_index.dump().c_str());
        return nullptr;
    }
    return std::make_shared<data::xaccount_state_t>(unitstate, account_index);
}

bool xstatestore_impl_t::get_accountindex(const std::string& table_height, common::xaccount_address_t const & account_address, base::xaccount_index_t & account_index) const {
    xblock_number_t number;
    if (table_height == "latest")
        number = LatestBlock;
    else if (table_height == "earliest")
        number = 0;
    else if (table_height == "pending")
        number = PendingBlock;
    else {
        number = std::strtoul(table_height.c_str(), NULL, 16);
    }
    
    return get_accountindex(number, account_address, account_index);
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_state_by_table(common::xaccount_address_t const & account_address, const std::string& table_height) const {
    base::xaccount_index_t accountindex;
    if (false == get_accountindex(table_height, account_address, accountindex)) {
        xwarn("xstatestore_impl_t::get_unit_state_by_table fail-get accountindex.%s,height=%s", account_address.to_string().c_str(), table_height.c_str());
        return nullptr;
    }

    return get_unit_state_by_accountindex(account_address, accountindex);
}

uint64_t xstatestore_impl_t::get_blockchain_height(common::xaccount_address_t const & account_address) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::map_get fail-find account. account=%s", account_address.to_string().c_str());
        return 0;
    }
    return unitstate->height();
}

int32_t xstatestore_impl_t::map_get(common::xaccount_address_t const & account_address, const std::string &key, const std::string &field, std::string &value) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::map_get fail-find account. account=%s", account_address.to_string().c_str());
        return -1;
    }
    return unitstate->map_get(key, field, value);
}

int32_t xstatestore_impl_t::string_get(common::xaccount_address_t const & account_address, const std::string& key, std::string& value) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::string_get fail-find account. account=%s", account_address.to_string().c_str());
        return -1;
    }
    return unitstate->string_get(key, value);
}

int32_t xstatestore_impl_t::map_copy_get(common::xaccount_address_t const & account_address, const std::string &key, std::map<std::string, std::string> &map) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::map_copy_get fail-find account. account=%s", account_address.to_string().c_str());
        return -1;
    }
    return unitstate->map_copy_get(key, map);
}

int32_t xstatestore_impl_t::get_map_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string &name, std::map<std::string, std::string> &value) const {
    auto unitstate = get_unit_committed_state(account_address, height);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::get_map_property fail-find account. account=%s", account_address.to_string().c_str());
        return -1;
    }

    return unitstate->map_copy_get(name, value);
}

int32_t xstatestore_impl_t::get_string_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string &name, std::string &value) const {
    auto unitstate = get_unit_committed_state(account_address, height);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::get_string_property fail-find account. account=%s", account_address.to_string().c_str());
        return -1;
    }

    return unitstate->string_get(name, value);
}


base::xauto_ptr<base::xvblock_t> xstatestore_impl_t::get_latest_connectted_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account) {
    // TODO(jimmy) check if the binlog hash exists
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->get_latest_connected_block(account);
    if (vblock->is_state_changed_unit()) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(account, current_height - 1, base::enum_xvblock_flag_committed, false);
        if (prev_vblock == nullptr) {
            xwarn("xstatestore_impl_t::get_latest_connectted_state_changed_block fail-load unit.%s,height=%ld",account.get_account().c_str(), current_height - 1);
            return prev_vblock;
        }
        if (prev_vblock->is_state_changed_unit()) {
            return prev_vblock;
        }
        current_height = prev_vblock->get_height();
    }
    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xstatestore_impl_t::get_committed_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account, uint64_t max_height) {
    // there is mostly two empty units
    XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->load_block_object(account, max_height, base::enum_xvblock_flag_committed, false);
    xassert(vblock->check_block_flag(base::enum_xvblock_flag_committed));
    if (vblock->is_state_changed_unit()) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(account, current_height - 1, base::enum_xvblock_flag_committed, false);
        if (prev_vblock == nullptr) {
            xwarn("xstatestore_impl_t::get_committed_state_changed_block fail-load unit.%s,height=%ld",account.get_account().c_str(), current_height - 1);
            return prev_vblock;
        }

        if (prev_vblock->is_state_changed_unit()) {
            return prev_vblock;
        }

        current_height = prev_vblock->get_height();
    }
    return nullptr;
}

data::xtablestate_ptr_t xstatestore_impl_t::get_table_connectted_state(common::xaccount_address_t const & table_address) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_address.to_string());
    auto tablestate = tablestore->get_latest_connectted_table_state();
    if (nullptr != tablestate) {
        return tablestate->get_table_state();
    }
    return nullptr;
}

bool xstatestore_impl_t::get_receiptid_state_and_prove(common::xaccount_address_t const & table_address,
                                            base::xvblock_t * latest_commit_block,
                                            base::xvproperty_prove_ptr_t & property_prove_ptr,
                                            data::xtablestate_ptr_t & tablestate_ptr) const {
    base::enum_xvblock_class block_class = latest_commit_block->get_block_class();
    uint32_t nil_block_num = 0;
    base::xvblock_ptr_t non_nil_commit_block = nullptr;
    uint64_t height = latest_commit_block->get_height();
    if (block_class != base::enum_xvblock_class_nil) {
        base::xvblock_t * _block = latest_commit_block;
        _block->add_ref();
        non_nil_commit_block.attach(_block);
    } else {
        while (block_class == base::enum_xvblock_class_nil && height > 0) {
            nil_block_num++;
            if (nil_block_num > 2) {
                xerror("xstatestore_impl_t::get_receiptid_state_and_prove, continuous nil table block number is more than 2,table:%s,height:%llu",
                       table_address.to_string().c_str(),
                       height);
                return false;
            }

            auto commit_block =
                get_blockstore()->load_block_object(table_address.vaccount(), height - 1, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_txpool_id_state);
            if (commit_block == nullptr) {
                xwarn("xstatestore_impl_t::get_receiptid_state_and_prove load block fail,table:%s,height:%llu", table_address.to_string().c_str(), height - 1);
                return false;
            }
            height = commit_block->get_height();

            block_class = commit_block->get_block_class();
            if (block_class != base::enum_xvblock_class_nil) {
                base::xvblock_t * _block = commit_block.get();
                _block->add_ref();
                non_nil_commit_block.attach(_block);
                break;
            }
        }
    }

    if (non_nil_commit_block == nullptr) {
        xinfo("xstatestore_impl_t::get_receiptid_state_and_prove latest commit height is 0, no need send receipt id state.table:%s", table_address.to_string().c_str());
        return false;
    }

    data::xtablestate_ptr_t tablestate = get_table_state_by_block(non_nil_commit_block.get());
    if (tablestate == nullptr) {
        xwarn("xstatestore_impl_t::get_receiptid_state_and_prove table:%s,height:%llu,get bstate fail", table_address.to_string().c_str(), non_nil_commit_block->get_height());
        return false;
    }

    if (tablestate->get_receiptid_state()->get_all_receiptid_pairs()->get_all_pairs().empty()) {
        xinfo("xstatestore_impl_t::get_receiptid_state_and_prove table have no receipt id pairs.table:%s, commit height:%llu",
              table_address.to_string().c_str(),
              non_nil_commit_block->get_height());
        return false;
    }

    auto cert_block = get_blockstore()->load_block_object(table_address.vaccount(), non_nil_commit_block->get_height() + 2, 0, false);
    if (cert_block == nullptr) {
        xinfo("xstatestore_impl_t::get_receiptid_state_and_prove cert block load fail.table:%s, cert height:%llu",
              table_address.to_string().c_str(),
              non_nil_commit_block->get_height() + 2);
        return false;
    }

    if (false == get_blockstore()->load_block_input(table_address.vaccount(), non_nil_commit_block.get())) {
        xerror("xstatestore_impl_t::get_receiptid_state_and_prove fail load block input.table:%s, height:%llu",
              table_address.to_string().c_str(),
              non_nil_commit_block->get_height());
        return false;
    }

    auto property_prove = base::xpropertyprove_build_t::create_property_prove(non_nil_commit_block.get(), cert_block.get(), tablestate->get_bstate().get(), data::XPROPERTY_TABLE_RECEIPTID);
    if (property_prove == nullptr) {
        xwarn("xstatestore_impl_t::get_receiptid_state_and_prove create receipt state fail 2.table:%s, commit height:%llu",
              table_address.to_string().c_str(),
              non_nil_commit_block->get_height());
        return false;
    }
    xassert(property_prove->is_valid());
    property_prove_ptr = property_prove;
    tablestate_ptr = tablestate;
    return true;
}

base::xvblockstore_t*  xstatestore_impl_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

mbus::xmessage_bus_t * xstatestore_impl_t::get_mbus() const {
    return (mbus::xmessage_bus_t *)base::xvchain_t::instance().get_xevmbus();
}

// void xstatestore_impl_t::prune() {
//     uint64_t now = xverifier::xtx_utl::get_gmttime_s();
//     if ((now % state_prune_freq) == 0)
//     for (auto & table_statestore_pair : m_table_statestore) {
//         auto & table_statestore = table_statestore_pair.second;
//         table_statestore->state_prune();
//     }
// }

bool xstatestore_timer_t::on_timer_fire(const int32_t thread_id,
                                        const int64_t timer_id,
                                        const int64_t current_time_ms,
                                        const int32_t start_timeout_ms,
                                        int32_t & in_out_cur_interval_ms) {

    // m_statestore->prune();
    return true;
}

NS_END2
