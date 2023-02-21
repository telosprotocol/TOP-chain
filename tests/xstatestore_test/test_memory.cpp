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
#include "xmetrics/xmetrics.h"

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

void print_metrics() {
    {
    #ifdef ENABLE_METRICS
        int64_t free_memory  = 0;
        int percent = base::xsys_utl::get_memory_load(free_memory);
        std::cout << "free_memory " << free_memory << " percent=" << percent << std::endl;
        std::cout << "dataobject_xvaccount " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xvaccount) << std::endl;
        std::cout << "dataobject_xvblock " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xvblock) << std::endl;
        std::cout << "dataobject_account_address " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_account_address) << std::endl;
        std::cout << "dataobject_mpt_state_object " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_mpt_state_object) << std::endl;
        std::cout << "dataobject_mpt_trie_node_cnt " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_mpt_trie_node_cnt) << std::endl;
        std::cout << "dataobject_xvaccountobj " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xvaccountobj) << std::endl;
        std::cout << "dataobject_xvbstate " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xvbstate) << std::endl;
        std::cout << "dataobject_xaccount_index " << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xaccount_index) << std::endl;
        std::cout << "db_write " << XMETRICS_GAUGE_GET_VALUE(metrics::db_write) << std::endl;
        std::cout << "db_read " << XMETRICS_GAUGE_GET_VALUE(metrics::db_read) << std::endl;
        base::xvchain_t::instance().get_xdbstore()->GetDBMemStatus();
        std::cout << "db_rocksdb_block_cache " << XMETRICS_GAUGE_GET_VALUE(metrics::db_rocksdb_block_cache) << std::endl;
        std::cout << "db_rocksdb_table_readers " << XMETRICS_GAUGE_GET_VALUE(metrics::db_rocksdb_table_readers) << std::endl;
        std::cout << "db_rocksdb_total " << XMETRICS_GAUGE_GET_VALUE(metrics::db_rocksdb_total) << std::endl;
    #endif
    }
}

TEST_F(test_memory, first_mpt_block_execute_one_table_BENCH) {
    class test_xstatestore_executor_t : public statestore::xstatestore_executor_t {
    public:
        test_xstatestore_executor_t(common::xtable_address_t const& table_addr)
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

    print_metrics();

    statestore::xstatestore_hub_t::reset_instance();

    common::xtable_address_t table_addr = common::xtable_address_t::build_from(xstring_view_t{"Ta0000@40"});
    test_xstatestore_executor_t table_executor{table_addr};

    xtablestate_ext_ptr_t tablestate = table_executor.get_latest_executed_tablestate_ext();
    if (nullptr == db) {
        xerror("--tablestate null");
        return;
    }
    xobject_ptr_t<base::xvblock_t> cert_block = blockstore->get_latest_cert_block(table_addr.vaccount());
    std::cout << "7 Table="  << table_addr.to_string() <<
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
    xtablestate_ext_ptr_t prev_state = table_executor.execute_and_get_tablestate_ext(fist_mpt_prev_block.get(), true, ec);
    if (nullptr == prev_state) {
        std::cout << "fail fist_mpt_prev_block get state " << fist_mpt_prev_block->dump() << std::endl;
        return;
    }

    std::cout << std::endl;

    int64_t free_memory  = 0;
    for (uint32_t i = 0; i < 500; i++) {
        if (i % 50 == 0) {
            int percent = base::xsys_utl::get_memory_load(free_memory);
            std::cout << "execute i=" << i << " free_memory " << free_memory << " percent " << percent << std::endl;
        }

        xtablestate_ext_ptr_t cur_state = table_executor.force_do_make_state_from_prev_state_and_table(fist_mpt_block.get(), prev_state, ec);
        if (nullptr == cur_state) {
            std::cout << "fail fist_mpt_block get state " << fist_mpt_block->dump() << std::endl;
            return;
        }
        // base::xtime_utl::sleep_ms(1);
    }

    print_metrics();
    base::xtime_utl::sleep_ms(60000);
}

TEST_F(test_memory, first_mpt_block_execute_BENCH) {
    class test_xstatestore_executor_t : public statestore::xstatestore_executor_t {
    public:
        test_xstatestore_executor_t(common::xtable_address_t const& table_addr)
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

    print_metrics();

    statestore::xstatestore_hub_t::reset_instance();

    auto table_addrs = data::xblocktool_t::make_all_table_addresses();
    for (auto & addr : table_addrs) {
        common::xtable_address_t table_addr = common::xtable_address_t::build_from(addr);
        test_xstatestore_executor_t table_executor{table_addr};

        xtablestate_ext_ptr_t tablestate = table_executor.get_latest_executed_tablestate_ext();
        if (nullptr == db) {
            std::cout << "table=" << table_addr.to_string() << " fail no latest tablestate" << std::endl;
            continue;
        }
        xobject_ptr_t<base::xvblock_t> cert_block = blockstore->get_latest_cert_block(table_addr.vaccount());
        xobject_ptr_t<base::xvblock_t> current_block = cert_block;
        auto current_state_root = data::xblockextract_t::get_state_root_from_block(current_block.get());
        xobject_ptr_t<base::xvblock_t> fist_mpt_block = nullptr;
        xobject_ptr_t<base::xvblock_t> fist_mpt_prev_block = nullptr;
        if (current_state_root.empty()) {
            std::cout << "table=" << table_addr.to_string() << " fail cert block state root empty" << " cert_height=" << cert_block->get_height() << std::endl;
            continue;
        }

        while (current_block->get_height() > 1) {
            xobject_ptr_t<base::xvblock_t> prev_block = blockstore->load_block_object(table_addr.vaccount(), current_block->get_height()-1, current_block->get_last_block_hash(), false);
            if (nullptr == prev_block) {
                std::cout << "table=" << table_addr.to_string() << " fail prev_block is nullptr" << std::endl;
                return;
            }
            auto prev_state_root = data::xblockextract_t::get_state_root_from_block(prev_block.get());
            if (prev_state_root.empty()) {
                fist_mpt_block = current_block;
                fist_mpt_prev_block = prev_block;
                break;
            }
            current_block = prev_block;
        }

        if (fist_mpt_block == nullptr) {
            std::cout << "table=" << table_addr.to_string() << " fail can't find first mpt block" << std::endl;
            return;
        }

        std::error_code ec;
        xtablestate_ext_ptr_t prev_state = table_executor.execute_and_get_tablestate_ext(fist_mpt_prev_block.get(), true, ec);
        if (nullptr == prev_state) {
            std::cout << "table=" << table_addr.to_string() << " fail fist_mpt_prev_block get state " << fist_mpt_prev_block->dump() << std::endl;
            return;
        }


        {
            int64_t free_memory  = 0;
            int percent = base::xsys_utl::get_memory_load(free_memory);
            base::xvchain_t::instance().get_xdbstore()->GetDBMemStatus();
            std::cout << "table=" << table_addr.to_string()
            << " state=" << tablestate->get_table_state()->height()
            << " cert=" << cert_block->get_height()
            << " fisrt=" << fist_mpt_block->get_height()
            << " free=" << free_memory
            << " percent=" << percent
    #ifdef ENABLE_METRICS
            << " xvaccountobj=" << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xvaccountobj)
            << " xvblock=" << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_xvblock)
            << " lightunit=" << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_block_lightunit)
            << " db_cache=" << XMETRICS_GAUGE_GET_VALUE(metrics::db_rocksdb_total)
            << " trie_node=" << XMETRICS_GAUGE_GET_VALUE(metrics::dataobject_mpt_trie_node_cnt)
    #endif
            << std::endl;
        }


        for (uint32_t i = 0; i < 1; i++) {
            xtablestate_ext_ptr_t cur_state = table_executor.force_do_make_state_from_prev_state_and_table(fist_mpt_block.get(), prev_state, ec);
            if (nullptr == cur_state) {
                std::cout << "table=" << table_addr.to_string() << " fail fist_mpt_block get state " << fist_mpt_block->dump() << std::endl;
                break;
            }
        }
    }


    print_metrics();

    base::xtime_utl::sleep_ms(60000*10);

    print_metrics();

    base::xtime_utl::sleep_ms(60000);
}


// TEST_F(test_memory, new_delete_test) {

// class test_data {
// public:

//     std::string data1;
//     std::string data2;
//     std::string data3;
//     std::string data4;
//     std::string data5;
// };






// }
