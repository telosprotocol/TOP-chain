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

using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xstake;
using namespace top::contract;

class test_suite_zec_vote_contract_t : public xzec_vote_contract, public testing::Test {
public:
    test_suite_zec_vote_contract_t() : xzec_vote_contract(common::xnetwork_id_t{0}) {};

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

        return make_block(sys_contract_zec_reward_addr, tx);
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

xobject_ptr_t<store::xstore_face_t> test_suite_zec_vote_contract_t::m_store;
shared_ptr<xaccount_context_t> test_suite_zec_vote_contract_t::m_vote_account_ctx_ptr;
uint64_t test_suite_zec_vote_contract_t::m_original_balance;
std::string test_suite_zec_vote_contract_t::table_vote_contract_addr = std::string(sys_contract_sharding_vote_addr) + "@0";


/*TEST_F(test_suite_zec_vote_contract_t, on_receive_shard_votes) {
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

TEST_F(test_suite_zec_vote_contract_t, on_receive_shard_votes_1400_nodes) {
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
*/
TEST_F(test_suite_zec_vote_contract_t, test_handle_receive_shard_votes) {
    // case 1: different batch
    uint64_t report_time = 11;
    uint64_t last_report_time = 10;
    std::map<std::string, std::string> contract_adv_votes = {{"T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h", base::xstring_utl::tostring(100)}};
    std::map<std::string, std::string> merge_contract_adv_votes = {{"T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73", base::xstring_utl::tostring(200)}};
    auto ret = handle_receive_shard_votes(report_time, last_report_time, contract_adv_votes, merge_contract_adv_votes);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(merge_contract_adv_votes.size() == 1);
    auto iter = merge_contract_adv_votes.find("T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h");
    ASSERT_TRUE(iter != merge_contract_adv_votes.end());
    ASSERT_TRUE(xstring_utl::touint64(iter->second) == 100);

    // case 2: same batch
    report_time = 10;
    contract_adv_votes.clear();
    merge_contract_adv_votes.clear();
    contract_adv_votes.insert({"T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h", base::xstring_utl::tostring(100)});
    merge_contract_adv_votes.insert({"T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73", base::xstring_utl::tostring(200)});
    ret = handle_receive_shard_votes(report_time, last_report_time, contract_adv_votes, merge_contract_adv_votes);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(merge_contract_adv_votes.size() == 2);
    iter = merge_contract_adv_votes.find("T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h");
    ASSERT_TRUE(iter != merge_contract_adv_votes.end());
    ASSERT_TRUE(xstring_utl::touint64(iter->second) == 100);
    iter = merge_contract_adv_votes.find("T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73");
    ASSERT_TRUE(iter != merge_contract_adv_votes.end());
    ASSERT_TRUE(xstring_utl::touint64(iter->second) == 200);

    // case 3: earlier than last report
    report_time = 9;
    contract_adv_votes.clear();
    merge_contract_adv_votes.clear();
    contract_adv_votes.insert({"T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h", base::xstring_utl::tostring(100)});
    merge_contract_adv_votes.insert({"T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73", base::xstring_utl::tostring(200)});
    ret = handle_receive_shard_votes(report_time, last_report_time, contract_adv_votes, merge_contract_adv_votes);
    ASSERT_TRUE(ret == false);
}
