#include <sstream>
#define private public
#include "xbase/xutl.h"
#include "xbasic/xhex.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrelayblock_build.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xelection/xcache/xdata_accessor.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xcontract_helper.h"
#include "xvm/xsystem_contracts/consortium/xnode_manage_contract.h"
#include "xloader/xconfig_genesis_loader.h"
#include <gtest/gtest.h>

using namespace top;
using namespace top::contract;
using namespace top::xvm;
using namespace top::xvm::consortium;
using top::common::xbroadcast_id_t;
using top::common::xnode_id_t;
using top::common::xnode_type_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_node_info_t;

static std::string const ca_root_conent =
    R"T(-----BEGIN CERTIFICATE-----
MIIDmzCCAoOgAwIBAgIJAM3iW7K+W3CVMA0GCSqGSIb3DQEBCwUAMGQxCzAJBgNV
BAYTAkNOMREwDwYDVQQIDAhzaGFuZ2hhaTERMA8GA1UEBwwIc2hhbmdoYWkxEDAO
BgNVBAoMB2V4YW1wbGUxCzAJBgNVBAsMAml0MRAwDgYDVQQDDAdyb290LWNhMB4X
DTIyMTEwODEyMzAwNloXDTMyMTEwNTEyMzAwNlowZDELMAkGA1UEBhMCQ04xETAP
BgNVBAgMCHNoYW5naGFpMREwDwYDVQQHDAhzaGFuZ2hhaTEQMA4GA1UECgwHZXhh
bXBsZTELMAkGA1UECwwCaXQxEDAOBgNVBAMMB3Jvb3QtY2EwggEiMA0GCSqGSIb3
DQEBAQUAA4IBDwAwggEKAoIBAQC8pej34AyEtRyWqjzC6IhzjISlONTl1gCfb3xB
Cpct+bnycdTpJy24pKk5X1/TV5Dn9d2oSB/1G1dAQRQt2TsJcK8ZUwLhVmCMuWFg
cXikt4lqKuOObQucoUCz/AQgceKnADqh4E5OB2RPXZa5Aea1sl8jRAvY3iGYH+b8
bfu0PNv+E/sI7dsG8Y54akZ89HrldtnjLoJGa5+JgBSElE6mUpQYdI0cxNRZh/Ro
D9JuKMqXJbbAX72dxQwVtLriZ8iWi0wWwePa3LHPeqXwvMnE/H8EVuAHIAHc0MCj
Fby6n5WmlA4q5+t0I++UgHsDkmnocrGEou8P8kJgLmvM2WSBAgMBAAGjUDBOMB0G
A1UdDgQWBBSVR6WBPyFyQr4Mmbz663b7UHEPqDAfBgNVHSMEGDAWgBSVR6WBPyFy
Qr4Mmbz663b7UHEPqDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCj
Pd40HykRHivlIK4aKsdKisB7PsHMORInpwYtBL/6eh978lUp3xePlwHK5VUgsbBt
Pa11++nmnscBUiWSycO3MCwZjnyvZNsFgxZTKpsjzk6Tqh6DvzuGwSozC5ukv+DQ
+uOswyxPqgmf0qjVK9tX52W5sASoN3ekXCErQmiHQ5+1I6n9QOpRF8VR4wlnNFgx
eWEtDXYJ89ptC0pnhouFPYzpYBPkUQPGqFtB/RNEJX+QpLoF6bMftTox2xOq8vvx
nnbX5/JkpFTFIEXMPCRBjEy/ZuuoHyltruduB4RGvf5vWx1GS53iG8g5bbYJF7WM
RK2u3uM3gn7ba/6kb1N+
-----END CERTIFICATE-----
)T";

static std::string const ca_level_1_1_content =
    R"T(-----BEGIN CERTIFICATE-----
MIIDSTCCAjECCQDPsaqNxGKfBTANBgkqhkiG9w0BAQsFADBkMQswCQYDVQQGEwJD
TjERMA8GA1UECAwIc2hhbmdoYWkxETAPBgNVBAcMCHNoYW5naGFpMRAwDgYDVQQK
DAdleGFtcGxlMQswCQYDVQQLDAJpdDEQMA4GA1UEAwwHcm9vdC1jYTAeFw0yMjEx
MDgxMjM0NDhaFw0zMjExMDUxMjM0NDhaMGkxCzAJBgNVBAYTAkNOMREwDwYDVQQI
DAhzaGFuZ2hhaTERMA8GA1UEBwwIc2hhbmdoYWkxEDAOBgNVBAoMB2V4YW1wbGUx
CzAJBgNVBAsMAml0MRUwEwYDVQQDDAxzZWNvbmRhcnktY2EwggEiMA0GCSqGSIb3
DQEBAQUAA4IBDwAwggEKAoIBAQDJeM3M/k2Ip4MoAlZzLywyLFNTRxS0tG7jHkkO
ccuqSklHc5xKv3ZuqluvtklvIyLe63EjF45G4ecz0xzUi+ETManDX5WEIdP8O2OL
myphclhmikZIP9vUjqfPYtXL7uIgTc5mxCChvSubbNHWkmfBHad/cOeiOs0KFnZb
W97RrsonUyrTjtZ1+1zRBxLeh7BYbkmIbCWONf5BF2pE7kwRZsxn+3sJGICmNTZ3
Nd+dk5bQjK9HwMYd04bL/GoTLkh0h5OmCTUrZEu0ja+76kzFz5pkn/jQpP6+pZoA
Q3jN0j+fy6kQxRNg6uXQaAG8TvGRzoshUUhmeMv/ToaQghttAgMBAAEwDQYJKoZI
hvcNAQELBQADggEBAEO+O2n6oyJfU5BoGzZdPcciWKxdSZkrdtIWk4JW5rn0VToU
wgRUqNp1TdVsadY4WtlgQxPy5Yo6IdLYO4IGU4E0xpyZ2Y8JVOGFFiGgZLkOnd6k
YwjUv0bOooGsw8y2CO/zNu8vVDIzwpxTYa221j+9HsAvzAEG5Do8CsQSErtwIvm5
eFcImEynYFGKKFXtjZDYEKCOQqzFgjuGyfJ+z0h2VEBe01Q32dkxTUXWlEI2UBDQ
r3v88MV4vnCtqP9+ygHsZS9xKEy7Y5VC9w0ZuQuJdXtQBs3BT88rukETWmrolGHa
Li35ITDGmJnBqBozgzhKXg8znwlo8BMOBd6Wrts=
-----END CERTIFICATE-----
)T";

static std::string const ca_level_3_1_content =
    R"T(-----BEGIN CERTIFICATE-----
MIICVTCCAb4CCQCpXw7vY0lkgjANBgkqhkiG9w0BAQUFADBvMQswCQYDVQQGEwJD
TjETMBEGA1UECAwKbXlwcm92aW5jZTEPMA0GA1UEBwwGbXljaXR5MRcwFQYDVQQK
DA5teW9yZ2FuaXphdGlvbjEQMA4GA1UECwwHbXlncm91cDEPMA0GA1UEAwwGbXlu
YW1lMB4XDTIyMTEwODExNDI0MFoXDTIzMTEwODExNDI0MFowbzELMAkGA1UEBhMC
Q04xEzARBgNVBAgMCm15cHJvdmluY2UxDzANBgNVBAcMBm15Y2l0eTEXMBUGA1UE
CgwObXlvcmdhbml6YXRpb24xEDAOBgNVBAsMB215Z3JvdXAxDzANBgNVBAMMBm15
bmFtZTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAw2T3XDfWZXxr+jMGSnXX
VDBoV4akpGjQ7+BVgzXmPlTY7RPJRhagdkQrVNc+TDUVQ0Piq9hZZ50DYmZc2njY
zgmxiKwSWF+irNZrxXPnb6ARTjlseFbzEtK6vV6rYnYogOdxLfXalLvAfz7kmZ5t
MbwceCpinseg1j3ltbSk+38CAwEAATANBgkqhkiG9w0BAQUFAAOBgQCqGw3PTi3m
zeMTgIYAaFqtDstVgi8UkdaDO44UM7kXOFNw652d4xxF7dCawO64aXPf4cTWXcFY
UUl/U62ARoEe8tYLn546Os3+RtkPcjd+koU8eq/8t4LvyndfHvSUFLUy2DTNb6hv
MQUcj7cFiOt9YqSmG2hD+d9gPl6qfUjBUA==
-----END CERTIFICATE-----
)T";

static std::string const child_ca = "-----BEGIN CERTIFICATE-----\r\nMIIDcjCCAloCCQDDRN21sPa+ZjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg1y\r\nb290QHRlc3QuY29tMCAXDTIyMTIwNjA4MDYyMloYDzIwNzkwMTIxMDgwNjIyWjB8\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEc\r\nMBoGCSqGSIb3DQEJARYNb0RtWEB0ZXN0LmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD\r\nggEPADCCAQoCggEBAMutYCqExn7W6kAXUoeJYsvwHVlTeVxyok1dLpIdMTx0K9XD\r\n0FEBPy/Dr/rLqCXqnvtZ7yFl7VKfDLlz3Rb4x9kNb3p4XMcftyzAJSOXIKnmTTUr\r\n0ZQ680fGNsU0vrXgj+xw18Mw0MlTfb3zQVbxx+9Cil4CAtA+8fHhLKoz39ucWCGO\r\nk32SXpTkjGbIpgbrKkMF5wnw1FYF1bBf8e8NYtxoA3TeXHvAyuo9QUbkBr0UssdH\r\nVj2mZampjx/cLQE9Lcuhui/4CnVNK3yRtywmq3iLN2Xsg1eTY3u4MErhwNHeW9qQ\r\nM71qWPZmWQGyHGVLjOD2xAfCuFMjRz4/ywDiB70CAwEAATANBgkqhkiG9w0BAQsF\r\nAAOCAQEAE5MVpiqQ23lmuhZ4EeiLRX8LGTmk2YurfGrxpisFaqEerOFDrQjEU24s\r\n41ifHITFTiFTNX6hoMf8jwaJBvLEFeQYxF3OBSiTFVQYyoeDHjgBHZrf/CoW3kD7\r\nYVe8KsEkhVjK/oRtSJFIKdMvmCvLCsh8wlW1z8ySX5nuN3GWfHUQRb25eFwKQfye\r\naojkhQTUAlSZMCnoZWVKKwHLVN3y3yMY1mf9coxgByrEXv5kpFunuPruw1yUIq3w\r\nFikL6NgjPaSWPEyoVAV6UjjRTnI5HFze6jCPcW/nherwHEZ2Gr/CtpO4zRXuzNGd\r\ng7gHgQkjklC/AdTsXsEATJLN7WFUvA==\r\n-----END CERTIFICATE-----";

class xtop_test_xnode_manage_contract
    : public xnode_manage_contract,
      public testing::Test {

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_xnode_manage_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_xnode_manage_contract);

    xtop_test_xnode_manage_contract()
        : xnode_manage_contract(common::xnetwork_id_t { 0 }) {};

    xcontract_base* clone() override
    {
        return {};
    }

    void exec(top::xvm::xvm_context* vm_ctx)
    {
        return;
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
    std::string node_check { "check_all" };
    std::string node_ca_check { "check_ca" };
    std::string node_expiry_check { "check_expiry_time" };
};

#define PREPAIR                                                                                                                   \
    std::string root_account = top::data::xrootblock_t::get_extend_data_by_key(EXTEND_ROOT_ACCOUNT_KEY);                          \
    std::string root_ca = top::data::xrootblock_t::get_extend_data_by_key(EXTEND_ROOT_ACCOUNT_CA_KEY);                            \
    auto exe_addr = std::string { root_account };                                                                                 \
    auto contract_addr = common::xnode_id_t { sys_contract_rec_node_manage_addr };                                                \
    auto vbstate = make_object_ptr<xvbstate_t>(sys_contract_rec_node_manage_addr, 1, 1, std::string {}, std::string {}, 0, 0, 0); \
    auto unitstate = std::make_shared<xunit_bstate_t>(vbstate.get());                                                             \
    auto account_context = std::make_shared<xaccount_context_t>(unitstate);                                                       \
    auto contract_helper = std::make_shared<xcontract_helper>(account_context.get(), contract_addr, exe_addr);                    \
    set_contract_helper(contract_helper);                                                                                         \
    setup();

// checkt check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoAuthConfig)
{
    PREPAIR
    // test node_check
    std::vector<string> check_vec;
    check_vec.push_back(node_check);
    check_vec.push_back(node_ca_check);
    check_vec.push_back(node_expiry_check);

    std::string node_check_flag;
    for (auto check_type : check_vec) {
        // normal
        nodeInfoAuthConfig(check_type, "0");
        node_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, check_type);
        EXPECT_EQ(node_check_flag, "0");

        nodeInfoAuthConfig(check_type, "1");
        node_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, check_type);
        EXPECT_EQ(node_check_flag, "1");
    }
}

// register without all check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoReg_with_all_check)
{
    PREPAIR

    std::string account_str = "T80000a153f08dbd09b496b10c11b23100a87d714a1c13";

    nodeInfoAuthConfig(node_check, "1");
    nodeInfoAuthConfig(node_ca_check, "1");
    nodeInfoAuthConfig(node_expiry_check, "1");

    nodeInfoReg(account_str, 10000, child_ca);

    std::string out_str;
    EXPECT_EQ(nodeInfoExist(account_str, out_str), true);
    data::system_contract::xnode_manage_account_info_t account_reg_info;
    base::xstream_t _stream { base::xcontext_t::instance(), (uint8_t*)out_str.data(), static_cast<uint32_t>(out_str.size()) };
    account_reg_info.serialize_from(_stream);

    EXPECT_EQ(nodeInfoExpiryTimeCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCaCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), true);

    // unreg
    nodeInfoUnreg(account_str);
    EXPECT_EQ(nodeInfoExist(account_str, out_str), false);
}

// register without all check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoReg_without_all_check)
{
    PREPAIR

    std::string account_str = "T80000a153f08dbd09b496b10c11b23100a87d714a1c13";

    nodeInfoAuthConfig(node_check, "0");
    nodeInfoAuthConfig(node_ca_check, "0");
    nodeInfoAuthConfig(node_expiry_check, "0");

    nodeInfoReg(account_str, 0, "");

    std::string out_str;
    EXPECT_EQ(nodeInfoExist(account_str, out_str), true);
    data::system_contract::xnode_manage_account_info_t account_reg_info;
    base::xstream_t _stream { base::xcontext_t::instance(), (uint8_t*)out_str.data(), static_cast<uint32_t>(out_str.size()) };
    account_reg_info.serialize_from(_stream);

    EXPECT_EQ(nodeInfoExpiryTimeCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCaCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), true);

    nodeInfoAuthConfig(node_check, "1");
    nodeInfoAuthConfig(node_ca_check, "1");
    nodeInfoAuthConfig(node_expiry_check, "1");
    EXPECT_EQ(nodeInfoExpiryTimeCheck(account_reg_info), false);
    EXPECT_EQ(nodeInfoValidateCaCheck(account_reg_info), false);
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), false);

    // unreg
    nodeInfoUnreg(account_str);
    EXPECT_EQ(nodeInfoExist(account_str, out_str), false);
}

// register without all check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoReg_with_ca_check)
{
    PREPAIR

    std::string account_str = "T80000a153f08dbd09b496b10c11b23100a87d714a1c13";

    nodeInfoAuthConfig(node_check, "1");
    nodeInfoAuthConfig(node_ca_check, "1");
    nodeInfoAuthConfig(node_expiry_check, "0");

    nodeInfoReg(account_str, 0, child_ca);

    std::string out_str;
    EXPECT_EQ(nodeInfoExist(account_str, out_str), true);
    data::system_contract::xnode_manage_account_info_t account_reg_info;
    base::xstream_t _stream { base::xcontext_t::instance(), (uint8_t*)out_str.data(), static_cast<uint32_t>(out_str.size()) };
    account_reg_info.serialize_from(_stream);

    EXPECT_EQ(nodeInfoExpiryTimeCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCaCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), true);

    nodeInfoAuthConfig(node_expiry_check, "1");
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), false);

    // unreg
    nodeInfoUnreg(account_str);
    EXPECT_EQ(nodeInfoExist(account_str, out_str), false);
}

// register without all check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoReg_with_time_check)
{
    PREPAIR

    std::string account_str = "T80000a153f08dbd09b496b10c11b23100a87d714a1c13";

    nodeInfoAuthConfig(node_check, "1");
    nodeInfoAuthConfig(node_ca_check, "0");
    nodeInfoAuthConfig(node_expiry_check, "1");

    nodeInfoReg(account_str, 1000, "");

    std::string out_str;
    EXPECT_EQ(nodeInfoExist(account_str, out_str), true);
    data::system_contract::xnode_manage_account_info_t account_reg_info;
    base::xstream_t _stream { base::xcontext_t::instance(), (uint8_t*)out_str.data(), static_cast<uint32_t>(out_str.size()) };
    account_reg_info.serialize_from(_stream);

    EXPECT_EQ(nodeInfoExpiryTimeCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCaCheck(account_reg_info), true);
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), true);

    nodeInfoAuthConfig(node_ca_check, "1");
    EXPECT_EQ(nodeInfoValidateCheck(account_reg_info), false);

    // unreg
    nodeInfoUnreg(account_str);
    EXPECT_EQ(nodeInfoExist(account_str, out_str), false);
}

// register without all check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoReg_ca_replace)
{
    PREPAIR

    std::string new_root_account_str = "T00000a153f08dbd09b496b10c11b23100a87d714azbcd";
    std::string account_str = "T80000a153f08dbd09b496b10c11b23100a87d714a1c13";

    // get old
    std::string root_account_old = STRING_GET(data::system_contract::XPROPERTY_NODE_ROOT_ACCOUNT_KEY);
    std::string root_ca_old = STRING_GET(data::system_contract::XPROPERTY_NODE_ROOT_CA_KEY);
    EXPECT_EQ(root_account, root_account_old);
    EXPECT_EQ(root_ca, root_ca_old);

    nodeInfoRootCaReplace(new_root_account_str, ca_root_conent);
    // get new
    root_account = STRING_GET(data::system_contract::XPROPERTY_NODE_ROOT_ACCOUNT_KEY);
    root_ca = STRING_GET(data::system_contract::XPROPERTY_NODE_ROOT_CA_KEY);

    EXPECT_EQ(root_account, new_root_account_str);
    EXPECT_EQ(root_ca, ca_root_conent);
    EXPECT_NE(root_account, root_account_old);
    EXPECT_NE(root_ca, root_ca_old);
}

// register without all check
TEST_F(xtop_test_xnode_manage_contract, test_nodeInfoReg_carebase_replace)
{
    PREPAIR

    std::string account_str_1 = "T00000a153f08dbd09b496b10c11b23100a87d714azbcd";
    std::string account_str_2 = "T80000a153f08dbd09b496b10c11b23100a87d714acccc";
    std::string account_str_3 = "T80000a153f08dbd09b496b10c11b23100a87d714adddd";

    nodeInfoAuthConfig(node_check, "0");
    nodeInfoAuthConfig(node_ca_check, "0");
    nodeInfoAuthConfig(node_expiry_check, "0");

    nodeInfoReg(account_str_1, 1000, "");
    nodeInfoReg(account_str_2, 1000, child_ca);
    nodeInfoReg(account_str_3, 1000, ca_level_1_1_content);

    std::string out_str;
    EXPECT_EQ(nodeInfoExist(account_str_1, out_str), true);
    EXPECT_EQ(nodeInfoExist(account_str_2, out_str), true);
    EXPECT_EQ(nodeInfoExist(account_str_3, out_str), true);

    nodeInfoAuthConfig(node_check, "1");
    nodeInfoAuthConfig(node_ca_check, "1");
    nodeInfoAuthConfig(node_expiry_check, "1");

    nodeInfoCaRebase();

    EXPECT_EQ(nodeInfoExist(account_str_1, out_str), false);
    EXPECT_EQ(nodeInfoExist(account_str_2, out_str), true);
    EXPECT_EQ(nodeInfoExist(account_str_3, out_str), false);
}
