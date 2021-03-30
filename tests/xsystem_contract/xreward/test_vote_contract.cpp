// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>

#include <string>
#include <iterator>
#include <map>
#
#define private public
#include "xbasic/xobject_ptr.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xdata/xblocktool.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_reward_claiming_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xloader/xconfig_genesis_loader.h"


using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xstake;
using namespace top::contract;

class xtop_hash_t : public base::xhashplugin_t
{
public:
    xtop_hash_t()
        :base::xhashplugin_t(-1) //-1 = support every hash types
    {
    }
private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator = (const xtop_hash_t &) = delete;
public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input,enum_xhash_type type) override
    {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char*>(hash.data()), hash.size());
    }
};

static bool create_rootblock(const std::string & config_file) {
    auto genesis_loader = std::make_shared<top::loader::xconfig_genesis_loader_t>(config_file);
    // config::xconfig_register_t::get_instance().add_loader(genesis_loader);
    // config::xconfig_register_t::get_instance().load();
    xrootblock_para_t rootblock_para;
    if (false == genesis_loader->extract_genesis_para(rootblock_para)) {
        xerror("create_rootblock extract genesis para fail");
        return false;
    }
    if (false == xrootblock_t::init(rootblock_para)) {
        xerror("create_rootblock rootblock init fail");
        return false;
    }
    xinfo("create_rootblock success");
    return true;
}

class test_suite_xvote_contract_t : public testing::Test {
public:
    static void SetUpTestCase() {
        m_store = store::xstore_factory::create_store_with_memdb();
        auto mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
        static std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
        static std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
        auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);

        //std::string config_file = "/home/yinhp/xchain/src/libraries/xcomponent_administration/config/config.rec1.json";
        std::string config_file = "";
        using namespace std;
        using namespace base;

        auto hash_plugin = new xtop_hash_t();

        auto& config_center = top::config::xconfig_register_t::get_instance();
        auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>(config_file, "");
        config_center.add_loader(offchain_loader);
        config_center.load();
        config_center.remove_loader(offchain_loader);
        config_center.init_static_config();
        config_center.dump();

        if (false == create_rootblock(config_file)) {
            xassert(0);
        }

        config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_store), make_observer(mbus), make_observer(chain_timer.get()));
        config_center.add_loader(loader);
        config_center.load();

        // node registration contract
        xcontract_manager_t::instance().register_contract<xrec_registration_contract>(common::xaccount_address_t{sys_contract_rec_registration_addr},
                                                                                         common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_rec_registration_addr},
                                                                          common::xaccount_address_t{sys_contract_rec_registration_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_rec_registration_addr}, m_store.get());

        // timer contract
        /*xcontract_manager_t::instance().register_contract<top::elect::xbeacon_timer_contract>(common::xaccount_address_t{sys_contract_beacon_timer_addr},
                                                                                              common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_beacon_timer_addr},
                                                                          common::xaccount_address_t{sys_contract_beacon_timer_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_beacon_timer_addr}, m_store.get());*/

        // vote contract

        xcontract_manager_t::instance().register_contract<xtable_vote_contract>(common::xaccount_address_t{table_vote_contract_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{table_vote_contract_addr},
                                                                          common::xaccount_address_t{table_vote_contract_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{table_vote_contract_addr}, m_store.get());

        // reward claim contract
        xcontract_manager_t::instance().register_contract<top::xvm::system_contracts::reward::xtop_table_reward_claiming_contract>(common::xaccount_address_t{reward_claiming_contract_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{reward_claiming_contract_addr},
                                                                          common::xaccount_address_t{reward_claiming_contract_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{reward_claiming_contract_addr}, m_store.get());

        std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
        uint64_t node_morgage = 1000000000000;
        std::string node_types = "advance";
        int ret = registerNode(node_account, node_morgage, node_types);
        ASSERT_TRUE(ret == 0);
    }

    static void TearDownTestCase() {}

    static int make_block(const std::string & address, const xtransaction_ptr_t & tx) {
        auto account = m_store->clone_account(address);
        xassert(account != NULL);
        /*if (account == nullptr) {
            base::xvblock_t* genesis_block = xblocktool_t::create_genesis_lightunit(address, 1000000000000);
            assert(m_store->set_vblock(genesis_block));
            account = m_store->clone_account(address);
        }*/
        tx->set_last_nonce(account->account_send_trans_number() + 1);
        tx->set_digest();

        xaccount_context_t ac(address, m_store.get());

        xvm::xvm_service s;
        xtransaction_trace_ptr trace = s.deal_transaction(tx, &ac);
        xassert(0 == (int) trace->m_errno);
        store::xtransaction_result_t result;
        ac.get_transaction_result(result);

        xlightunit_block_para_t para1;
        para1.set_one_input_tx(tx);
        para1.set_transaction_result(result);
        base::xauto_ptr<base::xvblock_t> block(xlightunit_block_t::create_next_lightunit(para1, account));
        xassert(block);
        xassert(block->get_block_hash().empty());
        static uint64_t clock = 1;
        block->get_cert()->set_clock(clock++);
        block->get_cert()->set_viewid(block->get_height() + 1);
        block->get_cert()->set_validator({1, (uint64_t)-1});
        block->get_cert()->set_viewtoken(1111);
        if (block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
            block->set_extend_cert("1");
            block->set_extend_data("1");
        } else {
            block->set_verify_signature("1");
        }
        block->set_block_flag(base::enum_xvblock_flag_authenticated);
        block->set_block_flag(base::enum_xvblock_flag_locked);
        block->set_block_flag(base::enum_xvblock_flag_committed);
        block->set_block_flag(base::enum_xvblock_flag_executed);
        block->set_block_flag(base::enum_xvblock_flag_connected);

        auto ret = m_store->set_vblock(block->get_account(), block.get());
        xassert(ret);
        ret = m_store->execute_block(block.get());
        xassert(ret);
        return 0;
    }

    static int recreate_account(const std::string & account) {
        auto genesis_block = data::xblocktool_t::create_genesis_lightunit(account, 2000000000000);
        base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
        xassert(genesis_block);

        auto ret = m_store->set_vblock(genesis_block);
        if (!ret) {
            xerror("xtop_application::create_genesis_account store genesis block fail");
            return ret;
        }
        ret = m_store->execute_block(genesis_block);
        if (!ret) {
            xerror("xtop_application::create_genesis_account execute genesis block fail");
            return ret;
        }

        xaccount_ptr_t account_ptr  = m_store->query_account(account);
        if (account_ptr == NULL) {
            xerror("xtop_application::create_genesis_account execute genesis block fail");
            return 1;
        }

        xdbg("[recreate_account] account_balance: %llu", account_ptr->balance());
        m_original_balance = account_ptr->balance();

        m_vote_account_ctx_ptr = std::make_shared<xaccount_context_t>(
                sys_contract_rec_registration_addr,
                m_store.get());
        return 0;
    }

    static int registerNode(std::string node_account, uint64_t deposit, std::string node_types) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << deposit;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << node_types;
        tstream << std::string("test1");
        tstream << std::string();
        tstream << static_cast<uint32_t>(0);
#if defined XENABLE_MOCK_ZEC_STAKE
        tstream << node_account;
#endif

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(node_account);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("registerNode");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_rec_registration_addr, tx);
    }

    static int voteNode(std::string voter, std::string node_account, uint64_t votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << votes;
        sstream << std::string("");

        std::map<std::string, int64_t> vote_infos;
        vote_infos[node_account] = votes;

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << vote_infos;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_vote);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(voter);
        tx->get_source_action().set_action_type(xaction_type_source_null);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(table_vote_contract_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("voteNode");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(table_vote_contract_addr, tx);
    }

    static int unvoteNode(std::string voter, std::string node_account, uint64_t votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << votes;
        sstream << std::string("");

        std::map<std::string, int64_t> vote_infos;
        vote_infos[node_account] = votes;

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << vote_infos;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_abolish_vote);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(voter);
        tx->get_source_action().set_action_type(xaction_type_source_null);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(table_vote_contract_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("unvoteNode");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(table_vote_contract_addr, tx);
    }

    static int add_workload_award(std::string node_account, uint64_t issuance_clock_height, top::xstake::uint128_t reward) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << static_cast<uint64_t>(0);
        sstream << std::string("");

        std::map<std::string, top::xstake::uint128_t> rewards;
        rewards[node_account] = reward;

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << issuance_clock_height;
        tstream << rewards;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(reward_claiming_contract_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("recv_node_reward");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(reward_claiming_contract_addr, tx);
    }

    static int vote_reward3(std::string node_account, uint64_t issuance_clock_height, top::xstake::uint128_t reward) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << static_cast<uint64_t>(0);
        sstream << std::string("");

        std::map<std::string, top::xstake::uint128_t> rewards;
        rewards[node_account] = reward;

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << issuance_clock_height;
        tstream << rewards;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(reward_claiming_contract_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("recv_voter_dividend_reward");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(reward_claiming_contract_addr, tx);
    }

    static int claimNodeReward(std::string node_account) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << static_cast<uint64_t>(0);
        sstream << std::string("");

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(node_account);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(reward_claiming_contract_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("claimNodeReward");
        tx->get_target_action().set_action_param(std::string(""));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(reward_claiming_contract_addr, tx);
    }

    static int claimVoterDividend(const std::string & voter) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << static_cast<uint64_t>(0);
        sstream << std::string("");

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(voter);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(reward_claiming_contract_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("claimVoterDividend");
        tx->get_target_action().set_action_param(std::string(""));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(reward_claiming_contract_addr, tx);
    }

    static xobject_ptr_t<store::xstore_face_t> m_store;
    static shared_ptr<xaccount_context_t> m_vote_account_ctx_ptr;
    static uint64_t m_original_balance;
    static std::string table_vote_contract_addr;
    static std::string reward_claiming_contract_addr;
};

xobject_ptr_t<store::xstore_face_t> test_suite_xvote_contract_t::m_store;
shared_ptr<xaccount_context_t> test_suite_xvote_contract_t::m_vote_account_ctx_ptr;
uint64_t test_suite_xvote_contract_t::m_original_balance;
std::string test_suite_xvote_contract_t::table_vote_contract_addr = std::string(sys_contract_sharding_vote_addr) + "@0";
std::string test_suite_xvote_contract_t::reward_claiming_contract_addr = std::string(sys_contract_sharding_reward_claiming_addr) + "@0";

TEST_F(test_suite_xvote_contract_t, testUint128) {
    top::xstake::uint128_t reward = 300000000;
    std::string value;

    {
        base::xstream_t stream(base::xcontext_t::instance());
        stream << reward;
        value = std::string((char *)stream.data(), stream.size());
    }

    top::xstake::uint128_t reward2 = 0;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    stream >> reward2;
    xdbg("[test_suite_xvote_contract_t::testUint128] reward: [%llu, %u], reward2: [%llu, %u]",
        static_cast<uint64_t>(reward / xstake::REWARD_PRECISION),
        static_cast<uint32_t>(reward % xstake::REWARD_PRECISION),
        static_cast<uint64_t>(reward2 / xstake::REWARD_PRECISION),
        static_cast<uint32_t>(reward2 % xstake::REWARD_PRECISION));
    ASSERT_TRUE(reward2 == reward);
}

TEST_F(test_suite_xvote_contract_t, voteNode) {
    std::string voter = "T00000LPn1LhBzzpBj8DhHTR6GkQ7v4XQ6uVbmX7";
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint64_t votes = 10000;

    int ret = voteNode(voter, node_account, votes);
    ASSERT_TRUE(ret == 0);

    std::string value;
    uint32_t sub_map_no = (utl::xxh32_t::digest(voter) % 4) + 1;
    std::string property;
    property = property + XPORPERTY_CONTRACT_VOTES_KEY_BASE + "-" + std::to_string(sub_map_no);
    ret = m_store->map_get(table_vote_contract_addr, property, voter, value);
    ASSERT_TRUE(ret == 0);

    std::map<std::string, uint64_t> votes_table;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    stream >> votes_table;
    auto iter = votes_table.find(node_account);
    ASSERT_TRUE(iter != votes_table.end());
    ASSERT_TRUE(iter->second == votes);

    ret = m_store->map_get(table_vote_contract_addr, XPORPERTY_CONTRACT_POLLABLE_KEY, node_account, value);
    ASSERT_TRUE(ret == 0);
    ASSERT_TRUE(base::xstring_utl::touint64(value) == votes);
}

TEST_F(test_suite_xvote_contract_t, unvoteNode) {
    std::string voter = "T00000LPn1LhBzzpBj8DhHTR6GkQ7v4XQ6uVbmX7";
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint64_t votes = 300;

    int ret = unvoteNode(voter, node_account, votes);
    ASSERT_TRUE(ret == 0);

    std::string value;
    uint32_t sub_map_no = (utl::xxh32_t::digest(voter) % 4) + 1;
    std::string property;
    property = property + XPORPERTY_CONTRACT_VOTES_KEY_BASE + "-" + std::to_string(sub_map_no);
    ret = m_store->map_get(table_vote_contract_addr, property, voter, value);
    ASSERT_TRUE(ret == 0);

    std::map<std::string, uint64_t> votes_table;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    stream >> votes_table;
    auto iter = votes_table.find(node_account);
    ASSERT_TRUE(iter != votes_table.end());
    ASSERT_TRUE(iter->second == 10000 - votes);

    ret = m_store->map_get(table_vote_contract_addr, XPORPERTY_CONTRACT_POLLABLE_KEY, node_account, value);
    ASSERT_TRUE(ret == 0);
    ASSERT_TRUE(base::xstring_utl::touint64(value) == 10000 - votes);
}

TEST_F(test_suite_xvote_contract_t, add_workload_award) {
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint64_t issuance_clock_height = 100;
    top::xstake::uint128_t reward = 300000000;

    int ret = add_workload_award(node_account, issuance_clock_height, reward);
    ASSERT_TRUE(ret == 0);

    std::string value;
    ret = m_store->map_get(reward_claiming_contract_addr, XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account, value);
    ASSERT_TRUE(ret == 0);

    xreward_node_record record;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    record.serialize_from(stream);

    xdbg("test_suite_xvote_contract_t::add_workload_award m_accumulated: [%llu, %u]",
        static_cast<uint64_t>(record.m_accumulated / xstake::REWARD_PRECISION),
                     static_cast<uint32_t>(record.m_accumulated % xstake::REWARD_PRECISION));
    ASSERT_TRUE(record.m_accumulated == reward);
    ASSERT_TRUE(record.m_unclaimed == reward);
    ASSERT_TRUE(record.m_issue_time == issuance_clock_height);
}

TEST_F(test_suite_xvote_contract_t, vote_reward3) {
    std::string voter = "T00000LPn1LhBzzpBj8DhHTR6GkQ7v4XQ6uVbmX7";
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint64_t issuance_clock_height = 100;
    top::xstake::uint128_t reward = 300000000;

    int ret = vote_reward3(node_account, issuance_clock_height, reward);
    ASSERT_TRUE(ret == 0);

    std::string value;

    uint32_t sub_map_no = (utl::xxh32_t::digest(voter) % 4) + 1;
    std::string property;
    property = property + XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE + "-" + std::to_string(sub_map_no);
    ret = m_store->map_get(reward_claiming_contract_addr, property, voter, value);
    ASSERT_TRUE(ret == 0);

    xreward_record record;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    record.serialize_from(stream);

    ASSERT_TRUE(record.accumulated == reward);
    ASSERT_TRUE(record.unclaimed == reward);
    ASSERT_TRUE(record.issue_time == issuance_clock_height);

    bool found = false;
    node_record_t found_reward;
    for (auto const & node_reward : record.node_rewards) {
        if (node_reward.account == node_account) {
            found = true;
            found_reward = node_reward;
        }
    }
    ASSERT_TRUE(found == true);
    ASSERT_TRUE(found_reward.accumulated == reward);
    ASSERT_TRUE(found_reward.unclaimed == reward);
}

TEST_F(test_suite_xvote_contract_t, claimNodeReward) {
    std::string voter = "T00000LPn1LhBzzpBj8DhHTR6GkQ7v4XQ6uVbmX7";
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    top::xstake::uint128_t reward = 300000000;

    int ret = claimNodeReward(node_account);
    ASSERT_TRUE(ret == 0);

    std::string value;
    ret = m_store->map_get(reward_claiming_contract_addr, XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account, value);
    ASSERT_TRUE(ret == 0);

    xreward_node_record record;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    record.serialize_from(stream);

    ASSERT_TRUE(record.m_accumulated == reward);
    ASSERT_TRUE(record.m_unclaimed / xstake::REWARD_PRECISION * xstake::REWARD_PRECISION == 0);
}

TEST_F(test_suite_xvote_contract_t, claimVoterDividend) {
    std::string voter = "T00000LPn1LhBzzpBj8DhHTR6GkQ7v4XQ6uVbmX7";
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    top::xstake::uint128_t reward = 300000000;

    int ret = claimVoterDividend(voter);
    ASSERT_TRUE(ret == 0);

    std::string value;

    uint32_t sub_map_no = (utl::xxh32_t::digest(voter) % 4) + 1;
    std::string property;
    property = property + XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE + "-" + std::to_string(sub_map_no);
    ret = m_store->map_get(reward_claiming_contract_addr, property, voter, value);
    ASSERT_TRUE(ret == 0);

    xreward_record record;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    record.serialize_from(stream);

    ASSERT_TRUE(record.accumulated == reward);
    ASSERT_TRUE(record.unclaimed == reward % xstake::REWARD_PRECISION);

    for (auto const & node_reward : record.node_rewards) {
        ASSERT_TRUE(node_reward.accumulated == reward);
        ASSERT_TRUE(node_reward.unclaimed == 0);
    }
}

class xtable_vote_contract_test : public testing::Test {};

TEST_F(xtable_vote_contract_test, split_vote_trx) {
    std::size_t XVOTE_TRX_LIMIT = 10;
    xstake::xtable_vote_contract vote_contract{common::xnetwork_id_t{1}};

    {
        // not split
        std::map<std::string, std::string> vote;
        auto res = vote_contract.trx_split_helper(vote, XVOTE_TRX_LIMIT);
        ASSERT_TRUE(res.size() == 0);
    }

    {
        // not split
        std::map<std::string, std::string> vote;
        for (auto i = 0; i < 7; ++i) {
            vote[std::to_string(i)] = "content" + std::to_string(i);
        }
        auto res = vote_contract.trx_split_helper(vote, XVOTE_TRX_LIMIT);
        ASSERT_TRUE(res.size() == 1);
        ASSERT_EQ(vote, res[0]);

    }

    {
        //split
        std::map<std::string, std::string> vote;
        for (auto i = 0; i < 43; ++i) {
            vote[std::to_string(i)] = "content" + std::to_string(i);
        }
        auto res = vote_contract.trx_split_helper(vote, XVOTE_TRX_LIMIT);
        ASSERT_TRUE(res.size() == 5);
        for (std::size_t i = 0; i < res.size(); ++i) {
            std::map<std::string, std::string>::iterator start = vote.begin() ;
            std::advance(start, i * XVOTE_TRX_LIMIT);
            std::map<std::string, std::string>::iterator end = start;
            std::advance(end, XVOTE_TRX_LIMIT);
            if (i == res.size() - 1) {
                end = vote.end();
            }
            std::map<std::string, std::string> target{start, end};
            ASSERT_EQ(res[i], target);
        }

    }

    {
        //split
        std::map<std::string, std::string> vote;
        for (auto i = 0; i < 1000; ++i) {
            vote[std::to_string(i)] = "content" + std::to_string(i);
        }
        auto res = vote_contract.trx_split_helper(vote, XVOTE_TRX_LIMIT);
        ASSERT_TRUE(res.size() == 100);
        for (std::size_t i = 0; i < res.size(); ++i) {
            std::map<std::string, std::string>::iterator start = vote.begin() ;
            std::advance(start, i * XVOTE_TRX_LIMIT);
            std::map<std::string, std::string>::iterator end = start;
            std::advance(end, XVOTE_TRX_LIMIT);
            if (i == res.size() - 1) {
                end = vote.end();
            }
            std::map<std::string, std::string> target{start, end};
            ASSERT_EQ(res[i], target);

        }
    }

}
