#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"

#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvblockhub.h"
#include "xstatestore/xstatestore_face.h"
#include "xstatestore/xstatestore_exec.h"
#include "test_common.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::statestore;

class test_memory : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_memory, first_mpt_block_execute) {
    class test_xstatestore_executor_t : public statestore::xstatestore_executor_t {
    public:
        test_xstatestore_executor_t(common::xaccount_address_t const& table_addr)
        : statestore::xstatestore_executor_t(table_addr, nullptr) {
            init();
        }

        xtablestate_ext_ptr_t  force_do_make_state_from_prev_state_and_table(base::xvblock_t* current_block, xtablestate_ext_ptr_t const& prev_state, std::error_code & ec) const {
            return make_state_from_prev_state_and_table(current_block, prev_state, ec);
        }
    };

    std::string dbpath = "/chain/db_v3/";
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_kvdb(dbpath);
    if (nullptr == db) {
        xerror("--test fail for db open");
        return;
    }
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_static_kvdb(db);
    base::xvchain_t::instance().set_xdbstore(store.get());

    base::xvblockstore_t * blockstore = store::create_vblockstore(store.get());
    base::xvchain_t::instance().set_xblockstore(blockstore);

    statestore::xstatestore_hub_t::reset_instance();

    common::xaccount_address_t table_addr{"Ta0000@40"};
    test_xstatestore_executor_t table_executor{table_addr};

    xtablestate_ext_ptr_t tablestate = table_executor.get_latest_executed_tablestate_ext();
    if (nullptr == db) {
        xerror("--tablestate null");
        return;
    }        
    xobject_ptr_t<base::xvblock_t> cert_block = blockstore->get_latest_cert_block(table_addr.vaccount());
    std::cout << "4 Table="  << table_addr.value() << 
    " latest_state_height=" << tablestate->get_table_state()->height() << 
    " cert_block_height=" << cert_block->get_height() 
    << std::endl;

    xobject_ptr_t<base::xvblock_t> current_block = cert_block;
    auto current_state_root = data::xblockextract_t::get_state_root_from_block(current_block.get());
    xobject_ptr_t<base::xvblock_t> fist_mpt_block = nullptr;
    xobject_ptr_t<base::xvblock_t> fist_mpt_prev_block = nullptr;
    if (current_state_root.empty()) {
        std::cout << "fail cert block state root empty" << std::endl;
        return;
    } 

    while (current_block->get_height() > 1) {
        xobject_ptr_t<base::xvblock_t> prev_block = blockstore->load_block_object(table_addr.vaccount(), current_block->get_height()-1, current_block->get_last_block_hash(), false);
        if (nullptr == prev_block) {
            std::cout << "fail prev_block is nullptr" << std::endl;
            return;
        }
        auto prev_state_root = data::xblockextract_t::get_state_root_from_block(prev_block.get());
        if (prev_state_root.empty()) {
            fist_mpt_block = current_block;
            fist_mpt_prev_block = prev_block;
            std::cout << "find mpt state block " << fist_mpt_block->dump() << std::endl;
            break;
        }
        current_block = prev_block;
    }

    if (fist_mpt_block == nullptr) {
        std::cout << "fail can't find first mpt block" << std::endl;
        return;        
    }

    std::error_code ec;
    xtablestate_ext_ptr_t prev_state = nullptr;
    table_executor.execute_and_get_tablestate_ext(fist_mpt_prev_block.get(), prev_state, ec);
    if (nullptr == prev_state) {
        std::cout << "fail fist_mpt_prev_block get state " << fist_mpt_prev_block->dump() << std::endl;
        return;        
    }

    std::cout << std::endl;
    for (uint32_t i = 0; i < 500; i++) {
        std::cout << "execute i=" << i;
        xtablestate_ext_ptr_t cur_state = table_executor.force_do_make_state_from_prev_state_and_table(fist_mpt_block.get(), prev_state, ec);
        if (nullptr == cur_state) {
            std::cout << "fail fist_mpt_block get state " << fist_mpt_block->dump() << std::endl;
            return;        
        }        
        // base::xtime_utl::sleep_ms(1);
    }
    std::cout << std::endl;
    base::xtime_utl::sleep_ms(60000);
}