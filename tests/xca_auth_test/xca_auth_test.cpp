#include "gtest/gtest.h"
#include "xca_auth/xca_auth.h"
#include "xbase/xutl.h"
#include <fstream>
#include <gmock/gmock.h>

#define CERT_ROOT_PATH "./rootCA.crt" 
#define CERT_SELF_PATH "./selfCA.crt"
#define CERT_INVALID_PATH "./invalid_self.crt"
#define DER_ROOT_PATH "./rootCA.der"
#define DER_SELF_PATH "./selfCA.der"
#define DER_INVALID_PATH "./invalid_self.der"


using namespace top::ca_auth;

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

// static std::string const ca_level_1_2_content =
//     R"T(-----BEGIN CERTIFICATE-----
// MIICXzCCAcgCCQC4QJ86MQS8yzANBgkqhkiG9w0BAQUFADB5MQswCQYDVQQGEwJD
// TjEVMBMGA1UECAwMdHNldHByb3ZpbmNlMREwDwYDVQQHDAh0ZXN0Y2l0eTEZMBcG
// A1UECgwQdGVzdG9yZ2FuaXphdGlvbjESMBAGA1UECwwJdGVzdGdyb3VwMREwDwYD
// VQQDDAh0ZXN0bmFtZTAeFw0yMjExMDgxMTM5MzhaFw0yMzExMDgxMTM5MzhaMG8x
// CzAJBgNVBAYTAkNOMRMwEQYDVQQIDApteXByb3ZpbmNlMQ8wDQYDVQQHDAZteWNp
// dHkxFzAVBgNVBAoMDm15b3JnYW5pemF0aW9uMRAwDgYDVQQLDAdteWdyb3VwMQ8w
// DQYDVQQDDAZteW5hbWUwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBALerw8Tv
// h0UwU9/B1Xr0DJE/PxTCLZNuxdYscIzDd6bFuVKOPR0D7XSqDTLL6FVsLzLq8zVM
// hjzoZdtGboASZWIA8hOQ6RIU9bgD97Piq6XylNVV5j3yV4/804z7Ue1Q6f/AkutE
// 8PnVYZylX3QpNDx04l73t2giyU9iUYyx1dIBAgMBAAEwDQYJKoZIhvcNAQEFBQAD
// gYEAANVio1xPiCqf1k18Dml75bk9xf1pel8Un7CxrIpgNzgn3RDXKQiK7LsFNVse
// 0xujcNl1ZTxdM2x7AZjqaoQj3fK1vLfV+DMJ9sK36Ti+TXEz0PCf/+CZLyjFPGzH
// U+EkbXyNuF0MA5p4yiJQ1QwMYI0afT3rLT07K6S45Kfu13Y=
// -----END CERTIFICATE-----
// )T";


static std::string const ca_level_2_1_content =
    R"T(-----BEGIN CERTIFICATE-----
MIIDSDCCAjACCQDY2XtsvkDujDANBgkqhkiG9w0BAQsFADBpMQswCQYDVQQGEwJD
TjERMA8GA1UECAwIc2hhbmdoYWkxETAPBgNVBAcMCHNoYW5naGFpMRAwDgYDVQQK
DAdleGFtcGxlMQswCQYDVQQLDAJpdDEVMBMGA1UEAwwMc2Vjb25kYXJ5LWNhMB4X
DTIyMTEwODEyMzUxMVoXDTMyMTEwNTEyMzUxMVowYzELMAkGA1UEBhMCQ04xETAP
BgNVBAgMCHNoYW5naGFpMREwDwYDVQQHDAhzaGFuZ2hhaTEQMA4GA1UECgwHZXhh
bXBsZTELMAkGA1UECwwCaXQxDzANBgNVBAMMBnNlcnZlcjCCASIwDQYJKoZIhvcN
AQEBBQADggEPADCCAQoCggEBAOdWOkF+wwqAtEnUFSC3VjMOiyHIRxnW7mTEkMg7
uLpZ3qpkZYx3tTaBXVl1zsY0Xt5Aoxz5dc6qdrPoSpBnrqviazt8V6hGBYkcwzUq
TNfxu9w6PlHtYEoZcJERQNi88zvDKxRVz7yLXhgSpwF3pYuU+O8cdfiX4HArxJwA
XUx6zFabqJ1GDg73SYl3B0mxtS4r45/PES2CRgbiC9n71IHhsEFKFOmlS4hh/Jmx
k2owTBcAkrsfzPQ2l0PzEi4Bhc5R1U8EZ5SwS2+yyDiDj5t4eSoOVSZ30ThC+uhk
z8f4RUfStS7adUeDx/nYf24YsXsXGQdrYkUPTziGC/RL30sCAwEAATANBgkqhkiG
9w0BAQsFAAOCAQEAZG8qfEP7iDkQIHRJJSWrAFbkIE+O3ggely8eB8EqAULocRX/
W1BrVHqoHKmI4/VCUDQTKDJkwFikpQ1P+0Y7HgSaTOsrPEw/wR4eYDCaSw2WcOKY
rEmjKhAR1zFAPjzoOqph0wJIhuz6RNG7dYUCK5EdXazx0+iVsvb8OQvz3S8mn7bf
IerL2APPUxhs3lZ7RHtnGDfiX18URygCsqqDDxaufMEO4rtApmXb7ZhXvlErIFBV
nsWhn4E/3DOpwXkTmT/0qAQMdIAoMl3qM751tZlgpZ88erumdPI0WC+S6Btsoxf4
UGzdL29mxBiv0yx2OwG1qMNdHFbD0y8BLy98qg==
-----END CERTIFICATE-----
)T";


// static std::string const ca_level_3_1_content =
//     R"T(-----BEGIN CERTIFICATE-----
// MIICVTCCAb4CCQCpXw7vY0lkgjANBgkqhkiG9w0BAQUFADBvMQswCQYDVQQGEwJD
// TjETMBEGA1UECAwKbXlwcm92aW5jZTEPMA0GA1UEBwwGbXljaXR5MRcwFQYDVQQK
// DA5teW9yZ2FuaXphdGlvbjEQMA4GA1UECwwHbXlncm91cDEPMA0GA1UEAwwGbXlu
// YW1lMB4XDTIyMTEwODExNDI0MFoXDTIzMTEwODExNDI0MFowbzELMAkGA1UEBhMC
// Q04xEzARBgNVBAgMCm15cHJvdmluY2UxDzANBgNVBAcMBm15Y2l0eTEXMBUGA1UE
// CgwObXlvcmdhbml6YXRpb24xEDAOBgNVBAsMB215Z3JvdXAxDzANBgNVBAMMBm15
// bmFtZTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAw2T3XDfWZXxr+jMGSnXX
// VDBoV4akpGjQ7+BVgzXmPlTY7RPJRhagdkQrVNc+TDUVQ0Piq9hZZ50DYmZc2njY
// zgmxiKwSWF+irNZrxXPnb6ARTjlseFbzEtK6vV6rYnYogOdxLfXalLvAfz7kmZ5t
// MbwceCpinseg1j3ltbSk+38CAwEAATANBgkqhkiG9w0BAQUFAAOBgQCqGw3PTi3m
// zeMTgIYAaFqtDstVgi8UkdaDO44UM7kXOFNw652d4xxF7dCawO64aXPf4cTWXcFY
// UUl/U62ARoEe8tYLn546Os3+RtkPcjd+koU8eq/8t4LvyndfHvSUFLUy2DTNb6hv
// MQUcj7cFiOt9YqSmG2hD+d9gPl6qfUjBUA==
// -----END CERTIFICATE-----
// )T";

namespace top {
class test_xca_auth : public testing::Test {
protected:
    void SetUp() override
    {

      /*  std::ofstream root_file;
        root_file.open(CERT_ROOT_PATH);
        root_file << ca_cert_conent;
        root_file.close();

        std::ofstream self_file;
        self_file.open(CERT_SELF_PATH);
        self_file << valid_self_cert_conent;
        self_file.close();

        std::ofstream invalid_self_file;
        invalid_self_file.open(CERT_INVALID_PATH);
        invalid_self_file << invalid_other_cert_conent;
        invalid_self_file.close();*/
    }

    void TearDown() override
    {
    }

public:
};


//test root verify level_1
TEST_F(test_xca_auth, ca_root_verify_level_1_test)
{
    xtop_ca_auth ca_auth(CERT_SELF_PATH);
    EXPECT_EQ(1, ca_auth.add_root_cert(ca_root_conent.c_str()));
    EXPECT_EQ(1, ca_auth.init_self_cert(ca_level_1_1_content.c_str(), 1));
    EXPECT_EQ(1, ca_auth.verify_self_cert());
}

//test root verify level_1
// TEST_F(test_xca_auth, ca_root_verify_level_1_2_test)
// {
//     xtop_ca_auth ca_auth(CERT_SELF_PATH);
//     EXPECT_EQ(1, ca_auth.add_root_cert(ca_root_conent.c_str()));
//     EXPECT_EQ(1, ca_auth.init_self_cert(ca_level_1_2_content.c_str(), 1));
//     EXPECT_EQ(1, ca_auth.verify_self_cert());
// }

TEST_F(test_xca_auth, ca_root_verify_level_2_test)
{
    xtop_ca_auth ca_auth(CERT_SELF_PATH);
    EXPECT_EQ(1, ca_auth.add_root_cert(ca_root_conent.c_str()));
    EXPECT_EQ(1, ca_auth.init_self_cert(ca_level_2_1_content.c_str(), 1));
    EXPECT_EQ(1, ca_auth.verify_self_cert());
}

// TEST_F(test_xca_auth, ca_root_verify_level_3_test)
// {
//     xtop_ca_auth ca_auth(CERT_SELF_PATH);
//     EXPECT_EQ(1, ca_auth.add_root_cert(ca_root_conent.c_str()));
//     EXPECT_EQ(1, ca_auth.init_self_cert(ca_level_3_1_content.c_str(), 1));
//     EXPECT_EQ(1, ca_auth.verify_self_cert());
// }



// TEST_F(test_xca_auth, ca_invalid_cert_test)
// {
//     FILE* const file = fopen(CERT_ROOT_PATH, "r");
//     const std::string read_root = ::testing::internal::ReadEntireFile(file);
//     fclose(file);

//     xtop_ca_auth ca_auth(CERT_SELF_PATH);

//     EXPECT_EQ(1, ca_auth.add_root_cert(read_root.c_str()));
//     EXPECT_EQ(1, ca_auth.init_self_cert(1));
//     EXPECT_EQ(1, ca_auth.verify_self_cert());


//     std::string root_file = CERT_ROOT_PATH;
//     std::string invalid_file = CERT_INVALID_PATH;

//     xtop_ca_auth ca_auth(root_file, invalid_file);
//     EXPECT_EQ(1, ca_auth.init_root_cert(1));
//     EXPECT_EQ(1, ca_auth.init_self_cert(1));
//     EXPECT_NE(1, ca_auth.verify_self_cert());
// }

// TEST_F(test_xca_auth, ca_valid_der_test)
// {
//     std::string root_file = DER_ROOT_PATH;
//     std::string self_file = DER_SELF_PATH;

//     xtop_ca_auth ca_auth(root_file, self_file);
//     EXPECT_EQ(1, ca_auth.init_root_cert(2));
//     EXPECT_EQ(1, ca_auth.init_self_cert(2));
//     EXPECT_EQ(1, ca_auth.verify_self_cert());
// }

// TEST_F(test_xca_auth, ca_invalid_der_test)
// {
//     std::string root_file = DER_ROOT_PATH;
//     std::string invalid_file = DER_INVALID_PATH;

//     xtop_ca_auth ca_auth(root_file, invalid_file);
//     EXPECT_EQ(1, ca_auth.init_root_cert(2));
//     EXPECT_EQ(1, ca_auth.init_self_cert(2));
//     EXPECT_NE(1, ca_auth.verify_self_cert());
// }

}
