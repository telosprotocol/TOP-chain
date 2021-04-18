// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <mutex>
#include "xbasic/xmemory.hpp"
#include "xstore/xstore_face.h"
#include "xindexstore/xindexstore_face.h"

NS_BEG2(top, store)

class xindexstore_table_t : public xindexstore_face_t {
 public:
    xindexstore_table_t(const std::string & account, const xindexstore_resources_ptr_t & resources);
    ~xindexstore_table_t() {}

 public:
    virtual xtablestate_ptr_t       clone_tablestate(const xblock_ptr_t & block);
    virtual xtablestate_ptr_t       clone_tablestate();

    virtual bool  get_account_index(const std::string & account, base::xaccount_index_t & account_index);
    virtual bool  get_account_index(const xblock_ptr_t & committed_block, const std::string & account, base::xaccount_index_t & account_index);
    virtual bool  get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_info);

 private:
    store::xstore_face_t*       get_store() const {return m_resources->get_store();}
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    bool                        update_tablestate(const xblock_ptr_t & block);

 private:
    xtablestate_ptr_t               m_tablestate;
    xindexstore_resources_ptr_t     m_resources;
    mutable std::mutex              m_lock;
};
using xindexstore_table_ptr_t = xobject_ptr_t<xindexstore_table_t>;

NS_END2
