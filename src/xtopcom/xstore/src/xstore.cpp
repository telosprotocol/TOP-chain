// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <inttypes.h>
#include <mutex>
#include <utility>
#include <stack>
#include <string>

#include "xbase/xcontext.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbase/xhash.h"
#include "xvledger/xvdbkey.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvblockstore.h"
#include "xbasic/xmodule_type.h"
#include "xbase/xobject_ptr.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/xblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xproperty.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xgenesis_data.h"
#include "xdb/xdb_factory.h"

#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_consensus.h"
#include "xmbus/xevent_store.h"
#include "xmetrics/xmetrics.h"
#include "xstore/xaccount_context.h"
#include "xstore/xstore.h"
#include "xstore/xstore_error.h"

#include "xdata/xgenesis_data.h"

using namespace top::base;
using namespace top::data;

namespace top {
namespace store {

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xstore, store::xstore_error_to_string, store::xstore_error_base + 1, store::xstore_error_max);

xstore::xstore(const std::shared_ptr<db::xdb_face_t> &db)
    : m_db(db) {}

bool xstore::open() const {
    if (m_db == nullptr) {
        return false;
    }
    return m_db->open();
}

xaccount_ptr_t xstore::query_account(const std::string &address) const {
    base::xvaccount_t _vaddr(address);
    if (_vaddr.get_account().empty()) {
        xerror("xstore::query_account fail-invalid address. account=%s,size=%zu", address.c_str(), address.size());
        return nullptr;
    }

    XMETRICS_GAUGE(metrics::blockstore_access_from_store, 1);
    auto _block = base::xvchain_t::instance().get_xblockstore()->get_latest_connected_block(_vaddr);
    if (_block == nullptr) {
        xerror("xstore::query_account fail-load latest connectted block. account=%s", address.c_str());
        return nullptr;
    }

    if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
        xwarn("xstore::query_account fail-invalid state for empty genesis block. account=%s", address.c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get(), metrics::statestore_access_from_vnodesrv_load_state);
    if (bstate != nullptr) {
        xaccount_ptr_t account = std::make_shared<xunit_bstate_t>(bstate.get());
        return account;
    }
    xerror("xstore::query_account fail-load state.block=%s", _block->dump().c_str());
    return nullptr;
}

uint64_t xstore::get_blockchain_height(const std::string &owner) {
    xaccount_ptr_t blockchain = query_account(owner);
    if (blockchain != nullptr) {
        return blockchain->get_block_height();
    }
    return 0;
}

int32_t xstore::string_get(const std::string &address, const std::string &key, std::string &value) const {
    // TODO(jimmy)
    xaccount_ptr_t account = query_account(address);
    if (nullptr == account) {
        xwarn("jimmy xstore::string_get fail-find account. account=%s", address.c_str());
        return xaccount_account_not_exist;
    }
    xaccount_context_t context(account, const_cast<xstore *>(this));
    return context.string_get(key, value);
}

int32_t xstore::list_get_all(const std::string &address, const std::string &key, std::vector<std::string> &values) {
    xaccount_ptr_t account = query_account(address);
    if (nullptr == account) {
        xwarn("jimmy xstore::list_get_all fail-find account. account=%s", address.c_str());
        return xaccount_account_not_exist;
    }
    xaccount_context_t context(account, const_cast<xstore *>(this));
    return context.list_get_all(key, values);
}

int32_t xstore::map_get(const std::string &address, const std::string &key, const std::string &field, std::string &value) {
    xaccount_ptr_t account = query_account(address);
    if (nullptr == account) {
        xwarn("jimmy xstore::map_get fail-find account. account=%s", address.c_str());
        return xaccount_account_not_exist;
    }
    xaccount_context_t context(account, const_cast<xstore *>(this));
    return context.map_get(key, field, value);
}

int32_t xstore::map_copy_get(const std::string &address, const std::string &key, std::map<std::string, std::string> &map) const {
    xaccount_ptr_t account = query_account(address);
    if (nullptr == account) {
        xwarn("jimmy xstore::map_copy_get fail-find account. account=%s", address.c_str());
        return -1;
    }
    xaccount_context_t context(account, const_cast<xstore *>(this));
    return context.map_copy_get(key, map);
}

int32_t xstore::get_map_property(const std::string &address, uint64_t height, const std::string &name, std::map<std::string, std::string> &value) {
    xaccount_ptr_t account = get_target_state(address, height);
    if (nullptr == account) {
        xwarn("jimmy xstore::get_map_property fail-find account. account=%s", address.c_str());
        return -1;
    }

    bool ret = account->map_get(name, value);
    if (!ret) {
        return -1;
    }
    return xsuccess;
}

int32_t xstore::get_string_property(const std::string &address, uint64_t height, const std::string &name, std::string &value) {
    xaccount_ptr_t account = get_target_state(address, height);
    if (nullptr == account) {
        xwarn("jimmy xstore::get_string_property fail-find account. account=%s", address.c_str());
        return -1;
    }
    bool ret = account->string_get(name, value);
    if (!ret) {
        return -1;
    }
    return xsuccess;
}

xaccount_ptr_t xstore::get_target_state(base::xvblock_t* block) const {
    if (block == nullptr) {
        xerror("xstore::get_target_state block is null");
        return nullptr;
    }

    const std::string & address = block->get_account();
    base::xvaccount_t _vaddr(address);
    base::xvblkstatestore_t* blkstatestore = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
    base::xauto_ptr<base::xvbstate_t> bstate = blkstatestore->get_block_state(block, metrics::statestore_access_from_vnodesrv_load_state);
    if (bstate == nullptr) {
        xwarn("xstore::get_target_state fail-load state.block=%s", block->dump().c_str());
        return nullptr;
    }
    xaccount_ptr_t current_state = std::make_shared<xunit_bstate_t>(bstate.get());
    xassert(current_state->get_block_height() == block->get_height() && current_state->get_block_viewid() == block->get_viewid());
    xdbg("xstore::get_target_state succ-make target state.account=%s,height=%ld,viewid=%ld",
        block->get_account().c_str(), block->get_height(), block->get_viewid());
    return current_state;
}

bool xstore::string_property_get(base::xvblock_t* block, const std::string& prop, std::string& value) const {
    xaccount_ptr_t state = get_target_state(block);
    if (state == nullptr) {
        xwarn("xstore::string_property_get get target state fail.block=%s,prop=%s", block->dump().c_str(), prop.c_str());
        return false;
    }
    return state->string_get(prop, value);
}

xaccount_ptr_t xstore::get_target_state(const std::string &address, uint64_t height) const {
    base::xvaccount_t _vaddr(address);
    XMETRICS_GAUGE(metrics::blockstore_access_from_store, 1);
    base::xauto_ptr<base::xvblock_t> _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddr, height, base::enum_xvblock_flag_committed, false);
    if (_block == nullptr) {
        xwarn("xstore::get_target_state load block fail.account=%s,height=%ld", address.c_str(), height);
        return nullptr;
    }

    xaccount_ptr_t current_state = get_target_state(_block.get());
    if (current_state == nullptr) {
        xwarn("xstore::get_target_state get target state fail.account=%s,height=%ld", address.c_str(), height);
        return nullptr;
    }
    return current_state;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_memdb() {
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_memdb();
    auto                            store = top::make_object_ptr<xstore>(db);
    return store;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_kvdb(const std::string &db_path) {
    xinfo("store init with one db, db path %s", db_path.c_str());
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_kvdb(db_path);
    auto                            store = top::make_object_ptr<xstore>(db);
    return store;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_static_kvdb(const std::string &db_path) {
    xinfo("static store init with one db, db path %s", db_path.c_str());
    static std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_kvdb(db_path);
    auto                                   store = top::make_object_ptr<xstore>(db);
    return store;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_static_kvdb(std::shared_ptr<db::xdb_face_t>& db) {
    auto                                   store = top::make_object_ptr<xstore>(db);
    return store;
}

bool xstore::set_vblock(const std::string & store_path, base::xvblock_t* vblock) {
    xassert(false);
    return false;
}

bool xstore::set_vblock_header(const std::string & store_path, base::xvblock_t* vblock) {
    xassert(false);
    return false;
}

base::xvblock_t *xstore::get_vblock(const std::string & store_path, const std::string &account, uint64_t height) const {
    xassert(false);
    return nullptr;
}

base::xvblock_t * xstore::get_vblock_header(const std::string & store_path, const std::string &account, uint64_t height) const {
    return nullptr;
}

bool xstore::get_vblock_input(const std::string & store_path, base::xvblock_t* block) const {
    xassert(false);
    return false;
}

bool xstore::get_vblock_output(const std::string &account, base::xvblock_t* block) const {
    xassert(false);
    return false;
}

bool xstore::delete_block_by_path(const std::string & store_path,const std::string & account, uint64_t height, bool has_input_output) {
    return false;
}

bool xstore::set_value(const std::string &key, const std::string &value) {
    return m_db->write(key, value);
}

bool xstore::delete_value(const std::string &key) {
    return m_db->erase(key);
}

const std::string xstore::get_value(const std::string &key) const {
    std::string value;

    bool success = m_db->read(key, value);
    if (!success) {
        return std::string();
    }
    return value;
}

bool  xstore::delete_values(std::vector<std::string> & to_deleted_keys)
{
    std::map<std::string, std::string> empty_put;
    return m_db->batch_change(empty_put, to_deleted_keys);
}

//prefix must start from first char of key
bool   xstore::read_range(const std::string& prefix, std::vector<std::string>& values)
{
    return m_db->read_range(prefix,values);
}
 
//note:begin_key and end_key must has same style(first char of key)
bool   xstore::delete_range(const std::string & begin_key,const std::string & end_key)
{
    return m_db->delete_range(begin_key,end_key);
}

//compact whole DB if both begin_key and end_key are empty
//note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
bool  xstore::compact_range(const std::string & begin_key,const std::string & end_key)
{
    return m_db->compact_range(begin_key,end_key);
}

//key must be readonly(never update after PUT),otherwise the behavior is undefined
bool   xstore::single_delete(const std::string & target_key)//key must be readonly(never update after PUT),otherwise the behavior is undefined
{
    return m_db->single_delete(target_key);
}

} // namespace store
} // namespace top
