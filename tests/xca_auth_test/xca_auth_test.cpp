#include "gtest/gtest.h"
#include "xca_auth/xca_auth.h"
#include "xbase/xutl.h"
#include <fstream>

#define CERT_ROOT_PATH "./rootCA.crt"
#define CERT_SELF_PATH "./selfCA.crt"
#define CERT_INVALID_PATH "./invalid_self.crt"

using namespace top::ca_auth;

static auto ca_cert_conent =
    R"T(-----BEGIN CERTIFICATE-----
MIICxTCCAmugAwIBAgIDBTe2MAoGCCqGSM49BAMCMIGaMQswCQYDVQQGEwJDTjER
MA8GA1UECBMIemhlamlhbmcxETAPBgNVBAcTCGhhbmd6aG91MSYwJAYDVQQKDB10
b3B0b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzESMBAGA1UECxMJcm9vdC1jZXJ0
MSkwJwYDVQQDDCBjYS50b3B0b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzAeFw0y
MjExMDEwNjEwMTRaFw0zMjEwMjkwNjEwMTRaMIGaMQswCQYDVQQGEwJDTjERMA8G
A1UECBMIemhlamlhbmcxETAPBgNVBAcTCGhhbmd6aG91MSYwJAYDVQQKDB10b3B0
b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzESMBAGA1UECxMJcm9vdC1jZXJ0MSkw
JwYDVQQDDCBjYS50b3B0b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzBZMBMGByqG
SM49AgEGCCqGSM49AwEHA0IABEGc5+XIEv//eLALUGkWfba+UHC92KdqmVa+DPCH
e/fDBYkIZtjd/ee8Kdf6up0Lp4QLdeCH7CrZRmLYJi5gx6yjgZ0wgZowDgYDVR0P
AQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wKQYDVR0OBCIEIPCc3EI3U1n36Cx6
8InFAfiwfDz0IfVPokPNSW3IR0qMMEwGA1UdEQRFMEOCDmNoYWlubWFrZXIub3Jn
gglsb2NhbGhvc3SCIGNhLnRvcHRvcC1vcmcudG9wX2NvbnNvcml0dW0ub3JnhwR/
AAABMAoGCCqGSM49BAMCA0gAMEUCIQCEWGZrQKhyO7W+fm/IffqjuBT5yFZP2Ma9
gDBNaAuIegIgKEZvKegOvYATtyzZc4yqdC09MveKBIbXxlsHO96PCck=
-----END CERTIFICATE-----
)T";

static auto valid_self_cert_conent =
    R"T(-----BEGIN CERTIFICATE-----
MIIDGDCCAr6gAwIBAgIDDKq+MAoGCCqGSM49BAMCMIGaMQswCQYDVQQGEwJDTjER
MA8GA1UECBMIemhlamlhbmcxETAPBgNVBAcTCGhhbmd6aG91MSYwJAYDVQQKDB10
b3B0b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzESMBAGA1UECxMJcm9vdC1jZXJ0
MSkwJwYDVQQDDCBjYS50b3B0b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzAeFw0y
MjExMDEwNjEwMTRaFw0yNzEwMzEwNjEwMTRaMIGmMQswCQYDVQQGEwJDTjERMA8G
A1UECBMIemhlamlhbmcxETAPBgNVBAcTCGhhbmd6aG91MSYwJAYDVQQKDB10b3B0
b3Atb3JnLnRvcF9jb25zb3JpdHVtLm9yZzESMBAGA1UECxMJY29uc2Vuc3VzMTUw
MwYDVQQDDCxjb25zZW5zdXMxLnRscy50b3B0b3Atb3JnLnRvcF9jb25zb3JpdHVt
Lm9yZzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABDGkPy/VPljjfO2+ANTLYIPQ
x+iG+Vv59sfL6fFaKkiyNzxeSYpS8Fnaw0mhkkNF+QDol7+oyaIU0kSs1QLn6nKj
geQwgeEwDgYDVR0PAQH/BAQDAgP4MB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEF
BQcDAjApBgNVHQ4EIgQghBcCBF2123fv44wQtP7C5G9m6gJ8D+2VCvzbFFH72mww
KwYDVR0jBCQwIoAg8JzcQjdTWffoLHrwicUB+LB8PPQh9U+iQ81JbchHSowwWAYD
VR0RBFEwT4IOY2hhaW5tYWtlci5vcmeCCWxvY2FsaG9zdIIsY29uc2Vuc3VzMS50
bHMudG9wdG9wLW9yZy50b3BfY29uc29yaXR1bS5vcmeHBH8AAAEwCgYIKoZIzj0E
AwIDSAAwRQIhANs6CLWUDeSPG+LkmGXjm0+TdU/ErenZl8irebUX/EmVAiB7lIE4
Y28VF0i2cpjedNGZ27fm1l4/LDk0mLKd/4f2pg==
-----END CERTIFICATE-----
)T";

static auto invalid_other_cert_conent =
    R"T(-----BEGIN CERTIFICATE-----
MIIC8TCCAdmgAwIBAgIJAJb1XCIJaw3uMA0GCSqGSIb3DQEBCwUAMA8xDTALBgNV
BAMMBHRlc3QwHhcNMjIwODMxMDkxMTA3WhcNMjIwOTMwMDkxMTA3WjAPMQ0wCwYD
VQQDDAR0ZXN0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA5CWhEU04
IQ7VKouiQD0vPBxFE7jeqWXhx410Vwq9zQVh+PqcYqy7/8QRZExdF1HPqZh/FPum
a7hVh1hUClVW6dApvyaLnPgI3b8oY0ghVPFAuTVEVSzRflY9kLUxJpFk6YnuKLMs
kpkeCGGWeRBYsm9DXW+o1PiIgTK5+2HHjHKSqJP0kDcs6WxhmxF1qfl0tjQL1TVX
7zWe+qSlPfQQN6nDy+g+WAyGEISZGvy+HVTecSAQeN4C14yNdw+hEz43I6nCobqP
DzYv3KMcYeATdlqaX4wksZyAKWL9lAMIyEcYxDxk5qG+1aeZREXvqjQOFB6keMf4
9OYN0UxPui/2QQIDAQABo1AwTjAdBgNVHQ4EFgQUM5EHgVHsah5px2f3IyW5SGsE
OtcwHwYDVR0jBBgwFoAUM5EHgVHsah5px2f3IyW5SGsEOtcwDAYDVR0TBAUwAwEB
/zANBgkqhkiG9w0BAQsFAAOCAQEALmxp/6NMreqZjvwIrnQ+jheWRDX1YPPKwf9c
Df4BqU0/K1fOEVi4VlpfJKMvqv/QcaD2SCzOM6YNXi2panQZCrT9bQtKxwi5ahgG
W9XBi8e8WKDuKT6UrjAH35giDeskiicMsgumNJ//RJyDJR7pLbqemWOuAOh7VRVS
BlDGjlan0hyYssr5N5fvtrrjOnrhAKGuvG5O1noJ2AUr92q4Ckje802+da8t7jnX
ymZIkd8FHBBPIxeIvVMUXRHYXAIz0WPmP0XZMoTBYjmXumkaiI7Ug5BtxqKfjuK+
mGPnExdRtJSTNWgeqIlJBkEyFHwdjtYwoLLIITitwY+Px2jpLA==
-----END CERTIFICATE-----
)T";

namespace top {
class test_xca_auth : public testing::Test {
protected:
    void SetUp() override
    {

        std::ofstream root_file;
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
        invalid_self_file.close();
    }

    void TearDown() override
    {
    }

public:
};

TEST_F(test_xca_auth, ca_valid_cert_test)
{

    std::string root_file = CERT_ROOT_PATH;
    std::string self_file = CERT_SELF_PATH;

    xtop_ca_auth ca_auth(root_file, self_file);

    EXPECT_EQ(1, ca_auth.init_root_cert());
    EXPECT_EQ(1, ca_auth.init_self_cert());
    EXPECT_EQ(1, ca_auth.verify_self_cert());
}

TEST_F(test_xca_auth, ca_invalid_cert_test)
{

    std::string root_file = CERT_ROOT_PATH;
    std::string invalid_file = CERT_INVALID_PATH;

    xtop_ca_auth ca_auth(root_file, invalid_file);

    EXPECT_EQ(1, ca_auth.init_root_cert());
    EXPECT_EQ(1, ca_auth.init_self_cert());
    EXPECT_NE(1, ca_auth.verify_self_cert());
}

}