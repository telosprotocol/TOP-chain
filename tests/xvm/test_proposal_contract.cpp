// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>
#include <string>
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbase/xobject_ptr.h"
#include "xdbstore/xstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xdata/xtransaction.h"
#include "xdata/xnative_contract_address.h"
#include "xconfig/xpredefined_configurations.h"
#include "xvm/xsystem_contracts/tcc/xrec_proposal_contract.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xconfig/xconfig_update_parameter_action.h"

#define private public
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xdata/xgenesis_data.h"
#include "xvm/manager/xcontract_manager.h"


using namespace top;
using namespace top::xvm;
using namespace top::tcc;
using namespace top::contract;
using namespace top::xvm::xcontract;


class test_proposal_contract: public testing::Test {

public:

    static void SetUpTestCase() {
        auto m_store = store::xstore_factory::create_store_with_memdb();
        auto mbus =  top::make_unique<mbus::xmessage_bus_t>(true, 1000);
        std::shared_ptr<xbase_io_context_wrapper_t> io_object = std::make_shared<xbase_io_context_wrapper_t>();
        std::shared_ptr<xbase_timer_driver_t> timer_driver = std::make_shared<xbase_timer_driver_t>(io_object);
        auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);
        auto& config_center = top::config::xconfig_register_t::get_instance();

        config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(mbus), make_observer(chain_timer));
        config_center.add_loader(loader);
        config_center.load();
        config_center.init_static_config();

        // tcc contract
        xcontract_manager_t::instance().register_contract<xrec_proposal_contract>(common::xaccount_address_t{sys_contract_rec_tcc_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_rec_tcc_addr}, common::xaccount_address_t{sys_contract_rec_tcc_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_rec_tcc_addr});
    }

    static data::xtransaction_ptr_t submit_proposal(std::string const& source_param, std::string const& target_param) {
        xaction_t source_action;
        xaction_t destination_action;

        source_action.set_account_addr("T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73");
        source_action.set_action_param(source_param);

        destination_action.set_account_addr(sys_contract_rec_tcc_addr);
        destination_action.set_action_name("submitProposal");
        destination_action.set_action_param(target_param);

        data::xtransaction_ptr_t submit_proposal_trx = make_object_ptr<xtransaction_t>();
        submit_proposal_trx->set_source_action(source_action);
        submit_proposal_trx->set_target_action(destination_action);
        return submit_proposal_trx;

    }

    static data::xtransaction_ptr_t withdraw_proposal(std::string const& target_param) {
        xaction_t source_action;
        xaction_t destination_action;

        source_action.set_account_addr("T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73");

        destination_action.set_account_addr(sys_contract_rec_tcc_addr);
        destination_action.set_action_name("withdrawProposal");
        destination_action.set_action_param(target_param);

        data::xtransaction_ptr_t withdraw_proposal_trx = make_object_ptr<xtransaction_t>();
        withdraw_proposal_trx->set_source_action(source_action);
        withdraw_proposal_trx->set_target_action(destination_action);
        return withdraw_proposal_trx;
    }

    static data::xtransaction_ptr_t vote_proposal(std::string const& target_param) {
        xaction_t source_action;
        xaction_t destination_action;

        source_action.set_account_addr("T00000Ldf7KcME5YaNvtFsr6jCFwNU9i7NeZ1b5a");

        destination_action.set_account_addr(sys_contract_rec_tcc_addr);
        destination_action.set_action_name("tccVote");
        destination_action.set_action_param(target_param);

        data::xtransaction_ptr_t vote_proposal_trx = make_object_ptr<xtransaction_t>();
        vote_proposal_trx->set_source_action(source_action);
        vote_proposal_trx->set_target_action(destination_action);
        return vote_proposal_trx;
    }

    static shared_ptr<xaccount_context_t> m_tcc_proposal_account_ctx_ptr;
};

shared_ptr<xaccount_context_t> test_proposal_contract::m_tcc_proposal_account_ctx_ptr;


TEST_F(test_proposal_contract, submit_param_proposal) {

    m_tcc_proposal_account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_rec_tcc_addr, store::xstore_factory::create_store_with_memdb().get());

    xtcc_transaction_ptr_t tcc_genesis = std::make_shared<xtcc_transaction_t>();
    m_tcc_proposal_account_ctx_ptr->map_create(ONCHAIN_PARAMS);
    for (auto const& entry: tcc_genesis->m_initial_values) {
        m_tcc_proposal_account_ctx_ptr->map_set(ONCHAIN_PARAMS, entry.first, entry.second);
    }

    m_tcc_proposal_account_ctx_ptr->string_create(SYSTEM_GENERATED_ID);
    m_tcc_proposal_account_ctx_ptr->string_set(SYSTEM_GENERATED_ID, "0");
    m_tcc_proposal_account_ctx_ptr->map_create(PROPOSAL_MAP_ID);

    base::xstream_t source_param(base::xcontext_t::instance());
    source_param << std::string("TOP");
    source_param << (uint64_t)100000000;

    base::xstream_t target_stream(base::xcontext_t::instance());
    target_stream << std::string("min_table_block_report");
    target_stream << std::string("30");
    target_stream << (uint8_t)1;
    target_stream << (uint64_t)300;

    auto trx_ptr = submit_proposal(std::string((char*)source_param.data(), source_param.size()) , std::string((char*)target_stream.data(), target_stream.size()));

    xvm_service vs;
    xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_tcc_proposal_account_ctx_ptr.get());
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);
}

TEST_F(test_proposal_contract, submit_community_proposal) {

    m_tcc_proposal_account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_rec_tcc_addr, store::xstore_factory::create_store_with_memdb().get());

    xtcc_transaction_ptr_t tcc_genesis = std::make_shared<xtcc_transaction_t>();
    m_tcc_proposal_account_ctx_ptr->map_create(ONCHAIN_PARAMS);
    for (auto const& entry: tcc_genesis->m_initial_values) {
        m_tcc_proposal_account_ctx_ptr->map_set(ONCHAIN_PARAMS, entry.first, entry.second);
    }

    m_tcc_proposal_account_ctx_ptr->string_create(SYSTEM_GENERATED_ID);
    m_tcc_proposal_account_ctx_ptr->string_set(SYSTEM_GENERATED_ID, "0");
    m_tcc_proposal_account_ctx_ptr->map_create(PROPOSAL_MAP_ID);

    base::xstream_t source_param(base::xcontext_t::instance());
    source_param << std::string("TOP");
    source_param << (uint64_t)100000000;

    base::xstream_t target_stream(base::xcontext_t::instance());
    target_stream << std::string("T-!-Ebj8hBvoLdvcEEUwNZ423zM3Kh9d4nL1Ug");
    target_stream << std::to_string(100000000);
    target_stream << (uint8_t)2;
    target_stream << (uint64_t)300;

    auto trx_ptr = submit_proposal(std::string((char*)source_param.data(), source_param.size()) , std::string((char*)target_stream.data(), target_stream.size()));

    xvm_service vs;
    xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_tcc_proposal_account_ctx_ptr.get());
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);
}

TEST_F(test_proposal_contract, withdraw_proposal) {

    // first sumbmit proposal
    m_tcc_proposal_account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_rec_tcc_addr, store::xstore_factory::create_store_with_memdb().get());

    xtcc_transaction_ptr_t tcc_genesis = std::make_shared<xtcc_transaction_t>();
    m_tcc_proposal_account_ctx_ptr->map_create(ONCHAIN_PARAMS);
    for (auto const& entry: tcc_genesis->m_initial_values) {
        m_tcc_proposal_account_ctx_ptr->map_set(ONCHAIN_PARAMS, entry.first, entry.second);
    }

    m_tcc_proposal_account_ctx_ptr->string_create(SYSTEM_GENERATED_ID);
    m_tcc_proposal_account_ctx_ptr->string_set(SYSTEM_GENERATED_ID, "0");
    m_tcc_proposal_account_ctx_ptr->map_create(PROPOSAL_MAP_ID);

    base::xstream_t source_param(base::xcontext_t::instance());
    source_param << std::string("TOP");
    source_param << (uint64_t)100000000;

    base::xstream_t target_stream(base::xcontext_t::instance());
    target_stream << std::string("min_table_block_report");
    target_stream << std::string("30");
    target_stream << (uint8_t)1;
    target_stream << (uint64_t)300;

    auto trx_ptr = submit_proposal(std::string((char*)source_param.data(), source_param.size()) , std::string((char*)target_stream.data(), target_stream.size()));

    xvm_service vs;
    xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_tcc_proposal_account_ctx_ptr.get());
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);

    // then withdraw
    base::xstream_t withdraw_target_stream(base::xcontext_t::instance());
    withdraw_target_stream << std::string("1");
    trx_ptr = withdraw_proposal(std::string((char*)withdraw_target_stream.data(), withdraw_target_stream.size()));

    trace = vs.deal_transaction(trx_ptr, m_tcc_proposal_account_ctx_ptr.get());
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);
}

TEST_F(test_proposal_contract, vote_proposal) {

    // first sumbmit proposal
    m_tcc_proposal_account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_rec_tcc_addr, store::xstore_factory::create_store_with_memdb().get());

    xtcc_transaction_ptr_t tcc_genesis = std::make_shared<xtcc_transaction_t>();
    m_tcc_proposal_account_ctx_ptr->map_create(ONCHAIN_PARAMS);
    for (auto const& entry: tcc_genesis->m_initial_values) {
        m_tcc_proposal_account_ctx_ptr->map_set(ONCHAIN_PARAMS, entry.first, entry.second);
    }

    m_tcc_proposal_account_ctx_ptr->string_create(SYSTEM_GENERATED_ID);
    m_tcc_proposal_account_ctx_ptr->string_set(SYSTEM_GENERATED_ID, "0");
    m_tcc_proposal_account_ctx_ptr->map_create(PROPOSAL_MAP_ID);
    m_tcc_proposal_account_ctx_ptr->map_create(VOTE_MAP_ID);

    base::xstream_t source_param(base::xcontext_t::instance());
    source_param << std::string("TOP");
    source_param << (uint64_t)100000000;

    base::xstream_t target_stream(base::xcontext_t::instance());
    target_stream << std::string("min_table_block_report");
    target_stream << std::string("30");
    target_stream << (uint8_t)1;
    target_stream << (uint64_t)300;

    auto trx_ptr = submit_proposal(std::string((char*)source_param.data(), source_param.size()) , std::string((char*)target_stream.data(), target_stream.size()));

    xvm_service vs;
    xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_tcc_proposal_account_ctx_ptr.get());
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);

    // then vote
    base::xstream_t vote_target_stream(base::xcontext_t::instance());
    vote_target_stream << std::string("1");
    vote_target_stream << true;
    trx_ptr = vote_proposal(std::string((char*)vote_target_stream.data(), vote_target_stream.size()));

    trace = vs.deal_transaction(trx_ptr, m_tcc_proposal_account_ctx_ptr.get());
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);
}

TEST_F(test_proposal_contract, test_incremental_utl) {

    {
        auto function_string = top::config::xconfig_utl::incremental_add_bwlist("addr1,addr2", "addr3,addr4");
        std::string target_string = "addr1,addr2,addr3,addr4";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_add_bwlist("addr1,addr2,addr3,addr4,addr5",
                "addr1,addr3,addr6");
        std::string target_string = "addr1,addr2,addr3,addr4,addr5,addr6";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_add_bwlist("addr1,addr2,addr3,addr4,addr5",
                "addr1,addr6");
        std::string target_string = "addr1,addr2,addr3,addr4,addr5,addr6";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_delete_bwlist("addr1,addr2", "addr1");
        std::string target_string = "addr2";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_delete_bwlist("addr1,addr2,addr3,addr4,addr5,addr5",
                "addr5");
        std::string target_string = "addr1,addr2,addr3,addr4,addr5";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_delete_bwlist("addr1,addr2,addr3,addr4,addr5",
                "addr7");
        std::string target_string = "addr1,addr2,addr3,addr4,addr5";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_delete_bwlist("addr1,addr2,addr3,addr4,addr5",
                "addr1,addr2,addr3,addr4,addr5");
        std::string target_string = "";
        ASSERT_EQ(function_string, target_string);
        function_string = top::config::xconfig_utl::incremental_add_bwlist("addr1", "addr1,addr3,addr6");
        target_string = "addr1,addr3,addr6";
        ASSERT_EQ(function_string, target_string);
    }

    {
        auto function_string = top::config::xconfig_utl::incremental_add_bwlist("addr1,addr2,addr3", "addr4,addr5,addr5");
        std::string target_string = "addr1,addr2,addr3,addr4,addr5,addr5";
        ASSERT_EQ(function_string, target_string);
        function_string = top::config::xconfig_utl::incremental_delete_bwlist("addr1,addr2,addr3,addr4,addr5,addr5", "addr5");
        target_string = "addr1,addr2,addr3,addr4,addr5";
        ASSERT_EQ(function_string, target_string);
    }


}







