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
    m_resources = std::make_shared<xindexstore_resources_impl_t>(store, blockstore);
}

xindexstore_face_ptr_t    xindexstorehub_impl_t::get_index_store(const std::string & table_account) {
    xindexstore_face_ptr_t indexstore = make_object_ptr<xindexstore_table_t>(table_account, m_resources);
    return indexstore;
}

NS_END2
