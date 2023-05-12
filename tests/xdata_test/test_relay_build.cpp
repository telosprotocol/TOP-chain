#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xrelayblock_build.h"
#include "xdata/xblockextract.h"
#include "xdata/xnative_contract_address.h"
#include "xcommon/common_data.h"
#include "tests/xdata_test/test_eth.hpp"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_relay_build : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_relay_build, basic_1) {
    {
        xeth_transaction_t _tx = test_ethtx_tool::create_test_eth();
        xeth_receipt_t _receipt;
        xrelayblock_crosstx_info_t  txinfo(_tx, _receipt);
        xbytes_t _bs = txinfo.encodeBytes();

        xrelayblock_crosstx_info_t txinfo2;
        std::error_code ec;
        txinfo2.decodeBytes(_bs, ec);
        if (ec) {xassert(false);}
    }

    {        
        xrelayblock_crosstx_infos_t txinfos;
        {
            xeth_transaction_t _tx = test_ethtx_tool::create_test_eth();
            xeth_receipt_t _receipt;
            xrelayblock_crosstx_info_t  txinfo(_tx, _receipt);
            txinfos.tx_infos.push_back(txinfo);
        }
        {
            xeth_transaction_t _tx = test_ethtx_tool::create_test_eth();
            xeth_receipt_t _receipt;
            xrelayblock_crosstx_info_t  txinfo(_tx, _receipt);
            txinfos.tx_infos.push_back(txinfo);
        }
        xbytes_t _bs = txinfos.encodeBytes();
        xrelayblock_crosstx_infos_t txinfos2;
        std::error_code ec;
        txinfos2.decodeBytes(_bs, ec);    
        if (ec) {xassert(false);}
        xassert(txinfos2.tx_infos.size() == 2);
    }
}

TEST_F(test_relay_build, same_table_test) {
    base::xvaccount_t _relaytable(sys_contract_relay_table_block_addr);
    base::xvaccount_t _relaychain(sys_contract_relay_block_addr);

    ASSERT_EQ(get_vledger_bucket_index(_relaytable.get_xvid()), get_vledger_bucket_index(_relaychain.get_xvid()));        
    ASSERT_EQ(get_vledger_chain_id(_relaytable.get_xvid()), get_vledger_chain_id(_relaychain.get_xvid()));
    ASSERT_EQ(get_vledger_zone_index(_relaytable.get_xvid()), get_vledger_zone_index(_relaychain.get_xvid()));
    ASSERT_EQ(get_vledger_book_index(_relaytable.get_xvid()), get_vledger_book_index(_relaychain.get_xvid()));
    ASSERT_EQ(get_vledger_table_index(_relaytable.get_xvid()), get_vledger_table_index(_relaychain.get_xvid()));    
    ASSERT_EQ(get_vledger_ledger_id(_relaytable.get_xvid()), get_vledger_ledger_id(_relaychain.get_xvid()));


    ASSERT_TRUE(_relaytable.is_table_address());
    ASSERT_FALSE(_relaytable.is_user_address());
    ASSERT_FALSE(_relaytable.is_relay_address());

    ASSERT_FALSE(_relaychain.is_table_address());
    ASSERT_FALSE(_relaychain.is_user_address());
    ASSERT_TRUE(_relaychain.is_relay_address());
    // ASSERT_EQ(_relaytable.get_xvid(), _relaychain.get_xvid());
}
