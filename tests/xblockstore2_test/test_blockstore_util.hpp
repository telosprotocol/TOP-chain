#pragma once
#include <string>
#include "gtest/gtest.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvledger.h"
#include "tests/mock/xtestdb.hpp"

using namespace top;

class test_blockstore_util {
 public:
    test_blockstore_util() {
        m_mbus_store = new mock::xveventbus_impl();
        base::xvchain_t::instance().set_xevmbus(m_mbus_store);

        const std::string  default_path = "/";
        m_store_face = store::xstore_factory::create_store_with_memdb();
        base::xvchain_t::instance().set_xdbstore(m_store_face.get());
        m_blockstore = store::get_vblockstore();
    }

    base::xvblockstore_t*   get_blockstore() {return m_blockstore;}
 public:
    static std::string                          create_random_user_address();

 private:
    xobject_ptr_t<store::xstore_face_t> m_store_face;
    mock::xveventbus_impl*              m_mbus_store;
    base::xvblockstore_t*               m_blockstore;
};

