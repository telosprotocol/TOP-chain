// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <mutex>
#include "xvledger/xvaccount.h"
#include "xbasic/xmemory.hpp"
#include "xstore/xstore_face.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xindexstore/xindexstore_face.h"

NS_BEG2(top, store)

class xindexstorehub_impl_t : public xindexstorehub_t {
 public:
    xindexstorehub_impl_t(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore);
    ~xindexstorehub_impl_t();

    virtual xindexstore_face_ptr_t    get_index_store(const std::string & table_account);

 private:
    mutable std::mutex                  m_lock;
    std::map<base::xtable_shortid_t, xindexstore_face_ptr_t> m_tablestore_mgrs;
    xindexstore_resources_ptr_t         m_resources;
};


NS_END2
