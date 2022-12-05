#include "gtest/gtest.h"

#include "test_common.hpp"

#include "xvledger/xvpropertyprove.h"
#include "xdata/xblocktool.h"
#include "xblockmaker/xtable_maker.h"
#include "xstatestore/xstatestore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_address.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::blockmaker;

class test_propertyreceipt : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_propertyreceipt, propertyreceipt_1) {
    

}

// inner table tx will not has receipt
TEST_F(test_propertyreceipt, propertyreceipt_inner_table_tx) {
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    mocktable.store_genesis_units(resources->get_blockstore());

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 2);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 3);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    auto tableblocks = mocktable.get_history_tables();
    // auto tablestate = resources->get_xblkstatestore()->get_block_state(tableblocks[1].get());

    xvproperty_prove_ptr_t propreceipt = nullptr;// xblocktool_t::create_receiptid_property_prove(tableblocks[1].get(), tableblocks[3].get(), tablestate.get());
    data::xtablestate_ptr_t tablestate = nullptr;

    auto ret = statestore::xstatestore_hub_t::instance()->get_receiptid_state_and_prove(common::xaccount_address_t(tableblocks[1]->get_account()), tableblocks[1].get(), propreceipt, tablestate);
    xassert(ret == false);
    xassert(propreceipt == nullptr);
}

TEST_F(test_propertyreceipt, propertyreceipt_between_table_tx) {
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = mock::xdatamock_address::make_unit_address(base::enum_chain_zone_consensus_index, 9);

    mocktable.store_genesis_units(resources->get_blockstore());

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 2);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 3);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    auto tableblocks = mocktable.get_history_tables();
    // auto tablestate = resources->get_xblkstatestore()->get_block_state(tableblocks[1].get());

    xvproperty_prove_ptr_t propreceipt = nullptr;// xblocktool_t::create_receiptid_property_prove(tableblocks[1].get(), tableblocks[3].get(), tablestate.get());
    data::xtablestate_ptr_t tablestate = nullptr;

    auto ret = statestore::xstatestore_hub_t::instance()->get_receiptid_state_and_prove(common::xaccount_address_t(tableblocks[1]->get_account()), tableblocks[1].get(), propreceipt, tablestate);
    xassert(ret == true);
    xassert(propreceipt != nullptr);


    std::string propreceipt_bin;
    propreceipt->serialize_to_string(propreceipt_bin);
    std::cout << "propreceipt_bin size=" << propreceipt_bin.size() << std::endl;

    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)propreceipt_bin.data(), (uint32_t)propreceipt_bin.size());
    xdataunit_t* _dataunit = xdataunit_t::read_from(_stream);
    xassert(_dataunit != nullptr);
    xvproperty_prove_t* _propreceipt = dynamic_cast<xvproperty_prove_t*>(_dataunit);
    xassert(_propreceipt != nullptr);
    xvproperty_prove_ptr_t propreceipt2;
    propreceipt2.attach(_propreceipt);
    xassert(propreceipt2->is_valid());

    base::xreceiptid_state_ptr_t receiptid_state = xblocktool_t::get_receiptid_from_property_prove(propreceipt2);
    xassert(receiptid_state->get_self_tableid() == mocktable.get_short_table_id());
    xassert(receiptid_state->get_block_height() == 1);
    xassert(receiptid_state->get_unconfirm_tx_num() == 2);
}
#if 0 // TODO(jimmy) depend on xstatestore
TEST_F(test_propertyreceipt, propertyreceipt_3) {
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(200, resources->get_blockstore()); // for make full-table
    auto tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(resources->get_blockstore()->store_block(mocktable, block.get()));
    }    

    for (uint32_t i = 0; i < tableblocks.size(); i++) {
        if (tableblocks[i]->get_block_class() == base::enum_xvblock_class_full) {
            auto tablestate = resources->get_xblkstatestore()->get_block_state(tableblocks[i].get());
            xassert(tablestate != nullptr);
            xvproperty_prove_ptr_t propreceipt = xblocktool_t::create_receiptid_property_prove(tableblocks[i].get(), tableblocks[i+2].get(), tablestate.get());
            xassert(propreceipt != nullptr);
            xassert(propreceipt->is_valid());

            std::string propreceipt_bin;
            propreceipt->serialize_to_string(propreceipt_bin);
            std::cout << "propreceipt_bin size=" << propreceipt_bin.size() << std::endl;

            base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)propreceipt_bin.data(), (uint32_t)propreceipt_bin.size());
            xdataunit_t* _dataunit = xdataunit_t::read_from(_stream);
            xassert(_dataunit != nullptr);
            xvproperty_prove_t* _propreceipt = dynamic_cast<xvproperty_prove_t*>(_dataunit);
            xassert(_propreceipt != nullptr);
            xvproperty_prove_ptr_t propreceipt2;
            propreceipt2.attach(_propreceipt);
            xassert(propreceipt2->is_valid());

            base::xreceiptid_state_ptr_t receiptid_state = xblocktool_t::get_receiptid_from_property_prove(propreceipt2);
            xassert(receiptid_state->get_self_tableid() == mocktable.get_short_table_id());
            xassert(receiptid_state->get_block_height() == i);
            auto all_pairs = receiptid_state->get_all_receiptid_pairs();
            std::cout << "all_pairs=" << all_pairs->dump() << std::endl;
            return;
        }
    }
    xassert(false);  // not find full-table
}
#endif