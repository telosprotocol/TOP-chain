#include "gtest/gtest.h"
#include "xtxexecutor/xunit_service_error.h"

#include "xstore/xstore_face.h"
#include "xconfig/xconfig_face.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xdata/xtransaction_maker.hpp"
#include "xdata/xnative_contract_address.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_errors.h"
#include "xrpc/xuint_format.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include "xverifier/xverifier_utl.h"

using namespace top;
using namespace top::xverifier;
using namespace top::utl;
using namespace top::store;

class test_xverifier_consortium : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}


    std::string  generate_address(xecprikey_t&  priv, const uint8_t addr_type = '0', const uint16_t net_id = 0, const std::string& parent_addr = "") {
        auto pub = priv.get_public_key();
        if (parent_addr == "") {
        return pub.to_address(addr_type, net_id);
        } else {
            return pub.to_address(parent_addr, addr_type, net_id);
        }
    }

    xecdsasig_t  generate_signature(xecprikey_t const& priv, uint256_t const& digest) {
        return priv.sign(digest);
    }


    void transaction_set_signature(data::xtransaction_ptr_t const & trx_ptr, xecdsasig_t sig) {
        trx_ptr->set_authorization(std::string(reinterpret_cast<char*>(sig.get_compact_signature()), sig.get_compact_signature_size()));
    }

    bool  address_is_valid(std::string const& addr) {
        xkeyaddress_t keyaddr{ addr };
        xinfo("address is %s", addr.c_str());
        return keyaddr.is_valid();
    }
};
data::xtransaction_ptr_t make_a_normal_transfer_tx();

TEST_F(test_xverifier_consortium, check_transfer) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->get_source_addr();
    std::cout << "src_addr: " << src_addr <<std::endl;
    std::string dst_addr = trx_ptr->get_target_addr();

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_toggle_register_whitelist_onchain_goverance_parameter::name, 1));

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_error_addr_invalid);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_register_whitelist_onchain_goverance_parameter::name, src_addr));

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_register_whitelist_onchain_goverance_parameter::name, dst_addr + "," + src_addr));

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_register_whitelist_onchain_goverance_parameter::name, dst_addr));

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_toggle_register_whitelist_onchain_goverance_parameter::name, 0));
    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_register_whitelist_onchain_goverance_parameter::name, std::string("")));

}
TEST_F(test_xverifier_consortium, check_register) {
    data::xtransaction_ptr_t trx_ptr = make_a_normal_transfer_tx();
    std::string src_addr = trx_ptr->get_source_addr();
    std::cout << "src_addr: " << src_addr <<std::endl;
    std::string dst_addr = trx_ptr->get_target_addr();

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_toggle_register_whitelist_onchain_goverance_parameter::name, 1));
    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_register_whitelist_onchain_goverance_parameter::name, std::string(src_addr)));

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    trx_ptr->set_different_source_target_address(src_addr, sys_contract_rec_registration_addr);
    trx_ptr->set_tx_type(data::xtransaction_type_run_contract);
    trx_ptr->set_digest();
    trx_ptr->set_len();

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_success);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_register_whitelist_onchain_goverance_parameter::name, std::string("")));

    ASSERT_EQ(xtx_verifier::verify_send_tx_validation(trx_ptr.get()), xverifier_error::xverifier_error_whitelist_limit);

    ASSERT_TRUE(top::config::xconfig_register_t::get_instance().set(top::config::xtop_toggle_register_whitelist_onchain_goverance_parameter::name, 0));

}

