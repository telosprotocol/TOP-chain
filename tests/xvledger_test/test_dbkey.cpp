#include "gtest/gtest.h"
#include "xvledger/xvdbkey.h"
#include "xvledger/xvtransaction.h"

using namespace top;
using namespace top::base;

class test_dbkey : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_dbkey, txkey_1) {
    xassert(base::enum_txindex_type_send == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_self));
    xassert(base::enum_txindex_type_send == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_send));
    xassert(base::enum_txindex_type_receive == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_recv));
    xassert(base::enum_txindex_type_confirm == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_confirm));
}



