#include "gtest/gtest.h"

#define private public
#define protected public

#include "test_common.hpp"
#include "xblockmaker/xtable_maker.h"
#include "xchain_fork/xutility.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_address.hpp"
#include "xdata/xnative_contract_address.h"
#include "xblockmaker/xproposal_maker.h"
#include "xblockmaker/xblock_builder.h"
#include "xstatectx/xstatectx_base.h"
#include "xmetrics/xmetrics.h"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::blockmaker;

class test_performance : public testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
};

// TEST_F(test_performance, create_unit_BENCH) {
// //total time: 101 ms hash_count: 20000
//     mock::xvchain_creator creator(true);
//     auto t1 = base::xtime_utl::time_now_ms();

//     int64_t begin_hash_count = XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);

//     uint64_t count = 10000;
//     std::string account = "T8000089e31fa7d08a0b145ba7145aab1e1780486e76a1";
//     data::xblock_consensus_para_t cs_para;
//     cs_para.set_clock(100000000);
//     cs_para.set_viewid(100000);
//     cs_para.set_parent_height(1000);

//     base::xauto_ptr<base::xvblock_t> _block = xblocktool_t::create_genesis_lightunit(account, 1000000);
//     data::xaccountstate_ptr_t account_state = xdatamock_unit::create_accountstate(_block.get());
//     data::xaccountstate_ptr_t proposal_accountstate = statectx::xstatectx_base_t::create_proposal_account_state(account_state->get_accountindex(), account_state->get_unitstate());
//     for (uint32_t i = 0; i < 10000; i++) {
//         proposal_accountstate->get_unitstate()->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(1));  
//     }

//     for (uint64_t i = 0; i < count; i++) {
//         xunit_build_result_t unit_result;
//         xunitbuilder_t::make_unitblock_and_unitstate(account_state, cs_para, unit_result);
//     }

//     int64_t end_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);    
//     auto t2 = base::xtime_utl::time_now_ms();
//     std::cout << "total time: " << t2 - t1 << " ms" 
//               << " hash_count: " << end_hash_count - begin_hash_count
//     << std::endl;    
//     // xassert(end_hash_count - begin_hash_count == count*2);
// }


// TEST_F(test_performance, create_table_BENCH) {
// // total time: 62945 ms hash_count: 721679    
//     mock::xvchain_creator creator(true);
//     base::xvblockstore_t * blockstore = creator.get_blockstore();

//     auto t1 = base::xtime_utl::time_now_ms();

//     int64_t begin_hash_count = XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
//     uint64_t count = 1000;
//     mock::xdatamock_table mocktable(1, 100);
//     mocktable.genrate_table_chain(count, blockstore);

//     int64_t end_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
//     auto t2 = base::xtime_utl::time_now_ms();
//     std::cout << "total time: " << t2 - t1 << " ms" 
//               << " hash_count: " << end_hash_count - begin_hash_count
//     << std::endl;    
//     // xassert(end_hash_count - begin_hash_count < count*400*2 + 10000*5);
// }
