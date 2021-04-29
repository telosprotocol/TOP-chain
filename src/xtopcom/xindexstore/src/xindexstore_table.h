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

    virtual bool  get_account_index(const std::string & account, base::xaccount_index_t & account_index);
    virtual bool  get_account_index(const xblock_ptr_t & committed_block, const std::string & account, base::xaccount_index_t & account_index);
    virtual bool  get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_info);

 private:
    store::xstore_face_t*       get_store() const {return m_resources->get_store();}
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    xtablestate_ptr_t           get_target_tablestate(const xblock_ptr_t & block);
    xtablestate_ptr_t           get_target_block_state(const xtablestate_ptr_t & old_state, const xblock_ptr_t & block);
    xtablestate_ptr_t           load_base_tablestate_from_db(const xtablestate_ptr_t & old_tablestate);
    xtablestate_ptr_t           rebuild_tablestate(const xtablestate_ptr_t & old_state, const std::map<uint64_t, xblock_ptr_t> & latest_blocks);

    void                        set_cache_state(const xblock_ptr_t & block, const xtablestate_ptr_t & state);
    xtablestate_ptr_t           get_cache_state(const xblock_ptr_t & block) const;
    void                        clear_old_cache_state();

 private:
    xtablestate_ptr_t                           m_tablestate;  // cache db committed state
    std::map<std::string, xtablestate_ptr_t>    m_cache_tablestate;  // cache more state for query performance
    uint64_t                        max_highest_height{0};
    xindexstore_resources_ptr_t     m_resources;
    mutable std::mutex              m_lock;
};
using xindexstore_table_ptr_t = xobject_ptr_t<xindexstore_table_t>;

NS_END2
