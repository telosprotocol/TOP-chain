#pragma once
#include <string>
#include "gtest/gtest.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvledger.h"
#include "tests/mock/xtestdb.hpp"

namespace top
{
    namespace mock
    {
        class xvchain_creator {
        public:
            void create_blockstore_with_xstore() {
                mock::xveventbus_impl* mbus_store = new mock::xveventbus_impl();
                base::xvchain_t::instance().set_xevmbus(mbus_store);

                m_store = store::xstore_factory::create_store_with_memdb();
                base::xvchain_t::instance().set_xdbstore(m_store.get());

                base::xvblockstore_t * blockstore = store::create_vblockstore();
                base::xvchain_t::instance().set_xblockstore(blockstore);
                std::cout << "create blockstore:" << blockstore << std::endl;
                std::cout << "create store:" << m_store.get() << std::endl;
            }
            base::xvblockstore_t* get_blockstore() const {
                std::cout << "get blockstore:" << base::xvchain_t::instance().get_xblockstore() << std::endl;
                std::cout << "get store:" << m_store.get() << std::endl;
                return base::xvchain_t::instance().get_xblockstore();
            }
            void clean_all() {
                base::xvchain_t::instance().clean_all(false);
            }
            store::xstore_face_t* get_xstore() const {return m_store.get();}

        private:
            xobject_ptr_t<store::xstore_face_t>  m_store{nullptr};
        };
    }
}

