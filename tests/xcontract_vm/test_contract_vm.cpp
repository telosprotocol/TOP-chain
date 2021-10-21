#define private public
#define protected public
#include "tests/mock/xvchain_creator.hpp"
#include "xbase/xobject_ptr.h"
#include "xcontract_vm/xaccount_vm.h"
#include "xcrypto/xckey.h"
#include "xdata/xblocktool.h"
#include "xdata/xtransaction_v2.h"
#include "xdemo_contract/xdemo_contract_a.h"
#include "xdemo_contract/xdemo_contract_b.h"
#include "xpbase/base/top_utils.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"

#include <gtest/gtest.h>
// #include "xblockstore/xblockstore_face.h"
// #include "xcontract_common/xcontract_state.h"
// #include "xcontract_runtime/xtop_action_generator.h"
// #include "xdata/xdata_common.h"
// #include "xdb/xdb_face.h"
// #include "xdb/xdb_factory.h"
// #include "xstore/xstore_face.h"
// #include "xsystem_contract_runtime/xsystem_action_runtime.h"
// #include "xsystem_contracts/xsystem_contract_addresses.h"
// #include "xsystem_contracts/xtransfer_contract.h"
// #include "xvledger/xvledger.h"
// #include "xvledger/xvstate.h"
// #include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract_new.h"
// #include "xvm/xsystem_contracts/xregistration/xrec_registration_contract_new.h"

NS_BEG3(top, tests, contract_runtime)

using namespace top::base;
using namespace top::contract_common;
using namespace top::contract_runtime;
using namespace top::contract_vm;
using namespace top::data;
using namespace top::mock;
using namespace top::xstake;

xvchain_creator creator;

class test_contract_vm : public testing::Test {
public:
    void SetUp() override {
        m_blockstore = creator.get_blockstore();
        m_manager = new system::xsystem_contract_manager_t();
        cs_para.m_clock = 5796740;
        cs_para.m_total_lock_tgas_token = 0;
        cs_para.m_proposal_height = 7;
        cs_para.m_account = "Ta0001@0";
        cs_para.m_random_seed = xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
    }
    void TearDown() override {
    }

    xvblockstore_t * m_blockstore;
    system::xsystem_contract_manager_t * m_manager{nullptr};
    xblock_consensus_para_t cs_para;
};

static const std::string user_address{"T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks"};
static const std::string public_key{"BFqS6Al19LkycuHhrHMuI/E1G6+rZi4NJTQ1w1U55UnMjhBnb8/ey4pj+Mn69lyVB0+r6GR6M6eett9Tv/yoizI="};
static const std::string sign_key{"NzjQLs3K3stpskP8j1VG5DKwZF2vvBJNLDaHAvxsFQA="};

TEST_F(test_contract_vm, test_send_tx) {
    const uint64_t last_nonce{10};
    const uint256_t last_hash{12345678};
    const uint64_t recv_num{1};
    const uint64_t unconfirm_num{1};

    m_manager->deploy_system_contract<system_contracts::xcontract_a_t>(
        common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));

    // tx
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    xstream_t param_stream(xcontext_t::instance());
    param_stream << std::string{"test_send_tx_str"};
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    tx->make_tx_run_contract("test_set_string_property", param);
    tx->set_different_source_target_address(user_address, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
    tx->set_digest();  
    utl::xecprikey_t pri_key_obj((uint8_t*)(DecodePrivateString(sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(enum_transaction_subtype_send);

    const uint256_t tx_hash = cons_tx->get_tx_hash_256();

    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);

    xobject_ptr_t<xvbstate_t> bstate = make_object_ptr<xvbstate_t>(user_address, 1, 0, "", "", 0, 0, 0);
    xobject_ptr_t<xvbstate_t> bstate_cmp = make_object_ptr<xvbstate_t>(user_address, 1, 0, "", "", 0, 0, 0);
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        if (bstate->find_property(XPROPERTY_TX_INFO) == false) {
            bstate->new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate_cmp->find_property(XPROPERTY_TX_INFO) == false) {
            bstate_cmp->new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        auto map_property = bstate->load_string_map_var(XPROPERTY_TX_INFO);
        auto map_property_cmp = bstate_cmp->load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(last_nonce);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(last_hash);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(recv_num);
            map_property->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(unconfirm_num);
            map_property->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
    }
    std::map<common::xaccount_address_t, observer_ptr<xvbstate_t>> state_pack;
    state_pack.insert(std::make_pair(common::xaccount_address_t{user_address}, make_observer(bstate.get())));

    xaccount_vm_t vm(make_observer(m_manager));
    auto result = vm.execute(input_txs, state_pack, cs_para);

    EXPECT_EQ(result.status.ec.value(), 0);
    EXPECT_EQ(result.success_tx_assemble.size(), 1);
    EXPECT_EQ(result.success_tx_assemble[0]->get_transaction(), tx.get());
    EXPECT_EQ(result.success_tx_assemble[0]->get_current_exec_status(), enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
    EXPECT_EQ(result.failed_tx_assemble.size(), 0);
    EXPECT_EQ(result.delay_tx_assemble.size(), 0);

    auto const & state_out = state_pack[common::xaccount_address_t{user_address}];
    {
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_nonce + 1);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
            auto value = top::from_bytes<uint256_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, tx_hash);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_RECVTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, recv_num);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, unconfirm_num + 1);
        }
        { EXPECT_EQ(false, state_out->find_property(xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY)); }
    }
    EXPECT_EQ(result.bincode_pack.size(), 1);
    EXPECT_EQ(result.binlog_pack.size(), 1);
    {
        std::string bincode;
        bstate_cmp->take_snapshot(bincode);
        EXPECT_NE(result.bincode_pack[common::xaccount_address_t{user_address}], bincode);
    }
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        auto map_property_cmp = bstate_cmp->load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(last_nonce + 1);
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(tx_hash);
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(unconfirm_num + 1);
            map_property_cmp->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        std::string bincode;
        bstate_cmp->take_snapshot(bincode);
        std::string binlog;
        canvas->encode(binlog);
        EXPECT_EQ(result.bincode_pack[common::xaccount_address_t{user_address}], bincode);
        EXPECT_EQ(result.binlog_pack[common::xaccount_address_t{user_address}], binlog);
    }
}

TEST_F(test_contract_vm, test_recv_tx) {
    const uint64_t last_nonce{10};
    const uint256_t last_hash{12345678};
    const uint64_t recv_num{1};
    const uint64_t unconfirm_num{1};
    const std::string property_string{"test_send_tx_str"};

    m_manager->deploy_system_contract<system_contracts::xcontract_a_t>(
        common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));

    // tx
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    xstream_t param_stream(xcontext_t::instance());
    param_stream << property_string;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    tx->make_tx_run_contract("test_set_string_property", param);
    tx->set_different_source_target_address(user_address, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
    tx->set_digest();  
    utl::xecprikey_t pri_key_obj((uint8_t*)(DecodePrivateString(sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);

    auto vblock = xblocktool_t::get_latest_committed_lightunit(m_blockstore, std::string{sys_contract_rec_standby_pool_addr});
    auto bstate = xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    xvbstate_t bstate_cmp(*(bstate.get()));
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        if (bstate->find_property(XPROPERTY_TX_INFO) == false) {
            bstate->new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate_cmp.find_property(XPROPERTY_TX_INFO) == false) {
            bstate_cmp.new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        auto map_property = bstate->load_string_map_var(XPROPERTY_TX_INFO);
        auto map_property_cmp = bstate_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(last_nonce);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(last_hash);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(recv_num);
            map_property->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(unconfirm_num);
            map_property->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
    }
    std::map<common::xaccount_address_t, observer_ptr<xvbstate_t>> state_pack;
    state_pack.insert(std::make_pair(common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, make_observer(bstate.get())));

    xaccount_vm_t vm(make_observer(m_manager));
    auto result = vm.execute(input_txs, state_pack, cs_para);
    EXPECT_EQ(result.status.ec.value(), 0);
    EXPECT_EQ(result.success_tx_assemble.size(), 1);
    EXPECT_EQ(result.success_tx_assemble[0]->get_transaction(), tx.get());
    EXPECT_EQ(result.success_tx_assemble[0]->get_current_exec_status(), enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
    EXPECT_EQ(result.failed_tx_assemble.size(), 0);
    EXPECT_EQ(result.delay_tx_assemble.size(), 0);

    auto const & state_out = state_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}];
    {
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_nonce);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
            auto value = top::from_bytes<uint256_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_hash);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_RECVTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, recv_num + 1);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, unconfirm_num);
        }
        {
            EXPECT_EQ(true, state_out->find_property(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY));
            auto string = state_out->load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY)->query();
            EXPECT_EQ(string, property_string);
        }
    }
    EXPECT_EQ(result.bincode_pack.size(), 1);
    EXPECT_EQ(result.binlog_pack.size(), 1);
    {
        std::string bincode;
        bstate_cmp.take_snapshot(bincode);
        EXPECT_NE(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], bincode);
    }
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        auto map_property_cmp = bstate_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(recv_num + 1);
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        auto string_property_cmp = bstate_cmp.load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
        {
            string_property_cmp->reset(property_string, canvas.get());
        }
        std::string bincode;
        bstate_cmp.take_snapshot(bincode);
        std::string binlog;
        canvas->encode(binlog);
        EXPECT_EQ(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], bincode);
        EXPECT_EQ(result.binlog_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], binlog);
    }
}

TEST_F(test_contract_vm, test_sync_call) {
    const uint64_t last_nonce{10};
    const uint256_t last_hash{12345678};
    const uint64_t recv_num{1};
    const uint64_t unconfirm_num{1};
    const std::string property_string{"test_send_tx_str"};

    m_manager->deploy_system_contract<system_contracts::xcontract_a_t>(
        common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));
    m_manager->deploy_system_contract<system_contracts::xcontract_b_t>(
        common::xaccount_address_t{sys_contract_rec_registration_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));

    // tx
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    xstream_t param_stream(xcontext_t::instance());
    param_stream << common::xaccount_address_t{sys_contract_rec_registration_addr};
    param_stream << std::string("test_set_string_property");
    param_stream << property_string;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    tx->make_tx_run_contract("test_sync_call", param);
    tx->set_different_source_target_address(user_address, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
    tx->set_digest();  
    utl::xecprikey_t pri_key_obj((uint8_t*)(DecodePrivateString(sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);

    xobject_ptr_t<xvblock_t> const & vblock_a = xblocktool_t::get_latest_committed_lightunit(m_blockstore, std::string{sys_contract_rec_standby_pool_addr});
    xobject_ptr_t<xvbstate_t> const & bstate_a = xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock_a.get());
    xvbstate_t bstate_a_cmp(*(bstate_a.get()));
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        if (bstate_a->find_property(XPROPERTY_TX_INFO) == false) {
            bstate_a->new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate_a_cmp.find_property(XPROPERTY_TX_INFO) == false) {
            bstate_a_cmp.new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        auto map_property = bstate_a->load_string_map_var(XPROPERTY_TX_INFO);
        auto map_property_cmp = bstate_a_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(last_nonce);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(last_hash);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(recv_num);
            map_property->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(unconfirm_num);
            map_property->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
    }
    xobject_ptr_t<xvblock_t> const & vblock_b = xblocktool_t::get_latest_committed_lightunit(m_blockstore, std::string{sys_contract_rec_registration_addr});
    xobject_ptr_t<xvbstate_t> const & bstate_b = xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock_b.get());
    xvbstate_t bstate_b_cmp(*(bstate_b.get()));
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        if (bstate_b->find_property(XPROPERTY_TX_INFO) == false) {
            bstate_b->new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate_b_cmp.find_property(XPROPERTY_TX_INFO) == false) {
            bstate_b_cmp.new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        auto map_property = bstate_b->load_string_map_var(XPROPERTY_TX_INFO);
        auto map_property_cmp = bstate_b_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(last_nonce);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(last_hash);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(recv_num);
            map_property->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(unconfirm_num);
            map_property->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
    }
    std::map<common::xaccount_address_t, observer_ptr<xvbstate_t>> state_pack;
    state_pack.insert(std::make_pair(common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, make_observer(bstate_a.get())));
    state_pack.insert(std::make_pair(common::xaccount_address_t{sys_contract_rec_registration_addr}, make_observer(bstate_b.get())));

    xaccount_vm_t vm(make_observer(m_manager));
    auto result = vm.execute(input_txs, state_pack, cs_para);
    EXPECT_EQ(result.status.ec.value(), 0);
    EXPECT_EQ(result.success_tx_assemble.size(), 1);
    EXPECT_EQ(result.success_tx_assemble[0]->get_transaction(), tx.get());
    EXPECT_EQ(result.success_tx_assemble[0]->get_current_exec_status(), enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
    EXPECT_EQ(result.failed_tx_assemble.size(), 0);
    EXPECT_EQ(result.delay_tx_assemble.size(), 0);

    auto const & state_a_out = state_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}];
    {
        {
            auto string = state_a_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_nonce);
        }
        {
            auto string = state_a_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
            auto value = top::from_bytes<uint256_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_hash);
        }
        {
            auto string = state_a_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_RECVTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, recv_num + 1);
        }
        {
            auto string = state_a_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, unconfirm_num);
        }
        {
            EXPECT_EQ(true, state_a_out->find_property(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY));
            auto string = state_a_out->load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY)->query();
            EXPECT_EQ(string, "sync_call_a_to_b");
        }
    }
    auto const & state_b_out = state_pack[common::xaccount_address_t{sys_contract_rec_registration_addr}];
    {
        {
            auto string = state_b_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_nonce);
        }
        {
            auto string = state_b_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
            auto value = top::from_bytes<uint256_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_hash);
        }
        {
            auto string = state_b_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_RECVTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, recv_num);
        }
        {
            auto string = state_b_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, unconfirm_num);
        }
        {
            EXPECT_EQ(true, state_b_out->find_property(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY));
            auto string = state_b_out->load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY)->query();
            EXPECT_EQ(string, property_string);
        }
    }
    EXPECT_EQ(result.bincode_pack.size(), 2);
    EXPECT_EQ(result.binlog_pack.size(), 2);
    {
        {
            std::string bincode;
            bstate_a_cmp.take_snapshot(bincode);
            EXPECT_NE(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], bincode);
        }
        {
            std::string bincode;
            bstate_b_cmp.take_snapshot(bincode);
            EXPECT_NE(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_registration_addr}], bincode);
        }
    }
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        auto map_property_cmp = bstate_a_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(recv_num + 1);
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        auto string_property_cmp = bstate_a_cmp.load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
        {
            string_property_cmp->reset("sync_call_a_to_b", canvas.get());
        }
        std::string bincode;
        bstate_a_cmp.take_snapshot(bincode);
        std::string binlog;
        canvas->encode(binlog);
        EXPECT_EQ(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], bincode);
        EXPECT_EQ(result.binlog_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], binlog);
    }
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        auto string_property_cmp = bstate_b_cmp.load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
        {
            string_property_cmp->reset(property_string, canvas.get());
        }
        std::string bincode;
        bstate_b_cmp.take_snapshot(bincode);
        std::string binlog;
        canvas->encode(binlog);
        EXPECT_EQ(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_registration_addr}], bincode);
        EXPECT_EQ(result.binlog_pack[common::xaccount_address_t{sys_contract_rec_registration_addr}], binlog);
    }
}

TEST_F(test_contract_vm, test_async_call) {
    const uint64_t last_nonce{10};
    const uint256_t last_hash{12345678};
    const uint64_t recv_num{1};
    const uint64_t unconfirm_num{1};
    const std::string property_string{"test_async_call_str"};

    m_manager->deploy_system_contract<system_contracts::xcontract_a_t>(
        common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));
    m_manager->deploy_system_contract<system_contracts::xcontract_b_t>(
        common::xaccount_address_t{sys_contract_rec_registration_addr}, common::xnode_type_t::rec, {}, {}, {}, {}, make_observer(m_blockstore));

    // tx
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    xstream_t param_stream(xcontext_t::instance());
    param_stream << common::xaccount_address_t{sys_contract_rec_registration_addr};
    param_stream << std::string("test_set_string_property");
    param_stream << property_string;
    param_stream << contract_common::xfollowup_transaction_schedule_type_t::immediately;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    tx->make_tx_run_contract("test_async_call", param);
    tx->set_different_source_target_address(user_address, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
    tx->set_digest();  
    utl::xecprikey_t pri_key_obj((uint8_t*)(DecodePrivateString(sign_key).data()));
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

    const uint256_t tx_hash = cons_tx->get_tx_hash_256();

    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);

    xobject_ptr_t<xvblock_t> const & vblock = xblocktool_t::get_latest_committed_lightunit(m_blockstore, std::string{sys_contract_rec_standby_pool_addr});
    xobject_ptr_t<xvbstate_t> const & bstate = xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    xvbstate_t bstate_cmp(*(bstate.get()));
    {
        xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        if (bstate->find_property(XPROPERTY_TX_INFO) == false) {
            bstate->new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        if (bstate_cmp.find_property(XPROPERTY_TX_INFO) == false) {
            bstate_cmp.new_string_map_var(XPROPERTY_TX_INFO, canvas.get());
        }
        auto map_property = bstate->load_string_map_var(XPROPERTY_TX_INFO);
        auto map_property_cmp = bstate_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        {
            auto value = top::to_bytes<uint64_t>(last_nonce);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint256_t>(last_hash);
            map_property->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(recv_num);
            map_property->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
        {
            auto value = top::to_bytes<uint64_t>(unconfirm_num);
            map_property->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
            map_property_cmp->insert(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        }
    }

    std::map<common::xaccount_address_t, observer_ptr<xvbstate_t>> state_pack;
    state_pack.insert(std::make_pair(common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, make_observer(bstate.get())));

    xaccount_vm_t vm(make_observer(m_manager));
    auto result = vm.execute(input_txs, state_pack, cs_para);
    EXPECT_EQ(result.status.ec.value(), 0);
    EXPECT_EQ(result.success_tx_assemble.size(), 2);
    EXPECT_EQ(result.success_tx_assemble[0]->get_transaction(), tx.get());
    EXPECT_EQ(result.success_tx_assemble[0]->get_current_exec_status(), enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
    EXPECT_EQ(result.success_tx_assemble[1]->get_current_exec_status(), enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success);
    EXPECT_EQ(result.success_tx_assemble[1]->get_transaction()->get_target_action().get_action_name(), std::string{"test_set_string_property"});
    std::string str;
    {
        base::xstream_t param_stream(base::xcontext_t::instance());
        param_stream << property_string;
        str = std::string{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())};
    }
    EXPECT_EQ(result.success_tx_assemble[1]->get_transaction()->get_target_action().get_action_param(), str);
    EXPECT_EQ(result.success_tx_assemble[1]->get_transaction()->get_source_addr(), sys_contract_rec_standby_pool_addr);
    EXPECT_EQ(result.success_tx_assemble[1]->get_transaction()->get_target_addr(), sys_contract_rec_registration_addr);
    EXPECT_EQ(result.success_tx_assemble[1]->get_transaction()->get_last_nonce(), last_nonce);
    EXPECT_EQ(result.failed_tx_assemble.size(), 0);
    EXPECT_EQ(result.delay_tx_assemble.size(), 0);

    auto const & state_out = state_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}];
    {
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, last_nonce + 1);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
            auto value = top::from_bytes<uint256_t>({std::begin(string), std::end(string)});
            EXPECT_NE(value, last_hash);
            EXPECT_NE(value, tx_hash);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_RECVTX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, recv_num + 1);
        }
        {
            auto string = state_out->load_string_map_var(XPROPERTY_TX_INFO)->query(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
            auto value = top::from_bytes<uint64_t>({std::begin(string), std::end(string)});
            EXPECT_EQ(value, unconfirm_num + 1);
        }
        {
            EXPECT_EQ(true, state_out->find_property(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY));
            auto string = state_out->load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY)->query();
            EXPECT_EQ(string, property_string);
        }
    }
    EXPECT_EQ(result.bincode_pack.size(), 1);
    {
        {
            std::string bincode;
            bstate_cmp.take_snapshot(bincode);
            EXPECT_NE(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], bincode);
        }
    }
    {
        // xobject_ptr_t<xvcanvas_t> canvas = make_object_ptr<xvcanvas_t>();
        // auto map_property_cmp = bstate_a_cmp.load_string_map_var(XPROPERTY_TX_INFO);
        // {
        //     auto value = top::to_bytes<uint64_t>(recv_num + 1);
        //     map_property_cmp->insert(XPROPERTY_TX_INFO_RECVTX_NUM, {std::begin(value), std::end(value)}, canvas.get());
        // }
        // auto string_property_cmp = bstate_a_cmp.load_string_var(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY);
        // {
        //     string_property_cmp->reset("sync_call_a_to_b", canvas.get());
        // }
        // std::string bincode;
        // bstate_a_cmp.take_snapshot(bincode);
        // std::string binlog;
        // canvas->encode(binlog);
        // EXPECT_EQ(result.bincode_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], bincode);
        // EXPECT_EQ(result.binlog_pack[common::xaccount_address_t{sys_contract_rec_standby_pool_addr}], binlog);
    }
}

// TEST_F(test_contract_vm, account_vm) {
//     std::string account{"T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks"};
//     std::string public_key{"BFqS6Al19LkycuHhrHMuI/E1G6+rZi4NJTQ1w1U55UnMjhBnb8/ey4pj+Mn69lyVB0+r6GR6M6eett9Tv/yoizI="};
//     xblock_consensus_para_t cs_para;
//     {
//         cs_para.m_clock = 5796740;
//         cs_para.m_total_lock_tgas_token = 0;
//         cs_para.m_proposal_height = 7;
//         cs_para.m_account = "Ta0001@0";
//         cs_para.m_random_seed = base::xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
//     }

//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
//     base::xstream_t param_stream(base::xcontext_t::instance());
//     param_stream << common::xtop_node_id{account};
//     param_stream << common::xnetwork_id_t{ static_cast<common::xnetwork_id_t::value_type>(top::config::to_chainid(XGET_CONFIG(chain_name))) };
// #if defined XENABLE_MOCK_ZEC_STAKE
//     ENUM_SERIALIZE(param_stream, common::xrole_type_t::advance);
//     param_stream << public_key;
//     param_stream << static_cast<uint64_t>(top::config::to_chainid(XGET_CONFIG(chain_name)));
// #endif
//     param_stream << std::string("1.1.1");
//     std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
//     tx->make_tx_run_contract("nodeJoinNetwork2", param);
//     tx->set_different_source_target_address(account, sys_contract_rec_standby_pool_addr);
//     tx->set_fire_and_expire_time(600);
//     tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
//     tx->set_tx_type(xtransaction_type_run_contract_new);
//     xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
//     cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

//     std::vector<xcons_transaction_ptr_t> input_txs;
//     input_txs.emplace_back(cons_tx);

//     mock::xvchain_creator creator;
//     base::xvblockstore_t* blockstore = creator.get_blockstore();

//     // contract_runtime::system::xtop_system_contract_manager::instance()->initialize(blockstore);
//     contract_runtime::system::xtop_system_contract_manager::instance()->deploy(make_observer(blockstore));
//     // auto system_contract_manager = make_unique<contract_runtime::system::xsystem_contract_manager_t>();
//     auto * system_contract_manager = contract_runtime::system::xsystem_contract_manager_t::instance();
//     xassert(system_contract_manager != nullptr);

//     auto latest_vblock = xblocktool_t::get_latest_committed_lightunit(blockstore, std::string{sys_contract_rec_standby_pool_addr});
//     auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
//     auto bstate_cmp = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);

//     contract_vm::xaccount_vm_t vm(make_observer(system_contract_manager));
//     auto result = vm.execute(input_txs, bstate, cs_para);
//     EXPECT_EQ(result.status.ec.value(), 0);
//     EXPECT_EQ(result.success_tx_assemble.size(), 2);
//     EXPECT_EQ(result.failed_tx_assemble.size(), 0);
//     EXPECT_EQ(result.delay_tx_assemble.size(), 0);
//     EXPECT_EQ(result.success_tx_assemble[0]->get_source_addr(), account);
//     EXPECT_EQ(result.success_tx_assemble[0]->get_target_addr(), sys_contract_rec_standby_pool_addr);
//     EXPECT_EQ(result.success_tx_assemble[1]->get_source_addr(), sys_contract_rec_standby_pool_addr);
//     EXPECT_EQ(result.success_tx_assemble[1]->get_target_addr(), sys_contract_rec_registration_addr);
// }

// TEST_F(test_contract_vm, test_follow_up_delay) {
//     mock::xvchain_creator creator;
//     base::xvblockstore_t* blockstore = creator.get_blockstore();
//     // contract_runtime::system::xtop_system_contract_manager::instance()->initialize(blockstore);
//     // contract_runtime::system::xtop_system_contract_manager::instance()->m_blockstore = make_observer(blockstore);
//     contract_runtime::system::xtop_system_contract_manager::instance()->deploy_system_contract<system_contracts::xtransfer_contract_t>(
//         common::xaccount_address_t{sys_contract_zec_reward_addr}, {}, {}, {}, {}, {}, make_observer(blockstore));

//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
//     base::xstream_t param_stream(base::xcontext_t::instance());
//     param_stream << uint64_t(1000);
//     std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
//     tx->make_tx_run_contract("follow_up_delay", param);
//     tx->set_different_source_target_address(sys_contract_zec_reward_addr, sys_contract_zec_reward_addr);
//     tx->set_fire_and_expire_time(600);
//     tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
//     // tx->set_tx_type(xtransaction_type_run_contract_new);
//     xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
//     cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

//     std::vector<xcons_transaction_ptr_t> input_txs;
//     input_txs.emplace_back(cons_tx);
//     auto * system_contract_manager = contract_runtime::system::xsystem_contract_manager_t::instance();
//     xassert(system_contract_manager != nullptr);

//     contract_vm::xaccount_vm_t vm(make_observer(system_contract_manager));

//     auto latest_vblock = xblocktool_t::get_latest_committed_lightunit(blockstore, std::string{sys_contract_zec_reward_addr});
//     auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
//     xblock_consensus_para_t cs_para;
//     {
//         cs_para.m_clock = 5796740;
//         cs_para.m_total_lock_tgas_token = 0;
//         cs_para.m_proposal_height = 7;
//         cs_para.m_account = "Ta0001@0";
//         cs_para.m_random_seed = base::xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
//     }
//     auto result = vm.execute(input_txs, bstate, cs_para);

//     EXPECT_EQ(result.status.ec.value(), 0);
//     EXPECT_EQ(result.delay_tx_assemble.size(), enum_vledger_const::enum_vbucket_has_tables_count);

//     auto nonce = 1;
//     auto amount = 1000;
//     for (auto const & tx : result.delay_tx_assemble) {
//         EXPECT_EQ(tx->get_source_addr(), sys_contract_zec_reward_addr);
//         EXPECT_EQ(tx->get_target_addr(), std::string{sys_contract_sharding_reward_claiming_addr} + "@" + std::to_string(nonce - 1));
//         auto tx_nonce = tx->get_tx_nonce();
//         EXPECT_EQ(nonce++, tx_nonce);
//     }
// }

NS_END3