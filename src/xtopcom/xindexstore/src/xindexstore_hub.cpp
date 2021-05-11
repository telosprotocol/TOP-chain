// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xdata/xgenesis_data.h"
#include "xindexstore/src/xindexstore_table.h"
#include "xindexstore/src/xindexstore_hub.h"

NS_BEG2(top, store)

using namespace top::data;

xobject_ptr_t<xindexstorehub_t> xindexstore_factory_t::create_indexstorehub(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore) {
    xobject_ptr_t<xindexstorehub_t> _hub = make_object_ptr<xindexstorehub_impl_t>(store, blockstore);
    return _hub;
}

xindexstore_face_ptr_t    xindexstore_factory_t::create_index_store(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore, const std::string & table_account) {
    xindexstore_resources_ptr_t resources = std::make_shared<xindexstore_resources_impl_t>(store, blockstore);
    xindexstore_face_ptr_t indexstore = make_object_ptr<xindexstore_table_t>(table_account, resources);
    return indexstore;
}

xindexstorehub_impl_t::xindexstorehub_impl_t(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore) {
    xdbg("xindexstorehub_impl_t::xindexstorehub_impl_t create,this=%p", this);
    m_resources = std::make_shared<xindexstore_resources_impl_t>(store, blockstore);
}
xindexstorehub_impl_t::~xindexstorehub_impl_t() {
    xdbg("xindexstorehub_impl_t::xindexstorehub_impl_t destroy,this=%p", this);
}

xindexstore_face_ptr_t    xindexstorehub_impl_t::get_index_store(const std::string & table_account) {
    std::lock_guard<std::mutex> lock(m_lock);

    // clear no use indexstore
    for (auto iter = m_tablestore_mgrs.begin(); iter != m_tablestore_mgrs.end();) {
        if (iter->second->get_refcount() == 1) {
            xdbg("xindexstorehub_impl_t::get_index_store delete.account=%s", iter->second->get_account().c_str());
            iter = m_tablestore_mgrs.erase(iter);
        } else {
            iter++;
        }
    }

    base::xvaccount_t _vaddr(table_account);
    base::xtable_shortid_t tableid = _vaddr.get_short_table_id();
    auto iter = m_tablestore_mgrs.find(tableid);
    if (iter != m_tablestore_mgrs.end()) {
        return iter->second;
    }
    xindexstore_face_ptr_t indexstore = make_object_ptr<xindexstore_table_t>(table_account, m_resources);
    m_tablestore_mgrs[tableid] = indexstore;
    xdbg("xindexstorehub_impl_t::get_index_store add.account=%s,refcount=%d,total_size=%zu", indexstore->get_account().c_str(), indexstore->get_refcount(), m_tablestore_mgrs.size());
    return indexstore;
}

NS_END2
