// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xmbus/xevent_store.h"
#include "xvledger/xvledger.h"
#include "xvledger/xaccountindex.h"
#include "xstatestore/xstatestore_impl.h"
#include "xdata/xblocktool.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmbus/xevent_store.h"

NS_BEG2(top, statestore)

xstatestore_face_t* xstatestore_hub_t::instance() {
    static xstatestore_face_t * _static_statestore = nullptr;
    if(_static_statestore)
        return _static_statestore;

    _static_statestore = new xstatestore_impl_t();
    return _static_statestore;
}

xstatestore_impl_t::xstatestore_impl_t() {
    init_all_tablestate();
}

void xstatestore_impl_t::init_all_tablestate() {
    std::vector<std::string> table_addrs = data::xblocktool_t::make_all_table_addresses();
    for (auto & v : table_addrs) {
        xstatestore_table_ptr_t tablestore = std::make_shared<xstatestore_table_t>(common::xaccount_address_t(v));
        m_table_statestore[v] = tablestore;
    }
}

bool xstatestore_impl_t::start(const xobject_ptr_t<base::xiothread_t> & iothread) {
    if (m_started) {
        xerror("xstatestore_impl_t::start already started.");
        return false;
    }
    m_started = true;

    // todo(nathan):use timer to update mpt and table state.
    m_timer = new xstatestore_timer_t(top::base::xcontext_t::instance(), iothread->get_thread_id(), this);
    m_timer->start(0, 1000);
    m_bus_listen_id = get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&xstatestore_impl_t::on_block_to_db_event, this, std::placeholders::_1));
    return false;
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
        mbus::xevent_store_block_committed_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_committed_t>(_event_obj->event);
        const data::xblock_ptr_t & block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_txpool_db_event_on_block);
        evm_common::xh256_t root_hash;
        execute_table_block(block.get());
        return true;
    };

    base::xauto_ptr<mbus::xevent_object_t> event_obj = new mbus::xevent_object_t(e, 0);
    base::xcall_t asyn_call(event_handler, event_obj.get());
    m_timer->send_call(asyn_call);
}

void xstatestore_impl_t::set_latest_executed_info(const base::xvaccount_t & table_addr, uint64_t height,const std::string & blockhash) const
{
    // TODO(jimmy) no need set executed block hash xvchain_t::instance().get_xblockstore()->set_latest_executed_info(table_addr, height, blockhash);
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr));
    account_obj->set_latest_executed_block(height, blockhash);
}
uint64_t xstatestore_impl_t::get_latest_executed_block_height(const base::xvaccount_t & table_addr) const
{
    // base::xvchain_t::instance().get_xblockstore()->get_latest_executed_block_height(table_addr);
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr));
    return account_obj->get_latest_executed_block_height();
}

bool xstatestore_impl_t::get_mpt(base::xvblock_t * block, xhash256_t & root_hash, std::shared_ptr<state_mpt::xtop_state_mpt> & mpt) const {
    evm_common::xh256_t state_root;
    auto ret = data::xblockextract_t::get_state_root(block, state_root);
    if (!ret) {
        xwarn("xstatestore_impl_t::get_mpt get state root fail. block:%s", block->dump().c_str());
        return false;
    }
    std::error_code ec;
    root_hash = xhash256_t(state_root.to_bytes());
    mpt = state_mpt::xtop_state_mpt::create(root_hash, base::xvchain_t::instance().get_xdbstore(), block->get_account(), ec);
    if (ec) {
        return false;
    }

    return true;
}

uint64_t xstatestore_impl_t::try_update_execute_height(const base::xvaccount_t & table_addr, uint64_t max_count) const {
    uint64_t old_execute_height = get_latest_executed_block_height(table_addr);
    uint64_t _highest_commit_block_height = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block_height(table_addr);

    if (old_execute_height >= _highest_commit_block_height) {
        return old_execute_height;
    }

    if (!is_archive_node() && _highest_commit_block_height >= old_execute_height + 1000) {
        xinfo("xstatestore_impl_t::try_update_execute_height fall behind too much. hight:%llu,%llu", _highest_commit_block_height, old_execute_height);
        // todo(nathan): trigger mpt sync if not archive node, is _highest_commit_block_height >= old_execute_height + 1000
        return old_execute_height;
    }

    uint64_t _begin_height = old_execute_height == 0 ? 0 : old_execute_height + 1;
    uint64_t new_execute_height = old_execute_height;
    std::string new_execute_hash;
    uint64_t height = _begin_height;

    xhash256_t pre_root_hash;

    bool ret = false;
    std::shared_ptr<state_mpt::xtop_state_mpt> pre_mpt = nullptr;
    bool pre_mpt_committed = false;
    do {
        if (height > _highest_commit_block_height) {
            xdbg("xstatestore_impl_t::try_update_execute_height finish. account=%s,height=%ld,_highest_commit_block_height=%ld,old_execute_height=%ld,new_execute_height=%ld",
                 table_addr.get_account().c_str(),
                 height,
                 _highest_commit_block_height,
                 old_execute_height,
                 new_execute_height);
            break;
        }

        auto block = base::xvchain_t::instance().get_xblockstore()->load_block_object(
            table_addr, height, base::enum_xvblock_flag::enum_xvblock_flag_committed, false, (int)metrics::blockstore_access_from_statestore_get_commit_state);
        if (block == nullptr) {
            xwarn("xstatestore_impl_t::try_update_execute_height fail-load committed block. account=%s,height=%ld", table_addr.get_account().c_str(), height);
            break;
        }

        std::shared_ptr<state_mpt::xtop_state_mpt> mpt;
        xhash256_t root_hash;
        ret = get_mpt(block.get(), root_hash, mpt);
        if (ret) {
            pre_mpt_committed = false;
            pre_mpt = mpt;
        } else {
            if (height == _begin_height && height > 1) {
                auto pre_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(
                    table_addr, height - 1, base::enum_xvblock_flag::enum_xvblock_flag_committed, false, (int)metrics::blockstore_access_from_statestore_get_commit_state);
                if (pre_block == nullptr) {
                    xerror("xstatestore_impl_t::try_update_execute_height fail-load committed block. account=%s,height=%ld", table_addr.get_account().c_str(), height - 1);
                    break;
                }
                ret = get_mpt(pre_block.get(), pre_root_hash, pre_mpt);
                if (!ret) {
                    xerror("xstatestore_impl_t::try_update_execute_height get_block_and_mpt fail.");
                    break;
                }
            }

            std::error_code ec;
            if (pre_mpt_committed) {
                pre_mpt = state_mpt::xtop_state_mpt::create(pre_root_hash, base::xvchain_t::instance().get_xdbstore(), table_addr.get_account(), ec);
                if (ec) {
                    xerror("xstatestore_impl_t::try_update_execute_height get_block_and_mpt fail.");
                    break;
                }
            }

            ret = set_and_commit_mpt(block.get(), root_hash, pre_mpt, pre_mpt_committed);
            if (!ret) {
                break;
            }
        }

        auto state = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block.get());
        if (state == nullptr) {
            break;
        }

        pre_root_hash.clear();
        pre_root_hash = root_hash;
        new_execute_height = block->get_height();
        new_execute_hash = block->get_block_hash();
        height++;
    } while (max_count-- > 0);

    if (new_execute_height > old_execute_height) {
        set_latest_executed_info(table_addr, new_execute_height, new_execute_hash);
        xinfo("xstatestore_impl_t::try_update_execute_height succ-update. account=%s,height=%ld,old_execute_height=%ld,new_execute_height=%ld,commit_height=%ld",
              table_addr.get_account().c_str(),
              _begin_height,
              old_execute_height,
              new_execute_height,
              _highest_commit_block_height);
    } else {
        xdbg("xstatestore_impl_t::try_update_execute_height finish2. account=%s,_begin_height=%ld,_highest_commit_block_height=%ld,old_execute_height=%ld,new_execute_height=%ld",
             table_addr.get_account().c_str(),
             _begin_height,
             _highest_commit_block_height,
             old_execute_height,
             new_execute_height);
    }

    return new_execute_height;
}

bool xstatestore_impl_t::set_and_commit_mpt(base::xvblock_t * block, const xhash256_t root_hash, std::shared_ptr<state_mpt::xtop_state_mpt> pre_mpt, bool & mpt_committed) const {
    base::xvaccount_t table_addr(block->get_account());
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_output(table_addr, block, metrics::blockstore_access_from_statestore_load_state)) {
        xerror("xstatestore_impl_t::set_and_commit_mpt fail-load block input output, block=%s", block->dump().c_str());
        return false;
    }

    auto account_indexs_str = block->get_account_indexs();
    if (!account_indexs_str.empty()) {
        data::xtable_account_indexs_t account_indexs;
        account_indexs.serialize_from_string(account_indexs_str);
        std::error_code ec;
        for (auto & index : account_indexs.get_account_indexs()) {
            pre_mpt->set_account_index(index.first, index.second, ec);
            if (ec) {
                xerror("xstatestore_impl_t::set_and_commit_mpt set account index from table property to mpt fail.block:%s", block->dump().c_str());
                return false;
            }
        }

        ec.clear();
        // check if root matches.
        auto cur_root_hash = pre_mpt->get_root_hash(ec);
        if (ec || cur_root_hash != root_hash) {
            xerror("xstatestore_impl_t::set_and_commit_mpt hash not match cur_root_hash:%s,state_root_hash:%s,block:%s",
                cur_root_hash.as_hex_str().c_str(),
                root_hash.as_hex_str().c_str(),
                block->dump().c_str());
            return false;
        }

        pre_mpt->commit(ec);
        if (ec) {
            xdbg("xstatestore_impl_t::set_and_commit_mpt commit fail");
            return false;
        }
        xdbg("xstatestore_impl_t::set_and_commit_mpt commit hash:%s", cur_root_hash.as_hex_str().c_str());

        mpt_committed = true;
    } else {
        mpt_committed = false;
    }
    return true;
}

bool xstatestore_impl_t::execute_block_recurse(base::xvblock_t * block, const xhash256_t root_hash, uint64_t executed_height) const {
    xdbg("xstatestore_impl_t::execute_block_recurse begin.block=%s,execute_height=%ld",block->dump().c_str(), executed_height);
    if (block->get_height() <= executed_height) {
        xdbg("xstatestore_impl_t::execute_block_recurse finish-block less than execute.account=%s,height=%ld,execute_height=%ld",block->get_account().c_str(),block->get_height(),executed_height);
        return false;
    }

    base::xvaccount_t table_addr(block->get_account());
    uint64_t pre_height = block->get_height() - 1;
    xassert(block->get_height() > 0);
    auto pre_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(
            table_addr, pre_height, block->get_last_block_hash(), false, (int)metrics::blockstore_access_from_statestore_get_block_state);
    if (pre_block == nullptr) {
        xwarn("xstatestore_impl_t::execute_block_recurse fail-load committed block. account=%s,height=%ld", table_addr.get_account().c_str(), pre_height);
        return false;
    }

    std::shared_ptr<state_mpt::xtop_state_mpt> pre_mpt;
    xhash256_t pre_root_hash;
    auto ret = get_mpt(pre_block.get(), pre_root_hash, pre_mpt);
    if (!ret) {
        xwarn("xstatestore_impl_t::execute_block_recurse fail-get_mpt first time. block=%s", block->dump().c_str());
        ret = execute_block_recurse(pre_block.get(), pre_root_hash, executed_height);
        if (!ret) {
            return false;
        }
        ret = get_mpt(pre_block.get(), pre_root_hash, pre_mpt);
        if (!ret) {
            xerror("xstatestore_impl_t::execute_block_recurse fail-get_mpt second time. block=%s", block->dump().c_str());
            return false;
        }
    }

    bool mpt_committed;
    ret = set_and_commit_mpt(block, root_hash, pre_mpt, mpt_committed);
    if (!ret) {
        xwarn("xstatestore_impl_t::execute_block_recurse fail-set_and_commit_mpt. block=%s", block->dump().c_str());
        return false;
    }
    auto state = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block);
    if (state == nullptr) {
        xerror("xstatestore_impl_t::execute_block_recurse fail-get_block_state. block=%s", block->dump().c_str());
        return false;
    }
    xdbg("xstatestore_impl_t::execute_block_recurse succ.block=%s,execute_height=%ld",block->dump().c_str(), executed_height);
    return true;
}

bool xstatestore_impl_t::execute_table_block(base::xvblock_t * block) const {
    base::xvaccount_t table_addr(block->get_account());
    uint64_t execute_height = get_latest_executed_block_height(table_addr);
    xdbg("xstatestore_impl_t::execute_table_block begin.block=%s,execute_height=%ld",block->dump().c_str(), execute_height);
    auto height = block->get_height();
    if (height <= execute_height) {
        return true;
    }

    if (height > execute_height + 4) {  // TODO(jimmy)
        push_try_execute_table(block->get_account());
        xdbg("xstatestore_impl_t::execute_table_block block height(%llu) > execute height(%llu) + 2", height, execute_height);
        return false;
    }

    std::shared_ptr<state_mpt::xtop_state_mpt> mpt;
    xhash256_t root_hash;
    auto ret = get_mpt(block, root_hash, mpt);
    if (ret) {
        auto state = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block);
        if (state == nullptr) {
            return false;
        }
        xdbg("xstatestore_impl_t::execute_table_block succ-direct.block=%s,execute_height=%ld",block->dump().c_str(), execute_height);
        return true;
    }

    return execute_block_recurse(block, root_hash, execute_height);
}

xstatestore_table_ptr_t xstatestore_impl_t::get_table_statestore_from_unit_addr(common::xaccount_address_t const & account_address) const {
    std::string table_address = base::xvaccount_t::make_table_account_address(account_address.vaccount());
    auto iter = m_table_statestore.find(table_address);
    if (iter != m_table_statestore.end()) {
        return iter->second;
    }
    xassert(false);
    return nullptr;
}

xstatestore_table_ptr_t xstatestore_impl_t::get_table_statestore_from_table_addr(common::xaccount_address_t const & table_address) const {
    auto iter = m_table_statestore.find(table_address.value());
    if (iter != m_table_statestore.end()) {
        return iter->second;
    }
    xassert(false);
    return nullptr;
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_state_from_block(common::xaccount_address_t const & account_address, base::xvblock_t * target_block) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_unit_addr(account_address);
    return tablestore->get_unit_state_from_block(target_block);
}

bool xstatestore_impl_t::get_accountindex_from_latest_connected_table(common::xaccount_address_t const & account_address, base::xaccount_index_t& account_index) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_unit_addr(account_address);
    auto table_block = get_blockstore()->get_latest_connected_block(tablestore->get_table_address().vaccount());
    if (table_block == nullptr) {
        xerror("xstatestore_impl_t::get_accountindex_from_latest_connected_table fail-load latest connectted block. account=%s", account_address.value().c_str());
        return false;
    }    
    return get_accountindex_from_table_block(account_address, table_block.get(), account_index);
}

bool xstatestore_impl_t::get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_unit_addr(account_address);
    return tablestore->get_accountindex_from_table_block(account_address, table_block, account_index);
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_latest_connectted_change_state(common::xaccount_address_t const & account_address) const {
    auto _block = get_latest_connectted_state_changed_block(get_blockstore(), account_address.vaccount());
    if (nullptr == _block) {
        xerror("xstatestore_impl_t::get_unit_latest_connectted_change_state fail-get block");
        return nullptr;
    }

    return get_unit_state_from_block(account_address, _block.get());
}

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_latest_connectted_state(common::xaccount_address_t const & account_address) const {
    auto _block = get_blockstore()->get_latest_connected_block(account_address.vaccount());
    if (_block == nullptr) {
        xerror("xstatestore_impl_t::get_unit_latest_connectted_state fail-load latest connectted block. account=%s", account_address.value().c_str());
        return nullptr;
    }

    if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
        xwarn("xstatestore_impl_t::get_unit_latest_connectted_state fail-invalid state for empty genesis block. account=%s", account_address.value().c_str());
        return nullptr;
    }
    return get_unit_state_from_block(account_address, _block.get());
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
        xwarn("xstatestore_impl_t::get_unit_committed_state fail-get block.%s,height=%ld",account_address.value().c_str(), height);
        return nullptr;
    }
    if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
        xwarn("xstatestore_impl_t::get_unit_committed_state fail-invalid state for empty genesis block. account=%s", account_address.value().c_str());
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

data::xunitstate_ptr_t xstatestore_impl_t::get_unit_state_by_table(common::xaccount_address_t const & account_address, const std::string& table_height) const {
    std::string table_address = base::xvaccount_t::make_table_account_address(account_address.vaccount());
    common::xaccount_address_t _table_addr(table_address);

    xobject_ptr_t<base::xvblock_t> _block;
    if (table_height == "latest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr.vaccount());
    else if (table_height == "earliest")
        _block = base::xvchain_t::instance().get_xblockstore()->get_genesis_block(_table_addr.vaccount());
    else if (table_height == "pending")
        _block = base::xvchain_t::instance().get_xblockstore()->get_latest_cert_block(_table_addr.vaccount());
    else {
        uint64_t height = std::strtoul(table_height.c_str(), NULL, 16);
        _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_table_addr.vaccount(), height, base::enum_xvblock_flag_committed, false);
    }

    if (nullptr == _block) {
        xwarn("xstatestore_impl_t::get_unit_state_by_table fail-get table block.%s,height=%s",account_address.value().c_str(), table_height.c_str());
        return nullptr;
    }

    execute_table_block(_block.get());  // TODO(jimmy)  try to execute current block
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_unit_addr(account_address);
    return tablestore->get_unit_state_from_table_block(account_address, _block.get());
}

uint64_t xstatestore_impl_t::get_blockchain_height(common::xaccount_address_t const & account_address) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::map_get fail-find account. account=%s", account_address.value().c_str());
        return 0;
    }
    return unitstate->get_block_height();
}

int32_t xstatestore_impl_t::map_get(common::xaccount_address_t const & account_address, const std::string &key, const std::string &field, std::string &value) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::map_get fail-find account. account=%s", account_address.value().c_str());
        return -1;
    }
    return unitstate->map_get(key, field, value);
}

int32_t xstatestore_impl_t::string_get(common::xaccount_address_t const & account_address, const std::string& key, std::string& value) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::string_get fail-find account. account=%s", account_address.value().c_str());
        return -1;
    }
    return unitstate->string_get(key, value);
}

int32_t xstatestore_impl_t::map_copy_get(common::xaccount_address_t const & account_address, const std::string &key, std::map<std::string, std::string> &map) const {
    auto unitstate = get_unit_latest_connectted_state(account_address);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::map_copy_get fail-find account. account=%s", account_address.value().c_str());
        return -1;
    }
    return unitstate->map_copy_get(key, map);
}

int32_t xstatestore_impl_t::get_map_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string &name, std::map<std::string, std::string> &value) const {
    auto unitstate = get_unit_committed_state(account_address, height);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::get_map_property fail-find account. account=%s", account_address.value().c_str());
        return -1;
    }

    return unitstate->map_copy_get(name, value);
}

int32_t xstatestore_impl_t::get_string_property(common::xaccount_address_t const & account_address, uint64_t height, const std::string &name, std::string &value) const {
    auto unitstate = get_unit_committed_state(account_address, height);
    if (nullptr == unitstate) {
        xwarn("xstatestore_impl_t::get_string_property fail-find account. account=%s", account_address.value().c_str());
        return -1;
    }

    return unitstate->string_get(name, value);
}


base::xauto_ptr<base::xvblock_t> xstatestore_impl_t::get_latest_connectted_state_changed_block(base::xvblockstore_t* blockstore, const base::xvaccount_t & account) {
    // TODO(jimmy) check if the binlog hash exists
    base::xauto_ptr<base::xvblock_t> vblock = blockstore->get_latest_connected_block(account);
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }
    if (vblock->get_block_class() == base::enum_xvblock_class_full &&
        base::xvblock_fork_t::is_block_match_version(vblock->get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
        return vblock;
    }
    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(account, current_height - 1, base::enum_xvblock_flag_committed, false);
        if (prev_vblock == nullptr || prev_vblock->get_block_class() == base::enum_xvblock_class_light) {
            return prev_vblock;
        }
        if (prev_vblock->get_block_class() == base::enum_xvblock_class_full &&
            base::xvblock_fork_t::is_block_match_version(prev_vblock->get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
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
    if (vblock->get_block_class() == base::enum_xvblock_class_light) {
        return vblock;
    }
    if (vblock->get_block_class() == base::enum_xvblock_class_full &&
        base::xvblock_fork_t::is_block_match_version(vblock->get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
        return vblock;
    }

    uint64_t current_height = vblock->get_height();
    while (current_height > 0) {
        XMETRICS_GAUGE(metrics::blockstore_access_from_application, 1);
        base::xauto_ptr<base::xvblock_t> prev_vblock = blockstore->load_block_object(account, current_height - 1, base::enum_xvblock_flag_committed, false);
        if (prev_vblock == nullptr || prev_vblock->get_block_class() == base::enum_xvblock_class_light) {
            return prev_vblock;
        }
        if (prev_vblock->get_block_class() == base::enum_xvblock_class_full &&
            base::xvblock_fork_t::is_block_match_version(prev_vblock->get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
            return prev_vblock;
        }

        current_height = prev_vblock->get_height();
    }
    return nullptr;
}

data::xtablestate_ptr_t xstatestore_impl_t::get_table_state_by_block_internal(common::xaccount_address_t const& table_address, base::xvblock_t * target_block) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_address);
    return tablestore->get_table_state_from_block(target_block);
}

data::xtablestate_ptr_t xstatestore_impl_t::get_table_state_by_block(base::xvblock_t * target_block) const {
    common::xaccount_address_t table_address(target_block->get_account());
    return get_table_state_by_block_internal(table_address, target_block);
}


data::xtablestate_ptr_t xstatestore_impl_t::get_table_connectted_state(common::xaccount_address_t const & table_address) const {
    xstatestore_table_ptr_t tablestore = get_table_statestore_from_table_addr(table_address);
    return tablestore->get_latest_connectted_table_state();
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
                xerror("xstatestore_impl_t::get_receiptid_state_and_prove, continuous nil table block number is more than 2,table:%s,height:%llu", table_address.value().c_str(), height);
                return false;
            }

            auto commit_block =
                get_blockstore()->load_block_object(table_address.vaccount(), height - 1, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_txpool_id_state);
            if (commit_block == nullptr) {
                xwarn("xstatestore_impl_t::get_receiptid_state_and_prove load block fail,table:%s,height:%llu", table_address.value().c_str(), height - 1);
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
        xinfo("xstatestore_impl_t::get_receiptid_state_and_prove latest commit height is 0, no need send receipt id state.table:%s", table_address.value().c_str());
        return false;
    }

    data::xtablestate_ptr_t tablestate = get_table_state_by_block_internal(table_address, non_nil_commit_block.get());
    if (tablestate == nullptr) {
        xwarn("xstatestore_impl_t::get_receiptid_state_and_prove table:%s,height:%llu,get bstate fail", table_address.value().c_str(), non_nil_commit_block->get_height());
        return false;
    }

    if (tablestate->get_receiptid_state()->get_all_receiptid_pairs()->get_all_pairs().empty()) {
        xinfo("xstatestore_impl_t::get_receiptid_state_and_prove table have no receipt id pairs.table:%s, commit height:%llu", table_address.value().c_str(), non_nil_commit_block->get_height());
        return false;
    }

    auto cert_block = get_blockstore()->load_block_object(table_address.vaccount(), non_nil_commit_block->get_height() + 2, 0, false);
    if (cert_block == nullptr) {
        xinfo("xstatestore_impl_t::get_receiptid_state_and_prove cert block load fail.table:%s, cert height:%llu", table_address.value().c_str(), non_nil_commit_block->get_height() + 2);
        return false;
    }

    auto property_prove = base::xpropertyprove_build_t::create_property_prove(non_nil_commit_block.get(), cert_block.get(), tablestate->get_bstate().get(), data::XPROPERTY_TABLE_RECEIPTID);
    if (property_prove == nullptr) {
        xwarn("xstatestore_impl_t::get_receiptid_state_and_prove create receipt state fail 2.table:%s, commit height:%llu", table_address.value().c_str(), non_nil_commit_block->get_height());
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

void xstatestore_impl_t::update_node_type(common::xnode_type_t combined_node_type) {
    m_combined_node_type = combined_node_type;
}

common::xnode_type_t xstatestore_impl_t::get_node_type() const {
    return m_combined_node_type;
}

bool xstatestore_impl_t::is_archive_node() const {
    return common::has<common::xnode_type_t::storage_archive>(get_node_type());
}

void xstatestore_impl_t::push_try_execute_table(std::string table_addr) const {
    if (!is_archive_node()) {
        std::lock_guard<std::mutex> lck(m_mutex);
        m_try_execute_tables.insert(table_addr);
    }
}

void xstatestore_impl_t::pop_try_execute_tables(std::set<std::string> & try_execute_tables) const {
    try_execute_tables.clear();
    if (is_archive_node()) {
        // TODO(jimmy) need optimize
        std::vector<std::string> tables_vec = data::xblocktool_t::make_all_table_addresses();
        std::set<std::string> tables(tables_vec.begin(), tables_vec.end());
        try_execute_tables.swap(tables);
    } else {
        std::lock_guard<std::mutex> lck(m_mutex);
        try_execute_tables.swap(m_try_execute_tables);
    }
}

void xstatestore_impl_t::try_update_tables_execute_height() const {
    std::set<std::string> try_execute_tables;
    pop_try_execute_tables(try_execute_tables);
    for (auto & table : try_execute_tables) {
        try_update_execute_height(base::xvaccount_t(table), 32);
    }
    
}

bool xstatestore_timer_t::on_timer_fire(const int32_t thread_id,
                                        const int64_t timer_id,
                                        const int64_t current_time_ms,
                                        const int32_t start_timeout_ms,
                                        int32_t & in_out_cur_interval_ms) {
    m_statestore->try_update_tables_execute_height();
    return true;
}

NS_END2
