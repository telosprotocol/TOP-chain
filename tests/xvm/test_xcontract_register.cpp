#include <gtest/gtest.h>
#include "xvm/manager/xcontract_register.h"
#include "xbase/xutl.h"
#include "xbase/xint.h"
#include "xcrypto/xckey.h"

using namespace top::contract;
using namespace top::xvm;

class test_contract : public xcontract_base {
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(test_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(test_contract);

    explicit
    test_contract(top::common::xnetwork_id_t const & network_id) : xcontract_base{ network_id } {
    }

    xcontract_base* clone() override {return new test_contract(network_id());}
    virtual void exec(xvm_context* vm_ctx) override {}
};

TEST(xcontract_register_t, test) {
    xcontract_register_t r;
    r.add<test_contract>(top::common::xaccount_address_t{"T-test"}, top::common::xtopchain_network_id);
    ASSERT_TRUE(r.get_contract(top::common::xaccount_address_t{"T-test"}) != nullptr);
    ASSERT_TRUE(r.get_contract(top::common::xaccount_address_t{"T"}) == nullptr);
}

TEST(xcontract_register_t, xregister_node_callback) {
    //init
    std::string node_addr = "T00000LMcqLyTzsk3HB8dhF51i6xEcVEuyX1Vx6p";
    std::string priv_str = top::base::xstring_utl::base64_decode("s6TLMaTYpxvjrEzAw6+qd2WcLcNLxw9xhpGAnxwaxko=");

    top::uint256_t hash_value = top::utl::xsha2_256_t::digest(node_addr);
    top::utl::xecprikey_t ecpriv{(uint8_t *)priv_str.data()};
    top::utl::xecdsasig_t signature = ecpriv.sign(hash_value);

     // check node_sign to verify node
    top::utl::xkeyaddress_t xaddr{node_addr};
    ASSERT_TRUE(xaddr.verify_signature(signature, hash_value));

    // not normal
    priv_str = top::base::xstring_utl::base64_decode("hVsRqvbRSjReJRcXNmBtzGAGQhcFqSeLwtPUW8dGcas=");
    top::utl::xecprikey_t ecpriv_err{(uint8_t *)priv_str.data()};
    signature = ecpriv_err.sign(top::utl::xsha2_256_t::digest("T00000LMcqLyTzsk3HB8dhF51i6xEcVEuyX1Vx6p"));
    ASSERT_FALSE(xaddr.verify_signature(signature, hash_value));
}
