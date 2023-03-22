#include "gtest/gtest.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include "xconfig/xconfig_face.h"
#include "xconfig/xconfig_register.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_maker.hpp"
#include "xdata/xtransaction_v1.h"
#include "xdata/xverifier/xblacklist_verifier.h"
#include "xdata/xverifier/xtx_verifier.h"
#include "xdata/xverifier/xverifier_errors.h"
#include "xdata/xverifier/xverifier_utl.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xrpc/xuint_format.h"
#include "xtxexecutor/xunit_service_error.h"

using namespace top;
using namespace top::xverifier;
using namespace top::utl;

class test_xverifier : public testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }

    std::string generate_address(xecprikey_t & priv, const uint8_t addr_type = '0', const uint16_t net_id = 0, const std::string & parent_addr = "") {
        auto pub = priv.get_public_key();
        if (parent_addr == "") {
            return pub.to_address(addr_type, net_id);
        } else {
            return pub.to_address(parent_addr, addr_type, net_id);
        }
    }

    xecdsasig_t generate_signature(xecprikey_t const & priv, uint256_t const & digest) {
        return priv.sign(digest);
    }

    void transaction_set_signature(data::xtransaction_ptr_t const & trx_ptr, xecdsasig_t sig) {
        trx_ptr->set_authorization(std::string(reinterpret_cast<char *>(sig.get_compact_signature()), sig.get_compact_signature_size()));
    }

    bool address_is_valid(std::string const & addr) {
        xkeyaddress_t keyaddr{addr};
        xinfo("address is %s", addr.c_str());
        return keyaddr.is_valid();
    }
};
// TODO(jimmy)
#if 0
TEST_F(test_xverifier, DISABLED_trx_verifier) {
    using namespace top::xverifier;

    auto m_store = xstore_factory::create_store_with_memdb();
    auto mbus =  top::make_unique<mbus::xmessage_bus_t>();
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);
    auto& config_center = top::config::xconfig_register_t::get_instance();

    config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_store), make_observer(mbus), make_observer(chain_timer));
    config_center.add_loader(loader);
    config_center.load();

    // initialization
    xecprikey_t priv;
    auto account_addr = generate_address(priv, '0', 0);
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    xtransaction_ptr_t trx_ptr = datamock.create_tx_create_user_account(account_addr);
    auto now = xverifier::xtx_utl::get_gmttime_s();

    // verify
    trx_ptr->set_tx_subtype(enum_transaction_subtype_self);
    auto ret = xtx_verifier::verify_address(trx_ptr.get());
    ASSERT_EQ(xverifier_error::xverifier_success, ret);
    ret = xtx_verifier::verify_tx_duration_expiration(trx_ptr.get(), now);
    ASSERT_EQ(xverifier_error::xverifier_success, ret);
    ret = xtx_verifier::verify_tx_fire_expiration(trx_ptr.get(), now + 550);
    ASSERT_EQ(xverifier_error::xverifier_error_tx_fire_expired, ret);

    ret = xtx_verifier::verify_tx_fire_expiration(trx_ptr.get(), now + 50);
    ASSERT_EQ(xverifier_error::xverifier_success, ret);

    ret = xtx_verifier::verify_account_min_deposit(ASSET_TOP(1));
    ASSERT_EQ(xverifier_error::xverifier_success, ret);
    ret = xtx_verifier::verify_tx_min_deposit(ASSET_TOP(1));
    ASSERT_EQ(xverifier_error::xverifier_success, ret);
    ret = xtx_verifier::verify_tx_min_deposit(ASSET_TOP(0.01));
    ASSERT_EQ(xverifier_error::xverifier_error_tx_min_deposit_invalid, ret);

    // signature
    auto trx_digest = trx_ptr->digest();
    auto sig = generate_signature(priv, trx_digest);
    uint8_t out_publickey_data[65] = { 0 };
    if (xsecp256k1_t::get_publickey_from_signature(sig, trx_digest, out_publickey_data))//signature is valid
    {
        xecpubkey_t verify_key(out_publickey_data);
        ASSERT_EQ(verify_key.to_address('0', 0), account_addr);
    }
    transaction_set_signature(trx_ptr, sig);

    ret = xtx_verifier::verify_tx_signature(trx_ptr.get(), make_observer(m_store));
    ASSERT_EQ(xverifier_error::xverifier_success, ret);

}
#endif
TEST_F(test_xverifier, generate_address) {
    // just for java sdk debug address
    // xecprikey_t priv;
    // auto priv_hex_str = top::xrpc::uint_to_str(priv.data(), priv.size());
    // xinfo("alone account private key hex str is %s", priv_hex_str.c_str());
    ////std::string addr1 = "T-s-13MT6WokNQhCuSG8RUDSThKzoEhedPgufC";
    // auto pub = priv.get_public_key();
    // auto pub_hex_str = top::xrpc::uint_to_str(pub.data(), pub.size());
    // xinfo("alone account public key hex str is %s", pub_hex_str.c_str());

    // uint8_t addr_type = '0';
    // uint16_t net_id = 65535;
    // uint32_t version_uint32 = (((uint32_t)net_id) << 8) | ((uint32_t)addr_type);
    // xinfo("alone account version_uint32: %d", version_uint32);
    // auto alone_account = generate_address(priv, addr_type, net_id);
    // xinfo("alone account the address is %s", alone_account.c_str());

    // xecprikey_t priv_sub;
    // priv_hex_str = top::xrpc::uint_to_str(priv_sub.data(), priv_sub.size());
    // xinfo("sub account private key hex str is %s", priv_hex_str.c_str());

    // pub = priv_sub.get_public_key();
    // pub_hex_str = top::xrpc::uint_to_str(pub.data(), pub.size());
    // xinfo("sub account public key hex str is %s", pub_hex_str.c_str());

    // addr_type = '3';
    // net_id = 65535;
    // version_uint32 = (((uint32_t)net_id) << 8) | ((uint32_t)addr_type);
    // xinfo("sub account version_uint32: %d", version_uint32);

    // auto sub_account = generate_address(priv_sub, addr_type, net_id, alone_account);
    // xinfo("sub account the address is %s", sub_account.c_str());

    // verify address
    xecprikey_t priv;
    auto alone_addr = generate_address(priv, '0', 0, "");
    ASSERT_EQ(address_is_valid(alone_addr), true);

    auto custom_sc_addr = generate_address(priv, '3', 0, alone_addr);
    ASSERT_EQ(address_is_valid(custom_sc_addr), true);
}

data::xtransaction_ptr_t make_a_normal_transfer_tx() {
    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    auto src_addr = xcrypto_util::make_address_by_random_key(addr_type, ledger_id);
    auto target_addr = xcrypto_util::make_address_by_random_key(addr_type, ledger_id);

    uint256_t last_hash;
    uint64_t last_nonce = 0;

    data::xtransaction_ptr_t trx_ptr = data::xtransaction_maker::make_transfer_tx_v1(src_addr, last_nonce, target_addr, 100, 0, 0, 0);
    return trx_ptr;
}

TEST_F(test_xverifier, trx_verifier_validation_normal) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_validation_1_local_tx) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    trx_ptr->set_same_source_target_address(sys_contract_rec_elect_edge_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_same_source_target_address(sys_contract_rec_elect_edge_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_validation_2_burn_tx) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string source_addr = trx_ptr->source_address().to_string();

    trx_ptr->set_same_source_target_address(black_hole_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_different_source_target_address(sys_contract_rec_elect_edge_addr, black_hole_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_different_source_target_address(sys_contract_rec_elect_edge_addr, black_hole_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_transfer);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_different_source_target_address(source_addr, black_hole_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_transfer);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}
TEST_F(test_xverifier, trx_verifier_validation_3_addr_type) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    std::string dst_addr = trx_ptr->target_address().to_string();

    std::string bad_src_addr = base::xvaccount_t::make_account_address(common::rec_table_base_address.to_string(), 1);
    trx_ptr->set_different_source_target_address(bad_src_addr, dst_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_different_source_target_address(src_addr, bad_src_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    // trx_ptr->set_different_source_target_address(sys_contract_sharding_vote_addr, sys_contract_sharding_statistic_info_addr);
    // trx_ptr->set_tx_type(xtransaction_type_transfer);
    // trx_ptr->set_digest();
    // trx_ptr->set_len();
    // ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_different_source_target_address(src_addr, sys_contract_rec_elect_edge_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_transfer);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}
TEST_F(test_xverifier, trx_verifier_validation_4_len) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    std::string dst_addr = trx_ptr->target_address().to_string();
    trx_ptr->set_tx_len(10000);
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    // trx_ptr->get_source_action().set_action_size(10000);
    // ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}
TEST_F(test_xverifier, trx_verifier_validation_5_system_contract_call_limit) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    std::string dst_addr = trx_ptr->target_address().to_string();
    trx_ptr->set_different_source_target_address(src_addr, sys_contract_zec_workload_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_different_source_target_address(src_addr, sys_contract_rec_registration_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}
TEST_F(test_xverifier, trx_verifier_validation_6_unuse_member) {
    data::xtransaction_ptr_t trx_base_ptr = make_a_normal_transfer_tx();
    trx_base_ptr->add_ref();
    data::xtransaction_v1_ptr_t trx_ptr;
    trx_ptr.attach(dynamic_cast<data::xtransaction_v1_t *>(trx_base_ptr.get()));
    trx_ptr->set_tx_version(1);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(1);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(0);
    trx_ptr->set_from_ledger_id(1);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(0);
    trx_ptr->set_from_ledger_id(0);
    trx_ptr->set_random_nonce(1);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(0);
    trx_ptr->set_from_ledger_id(0);
    trx_ptr->set_random_nonce(0);
    trx_ptr->set_challenge_proof("111");
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(0);
    trx_ptr->set_from_ledger_id(0);
    trx_ptr->set_random_nonce(0);
    trx_ptr->set_challenge_proof({});
    trx_ptr->set_ext("222");
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(0);
    trx_ptr->set_from_ledger_id(0);
    trx_ptr->set_random_nonce(0);
    trx_ptr->set_challenge_proof({});
    trx_ptr->set_ext({});
    trx_ptr->set_premium_price(11);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_version(0);
    trx_ptr->set_to_ledger_id(0);
    trx_ptr->set_from_ledger_id(0);
    trx_ptr->set_random_nonce(0);
    trx_ptr->set_challenge_proof({});
    trx_ptr->set_ext({});
    trx_ptr->set_premium_price(0);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_validation_7_tx_type) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    trx_ptr->set_tx_type(100);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_validation_8_shard_contract_addr) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    base::xvaccount_t _src_vaddr(src_addr);

    std::string bad_dst_addr = base::xvaccount_t::make_account_address(sys_contract_sharding_vote_addr, _src_vaddr.get_ledger_subaddr());
    trx_ptr->set_different_source_target_address(src_addr, bad_dst_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    uint16_t wrong_subaddr = (_src_vaddr.get_ledger_subaddr() + 1) & enum_vbucket_has_tables_count_mask;
    bad_dst_addr = base::xvaccount_t::make_account_address(sys_contract_sharding_vote_addr, wrong_subaddr);
    trx_ptr->set_different_source_target_address(src_addr, bad_dst_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    trx_ptr->set_different_source_target_address(src_addr, sys_contract_sharding_vote_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    // trx_ptr->set_different_source_target_address(src_addr, sys_contract_sharding_vote_addr);
    // trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    // trx_ptr->set_digest();
    // trx_ptr->set_len();
    // ASSERT_NE(xtx_verifier::verify_send_tx_validation(trx_ptr.get(), nullptr), xverifier_error::xverifier_success);

    // trx_ptr->adjust_target_address(_src_vaddr.get_ledger_subaddr());
    // ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get(), nullptr), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_source_1) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    std::string dst_addr = trx_ptr->target_address().to_string();
    // signature empty
    ASSERT_NE(xtx_verifier::verify_send_tx_source(trx_ptr.get(), false), xverifier_error::xverifier_success);
    trx_ptr->set_authorization("111");
    ASSERT_EQ(xtx_verifier::verify_send_tx_source(trx_ptr.get(), false), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_source_2) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    ASSERT_NE(xtx_verifier::verify_send_tx_source(trx_ptr.get(), true), xverifier_error::xverifier_success);
    trx_ptr->set_same_source_target_address(sys_contract_sharding_vote_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_NE(xtx_verifier::verify_send_tx_source(trx_ptr.get(), true), xverifier_error::xverifier_success);
    trx_ptr->set_same_source_target_address(sys_contract_sharding_vote_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();
    ASSERT_EQ(xtx_verifier::verify_send_tx_source(trx_ptr.get(), true), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_signature_1) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    std::string dst_addr = trx_ptr->target_address().to_string();
    // signature empty
    ASSERT_NE(xtx_verifier::verify_tx_signature(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_authorization("111");
    ASSERT_NE(xtx_verifier::verify_tx_signature(trx_ptr.get()), xverifier_error::xverifier_success);

    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    xecprikey_t pri_key_obj;
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    std::string new_src_addr = pub_key_obj.to_address(addr_type, ledger_id);
    trx_ptr->set_different_source_target_address(new_src_addr, dst_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();

    utl::xecdsasig_t signature_obj = pri_key_obj.sign(trx_ptr->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    trx_ptr->set_authorization(signature);
    ASSERT_EQ(xtx_verifier::verify_tx_signature(trx_ptr.get()), xverifier_error::xverifier_success);
}

TEST_F(test_xverifier, trx_verifier_legitimac_1) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->source_address().to_string();
    std::string dst_addr = trx_ptr->target_address().to_string();
    // signature empty
    ASSERT_NE(xtx_verifier::verify_send_tx_legitimacy(trx_ptr.get()), xverifier_error::xverifier_success);
    trx_ptr->set_authorization("111");
    ASSERT_NE(xtx_verifier::verify_send_tx_legitimacy(trx_ptr.get()), xverifier_error::xverifier_success);

    uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
    uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
    xecprikey_t pri_key_obj;
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    std::string new_src_addr = pub_key_obj.to_address(addr_type, ledger_id);
    trx_ptr->set_different_source_target_address(new_src_addr, dst_addr);
    trx_ptr->set_digest();
    trx_ptr->set_len();

    utl::xecdsasig_t signature_obj = pri_key_obj.sign(trx_ptr->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    trx_ptr->set_authorization(signature);
    ASSERT_EQ(xtx_verifier::verify_send_tx_legitimacy(trx_ptr.get()), xverifier_error::xverifier_success);
}
TEST_F(test_xverifier, address_is_valid) {
    ASSERT_EQ(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T80000968927100f3cb7b23e8d477298311648978d8613"));
    ASSERT_EQ(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T00000LhHKdV8rc9GHkgJUPS39XLCESGnJmJ2Zjg"));
    ASSERT_EQ(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T80000d49b8d3eb074344b153a8caf93f5daa277a6194e"));

    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T80000d49B8D3EB074344b153A8CAF93F5DAA277a6194E"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T800007BF0D244F6D7D17568817C6CFFA4437297E152BF"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T800007bf0D244F6D7D17568817C6CFfa4437297E152BF"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("t00000LRZAf5veEyzcVaYwdVCcYTh2nWdMQtJEnc"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("t800008c1cb12e9467dfc19d32111b2302af4cfd0954d1"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("t80000d49B8D3EB074344b153A8CAF93F5DAA277a6194E"));

    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("a800008c1cb12e9467dfc19d32111b2302af4cfd0954d1"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T00000LRZAf5veEyzcVaYwdVCcYTh2nWdM"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T00000LRZAf5veEyzcVaYwdVCcYTh2nWdMwdVCcYTh2nWdMwdVCcYTh2nWdM"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T80000d49b8d3eb074344b153a8caf93f5daa277a61"));
    ASSERT_NE(top::xverifier::xverifier_success, top::xverifier::xtx_utl::address_is_valid("T80000d49b8d3eb074344b153a8caf93f5daa277a6194edaa277a6194e"));
}

TEST_F(test_xverifier, verify_send_tx_validation_BENCH) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    uint64_t time_begin = base::xtime_utl::time_now_ms();

    uint32_t total_count = 100000;
    for (uint32_t i = 0; i < total_count; i++) {
        xverifier::xtx_verifier::verify_send_tx_validation(trx_ptr.get());
    }

    uint64_t time_finish = base::xtime_utl::time_now_ms();

    // TODO(jimmy) need do optimize total_count=100000 time_ms=6501
    std::cout << "total_count=" << total_count << " time_ms=" << time_finish - time_begin << std::endl;
}
