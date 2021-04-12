// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <string>

#define private public
#include "xbase/xobject_ptr.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xworkload_info.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
//#include "xvm/xrec/xelect/xbeacon_timer_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_workload_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_reward_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xchain_upgrade/xchain_upgrade_center.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"

using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xstake;
using namespace top::contract;

class test_suite_xworkload_contract_t : public testing::Test {
public:
    static void SetUpTestCase() {
        m_store = store::xstore_factory::create_store_with_memdb();
        auto mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
        std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
        std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
        auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);
        auto & config_center = top::config::xconfig_register_t::get_instance();

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

        // table vote contract
        xcontract_manager_t::instance().register_contract<xtable_vote_contract>(common::xaccount_address_t{sys_contract_sharding_vote_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_sharding_vote_addr},
                                                                          common::xaccount_address_t{sys_contract_sharding_vote_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_sharding_vote_addr}, m_store.get());

        // zec vote contract
        xcontract_manager_t::instance().register_contract<xzec_vote_contract>(common::xaccount_address_t{sys_contract_zec_vote_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_vote_addr},
                                                                          common::xaccount_address_t{sys_contract_zec_vote_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_vote_addr}, m_store.get());


        // workload contract
        xcontract_manager_t::instance().register_contract<xzec_workload_contract>(common::xaccount_address_t{sys_contract_zec_workload_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_workload_addr},
                                                                          common::xaccount_address_t{sys_contract_zec_workload_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_workload_addr}, m_store.get());

        // zec reward contract
        xcontract_manager_t::instance().register_contract<xzec_reward_contract>(common::xaccount_address_t{sys_contract_zec_reward_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_reward_addr},
                                                                          common::xaccount_address_t{sys_contract_zec_reward_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_reward_addr}, m_store.get());

        std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
        uint64_t node_morgage = 1000000000000;
        std::string node_types = "advance";

        int ret = registerNode(node_account, node_morgage, node_types);
        ASSERT_TRUE(ret == 0);
        for (auto i = 0; i < 1400; i++) {
            std::stringstream ss;
            ss << std::setw(40) << std::setfill('0') << i;
            auto node_account = ss.str();
            int ret = registerNode(node_account, node_morgage, node_types);
            ASSERT_TRUE(ret == 0);
        }

        uint64_t votes = 10000;
        ret = update_batch_stake(node_account, votes);
        ASSERT_TRUE(ret == 0);
    }

    static void TearDownTestCase() {}

    static int make_block(const std::string & address, const xtransaction_ptr_t & tx, uint64_t timer_height = 1) {
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
        ac.set_context_para(timer_height, "111", 1, 1);

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
        tstream << static_cast<uint32_t>(50);
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

    static int update_batch_stake(std::string node_account, uint64_t votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<std::string, std::string> adv_votes;
        adv_votes[node_account] = base::xstring_utl::tostring(votes);

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << adv_votes;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(table_vote_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("update_batch_stake");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_rec_registration_addr, tx);
    }

    static int on_receive_workload2(std::string node_account,
                                    int auditor_group_id,
                                    uint32_t auditor_txs,
                                    int validator_group_id,
                                    uint32_t validator_txs,
                                    int64_t table_pledge_balance_change_tgas) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<common::xcluster_address_t, xauditor_workload_info_t> auditor_workload_info;
        std::map<common::xcluster_address_t, xvalidator_workload_info_t> validator_workload_info;

        common::xnetwork_id_t   net_id{0};
        common::xzone_id_t      zone_id{0};
        common::xcluster_id_t   cluster_id{1};
        common::xgroup_id_t     group_id{0};

        group_id = common::xgroup_id_t{auditor_group_id};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xauditor_workload_info_t auditor_workload;
        auditor_workload.m_leader_count[node_account] = auditor_txs;
        auditor_workload_info[cluster] = auditor_workload;

        group_id = common::xgroup_id_t{validator_group_id};
        cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
        xvalidator_workload_info_t validator_workload;
        validator_workload.m_leader_count[node_account] = validator_txs;
        validator_workload_info[cluster] = validator_workload;

        top::base::xstream_t stream(base::xcontext_t::instance());
        MAP_OBJECT_SERIALIZE2(stream, auditor_workload_info);
        MAP_OBJECT_SERIALIZE2(stream, validator_workload_info);
        stream << table_pledge_balance_change_tgas;
        std::string workload_str = std::string((char *)stream.data(), stream.size());

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << workload_str;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        std::string table_workload_contract_addr = std::string(sys_contract_sharding_workload_addr) + "@0";
        tx->get_source_action().set_account_addr(table_workload_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_workload2");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_workload_addr, tx);
    }

    static int on_receive_shard_votes(std::string node_account, uint64_t votes, uint64_t cur_time) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<std::string, std::string> contract_adv_votes;
        contract_adv_votes[node_account] = xstring_utl::tostring(votes);

        top::base::xstream_t tstream(base::xcontext_t::instance());
        std::string target_action;
        chain_upgrade::xtop_chain_fork_config_center fork_config_center;
        auto fork_config = fork_config_center.chain_fork_config();
        if (chain_upgrade::xtop_chain_fork_config_center::is_forked(fork_config.reward_fork_point, cur_time)) {
            tstream << cur_time;
            tstream << contract_adv_votes;
            target_action = "on_receive_shard_votes_v2";
        } else {
            tstream << contract_adv_votes;
            target_action = "on_receive_shard_votes";
        }

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(table_vote_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_vote_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name(target_action);
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_vote_addr, tx);
    }

    static int on_receive_shard_votes2(std::map<std::string, std::string> contract_adv_votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << contract_adv_votes;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(table_vote_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_vote_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_shard_votes");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_vote_addr, tx);
    }

    static int on_zec_workload_timer(uint64_t onchain_timer_round) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << onchain_timer_round;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_timer");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_workload_addr, tx);
    }

    static int on_reward_timer(uint64_t onchain_timer_round) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << onchain_timer_round;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_timer");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_reward_addr, tx, onchain_timer_round);
    }

    /*static int calc_auditor_reward() {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<common::xcluster_address_t, cluster_workload_t> auditor_clusters_workloads;
        std::map<common::xcluster_address_t, cluster_workload_t> validator_clusters_workloads;
        common::xnetwork_id_t   net_id{0};
        common::xzone_id_t      zone_id{0};
        common::xcluster_id_t   cluster_id{1};
        common::xgroup_id_t     group_id{0};

        group_id = common::xgroup_id_t{1};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        std::string audit_leader = "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73";
        std::string validate_leader = "T00000LTHfpc9otZwKmNcXA24qiA9A6SMHKkxwkg";
        auto it2 = auditor_clusters_workloads.find(cluster);
        if (it2 == auditor_clusters_workloads.end()) {
            cluster_workload_t auditor_workload_info;
            std::pair<std::map<common::xcluster_address_t, cluster_workload_t>::iterator,bool> ret =
                        auditor_clusters_workloads.insert(std::make_pair(cluster, auditor_workload_info));
            it2 = ret.first;
        }
        uint32_t    txs_count = 149;
        it2->second.m_leader_count[audit_leader] += txs_count;
        it2->second.cluster_total_workload += txs_count;
        group_id = common::xgroup_id_t{2};
        cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
        auto it3 = auditor_clusters_workloads.find(cluster);
        if (it3 == auditor_clusters_workloads.end()) {
            cluster_workload_t auditor_workload_info;
            std::pair<std::map<common::xcluster_address_t, cluster_workload_t>::iterator,bool> ret =
                        auditor_clusters_workloads.insert(std::make_pair(cluster, auditor_workload_info));
            it3 = ret.first;
        }
        txs_count = 149;
        it3->second.m_leader_count[audit_leader] += txs_count;
        it3->second.cluster_total_workload += txs_count;

        xstream_t stream(xcontext_t::instance());
        MAP_SERIALIZE_SIMPLE(stream, auditor_clusters_workloads);
        MAP_SERIALIZE_SIMPLE(stream, validator_clusters_workloads);
        std::string workload_info = std::string((char *)stream.data(), stream.size());

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << onchain_timer_round;
        tstream << workload_info;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("calculate_reward");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_reward_addr, tx);
    }*/

    int is_mainnet_activated() {
        xactivation_record record;

        std::string value_str;
        auto ret = m_store->string_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, value_str);
        xassert(ret == 0);

        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(),
                        (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
            record.serialize_from(stream);
        }
        xdbg("[is_mainnet_activated] activated: %d, pid:%d\n", record.activated, getpid());
        return record.activated;
    }

    static xobject_ptr_t<store::xstore_face_t> m_store;
    static shared_ptr<xaccount_context_t> m_vote_account_ctx_ptr;
    static uint64_t m_original_balance;
    static std::string table_vote_contract_addr;
};

xobject_ptr_t<store::xstore_face_t> test_suite_xworkload_contract_t::m_store;
shared_ptr<xaccount_context_t> test_suite_xworkload_contract_t::m_vote_account_ctx_ptr;
uint64_t test_suite_xworkload_contract_t::m_original_balance;
std::string test_suite_xworkload_contract_t::table_vote_contract_addr = std::string(sys_contract_sharding_vote_addr) + "@0";

TEST_F(test_suite_xworkload_contract_t, on_receive_workload2) {
    int auditor_group_id = 1;
    int validator_group_id = 67;
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint32_t auditor_txs = 10;
    uint32_t validator_txs = 15;
    int64_t table_pledge_balance_change_tgas = 10000;

    int ret = on_receive_workload2(node_account, auditor_group_id, auditor_txs, validator_group_id, validator_txs, table_pledge_balance_change_tgas);
    ASSERT_TRUE(ret == 0);

    if (!is_mainnet_activated()) return;

    common::xnetwork_id_t   net_id{0};
    common::xzone_id_t      zone_id{0};
    common::xcluster_id_t   cluster_id{1};
    common::xgroup_id_t     group_id{0};
    group_id = common::xgroup_id_t{auditor_group_id};
    common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
    std::string cluster_key;
    {
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
    }
    std::string value;
    ret = m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_WORKLOAD_KEY, cluster_key, value);
    ASSERT_TRUE(ret == 0);

    cluster_workload_t workload;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    workload.serialize_from(stream);

    ASSERT_TRUE(workload.cluster_id == cluster_key);
    ASSERT_TRUE(workload.cluster_total_workload == auditor_txs);
    auto iter = workload.m_leader_count.find(node_account);
    ASSERT_TRUE(iter != workload.m_leader_count.end());
    ASSERT_TRUE(iter->first == node_account);
    ASSERT_TRUE(iter->second == auditor_txs);

    group_id = common::xgroup_id_t{validator_group_id};
    cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
    {
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
    }
    value.clear();
    ret = m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY, cluster_key, value);
    ASSERT_TRUE(ret == 0);
    cluster_workload_t workload2;
    base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    workload2.serialize_from(stream2);

    ASSERT_TRUE(workload2.cluster_id == cluster_key);
    ASSERT_TRUE(workload2.cluster_total_workload == validator_txs);
    auto iter2 = workload2.m_leader_count.find(node_account);
    ASSERT_TRUE(iter2 != workload2.m_leader_count.end());
    ASSERT_TRUE(iter2->first == node_account);
    ASSERT_TRUE(iter2->second == validator_txs);

    value.clear();
    ret = m_store->string_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_TGAS_KEY, value);
    ASSERT_TRUE(ret == 0);
    ASSERT_TRUE(xstring_utl::toint64(value) == table_pledge_balance_change_tgas);
}

TEST_F(test_suite_xworkload_contract_t, on_receive_workload_700_nodes) {
    int auditor_group_id = 1;
    int validator_group_id = 67;
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint32_t auditor_txs = 10;
    uint32_t validator_txs = 15;
    int64_t table_pledge_balance_change_tgas = 10000;
    int ret = 0;

    if (!is_mainnet_activated()) return;

    for (auto i = 0; i < 700; i++) {
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << i;
        auto node_account = ss.str();
        ret = on_receive_workload2(node_account, auditor_group_id, auditor_txs, validator_group_id, validator_txs, table_pledge_balance_change_tgas);
        ASSERT_TRUE(ret == 0);

        common::xnetwork_id_t   net_id{0};
        common::xzone_id_t      zone_id{0};
        common::xcluster_id_t   cluster_id{1};
        common::xgroup_id_t     group_id{0};
        group_id = common::xgroup_id_t{auditor_group_id};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        std::string cluster_key;
        {
            xstream_t stream(xcontext_t::instance());
            stream << cluster;
            cluster_key = std::string((const char*)stream.data(), stream.size());
        }
        std::string value;
        ret = m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_WORKLOAD_KEY, cluster_key, value);
        ASSERT_TRUE(ret == 0);

        cluster_workload_t workload;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
        workload.serialize_from(stream);

        ASSERT_TRUE(workload.cluster_id == cluster_key);
        ASSERT_TRUE(workload.cluster_total_workload == auditor_txs * (i + 2));
        auto iter = workload.m_leader_count.find(node_account);
        ASSERT_TRUE(iter != workload.m_leader_count.end());
        ASSERT_TRUE(iter->first == node_account);
        ASSERT_TRUE(iter->second == auditor_txs);

        group_id = common::xgroup_id_t{validator_group_id};
        cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
        {
            xstream_t stream(xcontext_t::instance());
            stream << cluster;
            cluster_key = std::string((const char*)stream.data(), stream.size());
        }
        value.clear();
        ret = m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY, cluster_key, value);
        ASSERT_TRUE(ret == 0);
        cluster_workload_t workload2;
        base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
        workload2.serialize_from(stream2);

        ASSERT_TRUE(workload2.cluster_id == cluster_key);
        ASSERT_TRUE(workload2.cluster_total_workload == validator_txs * (i + 2));
        auto iter2 = workload2.m_leader_count.find(node_account);
        ASSERT_TRUE(iter2 != workload2.m_leader_count.end());
        ASSERT_TRUE(iter2->first == node_account);
        ASSERT_TRUE(iter2->second == validator_txs);

        value.clear();
        ret = m_store->string_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_TGAS_KEY, value);
        ASSERT_TRUE(ret == 0);
        ASSERT_TRUE(xstring_utl::toint64(value) == table_pledge_balance_change_tgas * (i + 2));
    }
}

TEST_F(test_suite_xworkload_contract_t, on_zec_workload_timer) {
    top::config::xconfig_register_t::get_instance().set(config::xtask_num_per_round_onchain_goverance_parameter_t::name, 1);
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
    uint64_t onchain_timer_round = timer_interval + 1;

    if (!is_mainnet_activated()) return;

    int ret = on_zec_workload_timer(onchain_timer_round);
    ASSERT_TRUE(ret == 0);

    /*int32_t size;
    ret = m_store->map_size(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, size);
    ASSERT_TRUE(ret == 0);
    ASSERT_TRUE(size == 0);*/
}

TEST_F(test_suite_xworkload_contract_t, on_receive_shard_votes) {
    std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
    uint64_t votes = 10000;
    uint64_t cur_time = 10;

    if (!is_mainnet_activated()) return;

    int ret = on_receive_shard_votes(node_account, votes, cur_time);
    ASSERT_TRUE(ret == 0);
    std::string value;
    ret = m_store->map_get(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_TICKETS_KEY, table_vote_contract_addr, value);
    ASSERT_TRUE(ret == 0);

    std::map<std::string, std::string> contract_adv_votes;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    stream >> contract_adv_votes;

    auto iter = contract_adv_votes.find(node_account);
    ASSERT_TRUE(iter != contract_adv_votes.end());
    ASSERT_TRUE(xstring_utl::touint64(iter->second) == votes);
    ASSERT_TRUE(contract_adv_votes.size() == 1);

    chain_upgrade::xtop_chain_fork_config_center fork_config_center;
    auto fork_config = fork_config_center.chain_fork_config();
    {
        node_account = "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73";
        ret = on_receive_shard_votes(node_account, votes, cur_time);
        ASSERT_TRUE(ret == 0);
        std::string value;
        ret = m_store->map_get(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_TICKETS_KEY, table_vote_contract_addr, value);
        ASSERT_TRUE(ret == 0);
        std::map<std::string, std::string> contract_adv_votes;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
        stream >> contract_adv_votes;
        xdbg("[test_suite_xworkload_contract_t on_receive_shard_votes] contract_adv_votes size: %d", contract_adv_votes.size());
        if (chain_upgrade::xtop_chain_fork_config_center::is_forked(fork_config.reward_fork_point, cur_time)) {
            ASSERT_TRUE(contract_adv_votes.size() == 2);
        } else {
            ASSERT_TRUE(contract_adv_votes.size() == 1);
        }
    }

    {
        cur_time = 11;
        ret = on_receive_shard_votes(node_account, votes, cur_time);
        ASSERT_TRUE(ret == 0);
        std::string value;
        ret = m_store->map_get(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_TICKETS_KEY, table_vote_contract_addr, value);
        ASSERT_TRUE(ret == 0);
        std::map<std::string, std::string> contract_adv_votes;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
        stream >> contract_adv_votes;
        ASSERT_TRUE(contract_adv_votes.size() == 1);
    }
}

TEST_F(test_suite_xworkload_contract_t, on_receive_shard_votes_1400_nodes) {
    int nodes = 1400;
    uint64_t votes = 1000000;

    if (!is_mainnet_activated()) return;

    std::map<std::string, std::string> auditor_votes;
    for (int32_t i = 0; i < nodes; i++) {
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << i;
        auto node_account = ss.str();
        auditor_votes[node_account] = std::to_string(votes);
    }

    int ret = on_receive_shard_votes2(auditor_votes);
    ASSERT_TRUE(ret == 0);
    std::string value;
    ret = m_store->map_get(sys_contract_zec_vote_addr, XPORPERTY_CONTRACT_TICKETS_KEY, table_vote_contract_addr, value);
    ASSERT_TRUE(ret == 0);
    std::map<std::string, std::string> contract_adv_votes;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
    stream >> contract_adv_votes;
    xdbg("test_suite_xworkload_contract_t on_receive_shard_votes_1400_nodes, contract_adv_votes size: %d",
        contract_adv_votes.size());
    ASSERT_TRUE((int32_t)contract_adv_votes.size() == nodes);
    std::stringstream ss;
    ss << std::setw(40) << std::setfill('0') << 1;
    auto node_account = ss.str();
    auto iter = contract_adv_votes.find(node_account);
    ASSERT_TRUE(iter != contract_adv_votes.end());
    ASSERT_TRUE(xstring_utl::touint64(iter->second) == votes);
}

#define XSET_ONCHAIN_GOVERNANCE_PARAMETER(NAME, VALUE) static_cast<top::config::xconfig_register_t &>(top::config::xconfig_register_t::get_instance()).set(top::config::x ## NAME ## _onchain_goverance_parameter_t::name, VALUE)

TEST_F(test_suite_xworkload_contract_t, on_reward_timer) {
    top::config::xconfig_register_t::get_instance().set(config::xtask_num_per_round_onchain_goverance_parameter_t::name, 1);
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
    uint64_t onchain_timer_round = timer_interval + 1;

    if (!is_mainnet_activated()) return;

    chain_upgrade::xtop_chain_fork_config_center fork_config_center;
    auto fork_config = fork_config_center.chain_fork_config();
    if (chain_upgrade::xtop_chain_fork_config_center::is_forked(fork_config.reward_fork_point, onchain_timer_round)) {
        auto reg_contract_height = m_store->get_blockchain_height(sys_contract_rec_registration_addr);
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, reg_contract_height);
        onchain_timer_round = 1;
        for (uint64_t i = 0; i < reg_contract_height; i++) { // update registration contract height
            int ret = on_reward_timer(onchain_timer_round++);
            ASSERT_TRUE(ret == 0);
        }

        auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
        onchain_timer_round = timer_interval + 1;
        int ret = on_reward_timer(onchain_timer_round);
        ASSERT_TRUE(ret == 0);

        std::map<std::string, std::string> dispatch_tasks;
        ret = m_store->map_copy_get(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
        ASSERT_TRUE(ret == 0);

        int table_node_reward_tasks = 0;
        int table_vote_reward_tasks = 0;
        for (auto t : dispatch_tasks) {
            xstream_t stream(xcontext_t::instance(), (uint8_t *)t.second.c_str(), (uint32_t)t.second.size());
            xreward_dispatch_task task;
            task.serialize_from(stream);

            xdbg("[test_suite_xworkload_contract_t, on_reward_timer] task id: %s, onchain_timer_round: %llu, contract: %s, action: %s\n",
                 t.first.c_str(),
                 task.onchain_timer_round,
                 task.contract.c_str(),
                 task.action.c_str());
            if (task.action == XREWARD_CLAIMING_ADD_NODE_REWARD) {
                table_node_reward_tasks++;
            } else if (task.action == XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
                table_vote_reward_tasks++;
            }
        }
        //ASSERT_TRUE(table_node_reward_tasks == 2);
        ASSERT_TRUE(table_vote_reward_tasks == 2);
    } else {
        int ret = on_reward_timer(onchain_timer_round);
        ASSERT_TRUE(ret == 0);

        int32_t size;
        ret = m_store->map_size(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, size);
        ASSERT_TRUE(ret == 0);
        ASSERT_TRUE(size == 0);

        ret = on_reward_timer(onchain_timer_round + 1);
        ASSERT_TRUE(ret == 0);

        ret = m_store->map_size(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, size);
        ASSERT_TRUE(ret == 0);
        ASSERT_TRUE(size == 0);
    }
}

/*TEST_F(test_suite_xworkload_contract_t, calc_auditor_reward) {
    top::config::xconfig_register_t::get_instance().set(config::xtask_num_per_round_onchain_goverance_parameter_t::name, 1);
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
    uint64_t onchain_timer_round = timer_interval + 1;

    if (!is_mainnet_activated()) return;

    int ret = calc_auditor_reward(onchain_timer_round);
    ASSERT_TRUE(ret == 0);
}*/
