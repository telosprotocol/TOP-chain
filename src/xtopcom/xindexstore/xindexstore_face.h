// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xblock.h"
#include "xdata/xtablestate.h"
#include "xvledger/xaccountindex.h"
#include "xstore/xstore_face.h"

NS_BEG2(top, store)

class xindexstore_resources_t {
 public:
    virtual store::xstore_face_t*       get_store() const = 0;
    virtual base::xvblockstore_t*       get_blockstore() const = 0;
};
using xindexstore_resources_ptr_t = std::shared_ptr<xindexstore_resources_t>;

class xindexstore_resources_impl_t : public xindexstore_resources_t {
 public:
    xindexstore_resources_impl_t(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore)
    : m_store(store), m_blockstore(blockstore) {}
    virtual store::xstore_face_t*       get_store() const {return m_store.get();}
    virtual base::xvblockstore_t*       get_blockstore() const {return m_blockstore.get();}
 private:
    observer_ptr<store::xstore_face_t>          m_store{nullptr};
    observer_ptr<base::xvblockstore_t>          m_blockstore{nullptr};
};

class xaccount_basic_info_t {
 public:
    xaccount_basic_info_t() = default;
   //  xaccount_basic_info_t(const xblock_ptr_t & block, const base::xaccount_index_t & index)
   //  : m_latest_block(block), m_account_index(index) {}

 public:
   //  void    set_latest_block(const xblock_ptr_t & block) {m_latest_block = block;}
    void    set_account_index(const base::xaccount_index_t & index) {m_account_index = index;}
    void    set_latest_state(const xaccount_ptr_t & state) {m_latest_state = state;}
    void    set_sync_height_start(uint64_t height) {m_sync_height_start = height;}
    void    set_sync_num(uint32_t num) {m_sync_num = num;}

   //  const xblock_ptr_t &            get_latest_block() const {return m_latest_block;}
    const xaccount_ptr_t &          get_latest_state() const {return m_latest_state;}
    const base::xaccount_index_t &  get_accout_index() const {return m_account_index;}
    const uint64_t                  get_sync_height_start() const {return m_sync_height_start;}
    const uint32_t                  get_sync_num() const {return m_sync_num;}

 private:
   //  xblock_ptr_t            m_latest_block{nullptr};
    xaccount_ptr_t          m_latest_state{nullptr};
    base::xaccount_index_t  m_account_index;
    uint64_t                m_sync_height_start{0};
    uint32_t                m_sync_num{0};
};

class xindexstore_face_t : public base::xvaccount_t {
 public:
    xindexstore_face_t(const std::string & account)
    : base::xvaccount_t(account) {}
    virtual ~xindexstore_face_t() {}

    virtual xtablestate_ptr_t     clone_tablestate(const xblock_ptr_t & block) = 0;

    virtual bool  get_account_index(const std::string & account, base::xaccount_index_t & account_index) = 0;
    virtual bool  get_account_index(const xblock_ptr_t & committed_block, const std::string & account, base::xaccount_index_t & account_index) = 0;
    virtual bool  get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) = 0;
};

using xindexstore_face_ptr_t = xobject_ptr_t<xindexstore_face_t>;

// xindexstorehub_t manage multiple table indexstore
class xindexstorehub_t : public base::xobject_t {
 public:
    virtual xindexstore_face_ptr_t    get_index_store(const std::string & table_account) = 0;
};

class xindexstore_factory_t {
 public:
    static xobject_ptr_t<xindexstorehub_t> create_indexstorehub(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore);
    static  xindexstore_face_ptr_t    create_index_store(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore, const std::string & table_account);
};


NS_END2
