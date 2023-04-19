#pragma once
#include <string>
#include "gtest/gtest.h"
#include "xdb/xdb_factory.h"
#include "xdbstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvledger.h"
#include "xmbus/xmessage_bus.h"
#include "tests/mock/xtestdb.hpp"
#include "xbasic/xtimer_driver.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xgenesis/xgenesis_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
#include "xvm/manager/xcontract_manager.h"
#include "xstatestore/xstatestore_face.h"

namespace top
{
    namespace mock
    {
        class xvchain_creator {
        public:
            xvchain_creator(std::string dbpath = std::string()) {
                clear();
                std::error_code ec;
                m_bus = top::make_object_ptr<mbus::xmessage_bus_t>(false, 1000);
                base::xvchain_t::instance().set_xevmbus(m_bus.get());
                if (dbpath.empty()) {
                    m_db = db::xdb_factory_t::create_memdb();
                } else {
                    m_db = db::xdb_factory_t::create_kvdb(dbpath);
                }
                
                m_store = store::xstore_factory::create_store_with_static_kvdb(m_db);
                base::xvchain_t::instance().set_xdbstore(m_store.get());

                base::xvblockstore_t * blockstore = store::create_vblockstore(m_store.get());
                base::xvchain_t::instance().set_xblockstore(blockstore);
                base::xvchain_t::instance().set_xtxstore(txstore::create_txstore(make_observer<mbus::xmessage_bus_face_t>(m_bus.get()), nullptr));
                m_genesis_manager = top::make_unique<genesis::xgenesis_manager_t>(top::make_observer(blockstore));
                m_genesis_manager->init_genesis_block(ec);

                statestore::xstatestore_hub_t::reset_instance();
            }

            xvchain_creator(bool genesis, std::string dbpath = std::string()) {
                clear();
                m_bus = top::make_object_ptr<mbus::xmessage_bus_t>(false, 1000);
                base::xvchain_t::instance().set_xevmbus(m_bus.get());
                if (dbpath.empty()) {
                    m_db = db::xdb_factory_t::create_memdb();
                } else {
                    m_db = db::xdb_factory_t::create_kvdb(dbpath);
                }

                m_store = store::xstore_factory::create_store_with_static_kvdb(m_db);
                base::xvchain_t::instance().set_xdbstore(m_store.get());

                base::xvblockstore_t * blockstore = store::create_vblockstore(m_store.get());
                base::xvchain_t::instance().set_xblockstore(blockstore);

                // std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
                // std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
                base::xvchain_t::instance().set_xtxstore(txstore::create_txstore(make_observer<mbus::xmessage_bus_face_t>(m_bus.get()), nullptr));
                if (genesis) {
                    m_genesis_manager = top::make_unique<genesis::xgenesis_manager_t>(top::make_observer(blockstore));
                    contract::xcontract_deploy_t::instance().deploy_sys_contracts();
                    contract::xcontract_manager_t::instance().instantiate_sys_contracts();
                    contract::xcontract_manager_t::instance().register_address();
                    std::error_code ec;
                    m_genesis_manager->init_genesis_block(ec);
                    top::error::throw_error(ec);
                }
                statestore::xstatestore_hub_t::reset_instance();
            }
            ~xvchain_creator() {

            }
            void clear() {
                // clean
                base::xvchain_t::instance().clean_all(true);
                contract::xcontract_deploy_t::instance().clear();
                contract::xcontract_manager_t::instance().clear();
                if (m_db != nullptr) {
                    m_db->close();
                    m_db = nullptr;
                }
            }

            void create_blockstore_with_xstore() {
                // base::xvchain_t::instance().clean_all(true);
                // mock::xveventbus_impl* mbus_store = new mock::xveventbus_impl();
                // base::xvchain_t::instance().set_xevmbus(mbus_store);

                // m_store = store::xstore_factory::create_store_with_memdb();
                // base::xvchain_t::instance().set_xdbstore(m_store.get());

                // base::xvblockstore_t * blockstore = store::create_vblockstore(m_store.get());
                // base::xvchain_t::instance().set_xblockstore(blockstore);
            }
            base::xvblockstore_t* get_blockstore() const { return base::xvchain_t::instance().get_xblockstore(); }
            // base::xvblkstatestore_t*    get_xblkstatestore() const { return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store(); }
            void clean_all() {
                base::xvchain_t::instance().clean_all(false);
            }
            store::xstore_face_t* get_xstore() const {return m_store.get();}
            base::xvtxstore_t * get_txstore() const { return base::xvchain_t::instance().get_xtxstore(); }
            const xobject_ptr_t<top::mbus::xmessage_bus_face_t> & get_mbus() const {return m_bus;}
            const std::shared_ptr<db::xdb_face_t> &     get_xdb() const {return m_db;}

        private:
            std::shared_ptr<db::xdb_face_t>      m_db{nullptr};
            xobject_ptr_t<store::xstore_face_t>  m_store{nullptr};
            xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus;
            std::unique_ptr<genesis::xgenesis_manager_t> m_genesis_manager;
        };
    }
}

