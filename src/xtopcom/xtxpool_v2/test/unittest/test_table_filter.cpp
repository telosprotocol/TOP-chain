#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_face.h"
#include "xtxpool_v2/xtx_account_filter.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xblockstore/xblockstore_face.h"

using namespace top::xtxpool_v2;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;

class test_table_filter : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {}
public:
};

TEST_F(test_table_filter, one_duplicate_tx) {
}
