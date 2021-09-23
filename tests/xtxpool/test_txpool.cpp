#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xtxpool_v2/xtxpool.h"
#include "xtxpool_v2/xtxpool_para.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_xtxpool : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

void * sub_unsub_thread(void * arg) {
    xtxpool_t * xtxpool = (xtxpool_t *)arg;
    for (uint32_t i = 0; i < 1000; i++) {
        xtxpool->unsubscribe_tables(2, 0, 0, common::xnode_type_t::auditor);
        xtxpool->unsubscribe_tables(1, 0, 0, common::xnode_type_t::auditor);
        xtxpool->unsubscribe_tables(0, 0, 0, common::xnode_type_t::auditor);
    }

    return nullptr;
}

TEST_F(test_xtxpool, sub_unsub) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    store::xstore_face_t * xstore = creator.get_xstore();
    auto para = std::make_shared<xtxpool_resources>(make_observer(xstore), make_observer(blockstore), nullptr, nullptr);
    xtxpool_t xtxpool(para);

    pthread_t tid;
    pthread_create(&tid, NULL, sub_unsub_thread, &xtxpool);

    for (uint32_t i = 0; i < 1000; i++) {
        xtxpool.subscribe_tables(0, 0, 0, common::xnode_type_t::auditor);
        xtxpool.need_sync_lacking_receipts(0, 0);
        xtxpool.subscribe_tables(1, 0, 0, common::xnode_type_t::auditor);
        xtxpool.need_sync_lacking_receipts(1, 0);
        xtxpool.subscribe_tables(2, 0, 0, common::xnode_type_t::auditor);
        xtxpool.need_sync_lacking_receipts(2, 0);
    }

    pthread_join(tid, NULL);
}

TEST_F(test_xtxpool, check_black_hole_addr) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    store::xstore_face_t * xstore = creator.get_xstore();
    auto para = std::make_shared<xtxpool_resources>(make_observer(xstore), make_observer(blockstore), nullptr, nullptr);
    xtxpool_t xtxpool(para);

    auto tx_ent = xtxpool.query_tx(black_hole_addr, {});
    ASSERT_EQ(tx_ent, nullptr);
}
