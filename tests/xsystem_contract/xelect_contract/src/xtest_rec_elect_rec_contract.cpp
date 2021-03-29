// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#define private public

#include "xbasic/xobject_ptr.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xserialization/xserialization.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_archive_contract.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_edge_contract.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_rec_contract.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_zec_contract.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_elect_consensus_group_contract.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_group_association_contract.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_standby_pool_contract.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"

NS_BEG3(top, tests, election)

using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xvm::system_contracts::rec;
using namespace top::xvm::system_contracts::zec;
using namespace top::xstake;
using namespace top::contract;

using data::election::xstandby_result_store_t;

class xtop_hash_t : public base::xhashplugin_t {
public:
    xtop_hash_t()
      : base::xhashplugin_t(-1)  //-1 = support every hash types
    {}

private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator=(const xtop_hash_t &) = delete;

public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input, enum_xhash_type type) override {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    }
};

static bool create_rootblock(const std::string & config_file) {
    auto genesis_loader = std::make_shared<top::loader::xconfig_genesis_loader_t>(config_file);
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

class test_contract_on_timer : public testing::Test {
public:
    void SetUp() {
        m_store = store::xstore_factory::create_store_with_memdb();
        auto mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
        std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
        std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
        auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);

        std::string config_file = "";

        auto hash_plugin = new xtop_hash_t();

        auto & config_center = top::config::xconfig_register_t::get_instance();
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

#define contract_init(class, sys_address)                                                                                                                                          \
    xcontract_manager_t::instance().register_contract<class>(common::xaccount_address_t{sys_address}, common::xtopchain_network_id);                                               \
    xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_address}, common::xaccount_address_t{sys_address});                           \
    xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_address}, m_store.get());
        contract_init(xrec_registration_contract, sys_contract_rec_registration_addr);
        contract_init(xrec_standby_pool_contract_t, sys_contract_rec_standby_pool_addr);
        contract_init(xrec_elect_rec_contract_t, sys_contract_rec_elect_rec_addr);
        contract_init(xrec_elect_zec_contract_t, sys_contract_rec_elect_zec_addr);
        contract_init(xrec_elect_archive_contract_t, sys_contract_rec_elect_archive_addr);
        contract_init(xrec_elect_edge_contract_t, sys_contract_rec_elect_edge_addr);
        contract_init(xzec_standby_pool_contract_t, sys_contract_zec_standby_pool_addr);
        contract_init(xgroup_association_contract_t, sys_contract_zec_group_assoc_addr);
        contract_init(xzec_elect_consensus_group_contract_t, sys_contract_zec_elect_consensus_addr);
#undef contract_init
    }

    void TearDown() {}

    void update_standby_pool(const std::string & address) {
        auto account = m_store->clone_account(address);
        xassert(account != NULL);

        xaccount_context_t account_context(address, m_store.get());

        xstandby_result_store_t standby_result_store;

        auto bytes = codec::msgpack_encode(standby_result_store);
        std::string obj_str{std::begin(bytes), std::end(bytes)};
        account_context.string_set(XPROPERTY_CONTRACT_STANDBYS_KEY, obj_str, true);
    }

    void set_genesis_string() {
        auto account = m_store->clone_account(sys_contract_zec_elect_consensus_addr);
        xassert(account != NULL);

        xaccount_context_t account_context(sys_contract_zec_elect_consensus_addr, m_store.get());

        account_context.string_set(XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY, "1", true);
    }

    void exec_on_timer(const std::string & address, const xtransaction_ptr_t & tx) {
        auto account = m_store->clone_account(address);
        xassert(account != NULL);

        tx->set_last_nonce(account->account_send_trans_number() + 1);
        tx->set_digest();

        xaccount_context_t account_context(address, m_store.get());
        account_context.m_random_seed = "12345";

        xvm::xvm_service s;
        xtransaction_trace_ptr trace = s.deal_transaction(tx, &account_context);
    }

    int contract_on_timer(uint64_t onchain_timer_round, const string & contract_address) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << onchain_timer_round;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(contract_address);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(contract_address);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_timer");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        exec_on_timer(contract_address, tx);
        return 0;
    }
    static xobject_ptr_t<store::xstore_face_t> m_store;
};

xobject_ptr_t<store::xstore_face_t> test_contract_on_timer::m_store;

TEST_F(test_contract_on_timer, on_timer) {
    update_standby_pool(sys_contract_rec_standby_pool_addr);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_rec_standby_pool_addr) == 0);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_rec_elect_rec_addr) == 0);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_rec_elect_zec_addr) == 0);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_rec_elect_archive_addr) == 0);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_rec_elect_edge_addr) == 0);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_zec_standby_pool_addr) == 0);
    ASSERT_TRUE(contract_on_timer(0, sys_contract_zec_elect_consensus_addr) == 0);  // genesis
    set_genesis_string();
    ASSERT_TRUE(contract_on_timer(0, sys_contract_zec_elect_consensus_addr) == 0);  // non_genesis
}

NS_END3
