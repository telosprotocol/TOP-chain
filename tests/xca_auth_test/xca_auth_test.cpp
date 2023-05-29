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
#include "json/json.h"
#include "json/value.h"

using namespace top::ca_auth;

static std::string const g_ci_genesis_config =
    R"T(
{
    "ca_root_conent":"-----BEGIN CERTIFICATE-----\r\nMIIDbjCCAlYCCQCSWmaugxw7QjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg10\r\nZXN0QHRlc3QuY29tMCAXDTIyMTIwMjA1NDgyMVoYDzMwMjIwNDA0MDU0ODIxWjB4\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJ\r\nKoZIhvcNAQkBFg10ZXN0QHRlc3QuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\r\nMIIBCgKCAQEAvgco7RYHbRXS+2hT/xTfZXlM3z+uv7gcdX0aUv1igDLKeGWBVShf\r\n3yVBdX1PHeRPi+LHCt3IckZ5LY5LxbY4vk+O17X/nEjv2vIH+CroHVsQknziUpDM\r\ng7zlfyorBcJVdNveyogSIjcuLYAgr72tlse0SsoAq7AO4aF8Z7kk5tjc5Ilya2eE\r\n1uhlkszF96Bm4FxDLoD0G7v1wnju8FITE6z60mAzyCgPIHXMGGNkB135FBsKUYyv\r\nPp/KtmxptM5lLj7y2Foy20o5nkIE2oUFMJ6FRg/rsw884O9FZwMZsPa9/VjK0dAd\r\nmQcEnfj/HYSkaHy1f9pCbWtV7E7EkIel3wIDAQABMA0GCSqGSIb3DQEBCwUAA4IB\r\nAQC8sdD/HU2y35HeHM94r97sbtM5Ekz3D7Nq0Lj2VkMj0Sn7BoULbiuWQq5Z7MpZ\r\nhmYCUVe2poVjXsw0eWphWi0J1d79eLrRX52VXFLLjsz/zq/yWzzt9zpHZoOGOanM\r\nsEB6hp5oIxUUHUpRyUaEJJjxAEwmhiNnGc6sFU+VVtPA7fnZNSaiBvCuweU/6Jn1\r\nGPq6IgnnVY1WQel24eThb/4SOvGiJVN3pEUKpjUntqdWz6/YJK32FVZj2sRCnLnM\r\nbChKn52K7dztp48n4w90TUQxnmMQKIYaUC3+p1pbiWFdbnZubjPinNoHK0ar4PdD\r\ndrlV0SLeyuOoazkgeR86kYw3\r\n-----END CERTIFICATE-----",
    "ca_right_content":"-----BEGIN CERTIFICATE-----\r\nMIIDczCCAlsCCQCCo7H9ahGnyjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg10\r\nZXN0QHRlc3QuY29tMCAXDTIyMTIwMjA1NTUyNVoYDzIwNzkwMTE3MDU1NTI1WjB9\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEd\r\nMBsGCSqGSIb3DQEJARYOY2hpbGRAdGVzdC5jb20wggEiMA0GCSqGSIb3DQEBAQUA\r\nA4IBDwAwggEKAoIBAQDUgu6dIFP53E4Qedj1nXJEddIpkgvgFyedJTaw56YKmbSC\r\nVzioL9qoptQoyMV8gscYeEUj11oCb9BHg60c5ZHm3roHUU3lii0B/fyapGFKTDGf\r\nyiBRtPjvSVPgI0WQJbdPQ+cT3shLUamUTcrj1B0cQJLzS9KCdSae9XPehQqOoeAR\r\nVc1Fs3YZzrI2WCV4/j+s+TQG3MyLXyuK0Bf45ldvQ4w91j9jJyBdJ8P+dyf/NfZG\r\nBvzUkzl1oE0yOn+CyayYIitxG40qW3eoQYk2iTN8maGQCFuBdbkSBwOonFukEv+/\r\nL93R3M5Ze6Qb+ut2qwzQ8iZafvurzN0xpHSKrsmvAgMBAAEwDQYJKoZIhvcNAQEL\r\nBQADggEBAAOPq/Aq0LbpsYQqz6qHPp7reMbkkDoOYIZ/Bkxgki6lij17Nmr1GBv8\r\nJIjOcD+5LpVOiKnNxMLKPO+wYIyZ4XhoXQLKqzWIWC8LiZwhu19LWF7FbVei902f\r\njj7XGOYy0pvIJ0CMCUY+lSGsJy+gSd4lzTCIjn1tOrHwV9J5rtN2x3YKXfd0KkGB\r\nn72BSvmo+KXnItCp+GvqLceeMN8siilOcVPnUuw2BMJL883X4O/1YxaA78PVGVTl\r\n/XoLwLvgXev/eKjBxU0g6tcES6EYQZyitLTjlj8GwUZwSl0GC1tBMKBREIDCmmus\r\nPZK9awkEt567lRnzygG42LRO4t5Zr6w=\r\n-----END CERTIFICATE-----"
}
)T";

static std::string const ca_root_conent =
R"T(-----BEGIN CERTIFICATE-----
MIIDbjCCAlYCCQCSWmaugxw7QjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD
TjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD
VQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg10
ZXN0QHRlc3QuY29tMCAXDTIyMTIwMjA1NDgyMVoYDzMwMjIwNDA0MDU0ODIxWjB4
MQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK
DARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJ
KoZIhvcNAQkBFg10ZXN0QHRlc3QuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A
MIIBCgKCAQEAvgco7RYHbRXS+2hT/xTfZXlM3z+uv7gcdX0aUv1igDLKeGWBVShf
3yVBdX1PHeRPi+LHCt3IckZ5LY5LxbY4vk+O17X/nEjv2vIH+CroHVsQknziUpDM
g7zlfyorBcJVdNveyogSIjcuLYAgr72tlse0SsoAq7AO4aF8Z7kk5tjc5Ilya2eE
1uhlkszF96Bm4FxDLoD0G7v1wnju8FITE6z60mAzyCgPIHXMGGNkB135FBsKUYyv
Pp/KtmxptM5lLj7y2Foy20o5nkIE2oUFMJ6FRg/rsw884O9FZwMZsPa9/VjK0dAd
mQcEnfj/HYSkaHy1f9pCbWtV7E7EkIel3wIDAQABMA0GCSqGSIb3DQEBCwUAA4IB
AQC8sdD/HU2y35HeHM94r97sbtM5Ekz3D7Nq0Lj2VkMj0Sn7BoULbiuWQq5Z7MpZ
hmYCUVe2poVjXsw0eWphWi0J1d79eLrRX52VXFLLjsz/zq/yWzzt9zpHZoOGOanM
sEB6hp5oIxUUHUpRyUaEJJjxAEwmhiNnGc6sFU+VVtPA7fnZNSaiBvCuweU/6Jn1
GPq6IgnnVY1WQel24eThb/4SOvGiJVN3pEUKpjUntqdWz6/YJK32FVZj2sRCnLnM
bChKn52K7dztp48n4w90TUQxnmMQKIYaUC3+p1pbiWFdbnZubjPinNoHK0ar4PdD
drlV0SLeyuOoazkgeR86kYw3
-----END CERTIFICATE-----
)T";

static std::string const ca_right_content = "-----BEGIN CERTIFICATE-----\r\nMIIDczCCAlsCCQCCo7H9ahGnyjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD\r\nTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD\r\nVQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg10\r\nZXN0QHRlc3QuY29tMCAXDTIyMTIwMjA1NTUyNVoYDzIwNzkwMTE3MDU1NTI1WjB9\r\nMQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK\r\nDARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEd\r\nMBsGCSqGSIb3DQEJARYOY2hpbGRAdGVzdC5jb20wggEiMA0GCSqGSIb3DQEBAQUA\r\nA4IBDwAwggEKAoIBAQDUgu6dIFP53E4Qedj1nXJEddIpkgvgFyedJTaw56YKmbSC\r\nVzioL9qoptQoyMV8gscYeEUj11oCb9BHg60c5ZHm3roHUU3lii0B/fyapGFKTDGf\r\nyiBRtPjvSVPgI0WQJbdPQ+cT3shLUamUTcrj1B0cQJLzS9KCdSae9XPehQqOoeAR\r\nVc1Fs3YZzrI2WCV4/j+s+TQG3MyLXyuK0Bf45ldvQ4w91j9jJyBdJ8P+dyf/NfZG\r\nBvzUkzl1oE0yOn+CyayYIitxG40qW3eoQYk2iTN8maGQCFuBdbkSBwOonFukEv+/\r\nL93R3M5Ze6Qb+ut2qwzQ8iZafvurzN0xpHSKrsmvAgMBAAEwDQYJKoZIhvcNAQEL\r\nBQADggEBAAOPq/Aq0LbpsYQqz6qHPp7reMbkkDoOYIZ/Bkxgki6lij17Nmr1GBv8\r\nJIjOcD+5LpVOiKnNxMLKPO+wYIyZ4XhoXQLKqzWIWC8LiZwhu19LWF7FbVei902f\r\njj7XGOYy0pvIJ0CMCUY+lSGsJy+gSd4lzTCIjn1tOrHwV9J5rtN2x3YKXfd0KkGB\r\nn72BSvmo+KXnItCp+GvqLceeMN8siilOcVPnUuw2BMJL883X4O/1YxaA78PVGVTl\r\n/XoLwLvgXev/eKjBxU0g6tcES6EYQZyitLTjlj8GwUZwSl0GC1tBMKBREIDCmmus\r\nPZK9awkEt567lRnzygG42LRO4t5Zr6w=\r\n-----END CERTIFICATE-----";
//     R"T(-----BEGIN CERTIFICATE-----
// MIIDczCCAlsCCQCCo7H9ahGnyjANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD
// TjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD
// VQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg10
// ZXN0QHRlc3QuY29tMCAXDTIyMTIwMjA1NTUyNVoYDzIwNzkwMTE3MDU1NTI1WjB9
// MQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK
// DARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDzANBgNVBAMMBlNFUlZFUjEd
// MBsGCSqGSIb3DQEJARYOY2hpbGRAdGVzdC5jb20wggEiMA0GCSqGSIb3DQEBAQUA
// A4IBDwAwggEKAoIBAQDUgu6dIFP53E4Qedj1nXJEddIpkgvgFyedJTaw56YKmbSC
// VzioL9qoptQoyMV8gscYeEUj11oCb9BHg60c5ZHm3roHUU3lii0B/fyapGFKTDGf
// yiBRtPjvSVPgI0WQJbdPQ+cT3shLUamUTcrj1B0cQJLzS9KCdSae9XPehQqOoeAR
// Vc1Fs3YZzrI2WCV4/j+s+TQG3MyLXyuK0Bf45ldvQ4w91j9jJyBdJ8P+dyf/NfZG
// BvzUkzl1oE0yOn+CyayYIitxG40qW3eoQYk2iTN8maGQCFuBdbkSBwOonFukEv+/
// L93R3M5Ze6Qb+ut2qwzQ8iZafvurzN0xpHSKrsmvAgMBAAEwDQYJKoZIhvcNAQEL
// BQADggEBAAOPq/Aq0LbpsYQqz6qHPp7reMbkkDoOYIZ/Bkxgki6lij17Nmr1GBv8
// JIjOcD+5LpVOiKnNxMLKPO+wYIyZ4XhoXQLKqzWIWC8LiZwhu19LWF7FbVei902f
// jj7XGOYy0pvIJ0CMCUY+lSGsJy+gSd4lzTCIjn1tOrHwV9J5rtN2x3YKXfd0KkGB
// n72BSvmo+KXnItCp+GvqLceeMN8siilOcVPnUuw2BMJL883X4O/1YxaA78PVGVTl
// /XoLwLvgXev/eKjBxU0g6tcES6EYQZyitLTjlj8GwUZwSl0GC1tBMKBREIDCmmus
// PZK9awkEt567lRnzygG42LRO4t5Zr6w=
// -----END CERTIFICATE-----
// )T";

static std::string const ca_error_content =
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

static std::string const ca_right_other_content =
    R"T(-----BEGIN CERTIFICATE-----
MIIDcTCCAlkCCQCCo7H9ahGnyzANBgkqhkiG9w0BAQsFADB4MQswCQYDVQQGEwJD
TjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQKDARRRFpZMRUwEwYD
VQQLDAx3d3cudGVzdC5jb20xCzAJBgNVBAMMAkNBMRwwGgYJKoZIhvcNAQkBFg10
ZXN0QHRlc3QuY29tMCAXDTIyMTIwMjA4MDYyOFoYDzIwNzkwMTE3MDgwNjI4WjB7
MQswCQYDVQQGEwJDTjELMAkGA1UECAwCU0QxCzAJBgNVBAcMAkpOMQ0wCwYDVQQK
DARRRFpZMRUwEwYDVQQLDAx3d3cudGVzdC5jb20xDTALBgNVBAMMBHF3ZXIxHTAb
BgkqhkiG9w0BCQEWDmNoaWxkQHRlc3QuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOC
AQ8AMIIBCgKCAQEAsYxLX+dLVSZDq9PGDenXlX+NYWls2gkdSy9hOO5ht6auQB0V
3vheXoEkTo8+8xDFNfPjobTtEXawWkZgDsB/v3uz6xxuw+Iz3eqx+DWJ30pBZqTp
3vo7+lk73rfhRImAZCQy26dWSmkSRrF7ZmTTzbPXKzmtO7EQ7g0EGyRSRGvehVep
GcwHAP4NkO9lxy3zBsgHxalHqMrIKdZ4Lnwd+Z6KNI3v8iBAvOAIMzwpzlr47dr/
FaozHWXz3FJ/7l2VlKVEvrtTbD2CvtmTRQulO32h77kGorwPRYdmwxKJX/OTPwvj
Arfnvm4+5uW57NzNvDIKPAZf00SHis74o/na2wIDAQABMA0GCSqGSIb3DQEBCwUA
A4IBAQB91RTYMwZh9GkRdVCmBLoEGw29xSptZg7VAqq2sw3dLwMuDr0j84F9a7nt
PtSAqrYN+Ek6LiUPYTY1BAohwZ4kCtXaHtY1LkE08yMX7yrKngNsIC4IF/4qjhAd
QfDcds+AKKJVycO0Yhc4ojYbEms+FVwXfNVXEzla0EG6R1PsMpMTC8tmj5ELHTfh
k33iSrXlJv/e72iJOeP6jNnj13YzFNzOGrAHyMaYiBYDlzCw0xC4Rciz57G7XhC0
DroLqw2aQcYfmB1/avXjPA/0L188Tp7Ux8QP++tTJzonw6VLFjZbTe0OpEuT+JJ1
5cvQyvvmx4vrdZ0d0NcKhXDb5XbV
-----END CERTIFICATE-----
)T";

static const char cert_der[] = {
    0x30, 0x82, 0x01, 0x51, 0x30, 0x81, 0xf7, 0xa0, 0x03, 0x02, 0x01, 0x02,
    0x02, 0x02, 0x03, 0x09, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce,
    0x3d, 0x04, 0x03, 0x02, 0x30, 0x27, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03,
    0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x18, 0x30, 0x16, 0x06,
    0x03, 0x55, 0x04, 0x03, 0x0c, 0x0f, 0x63, 0x72, 0x79, 0x70, 0x74, 0x6f,
    0x67, 0x72, 0x61, 0x70, 0x68, 0x79, 0x20, 0x43, 0x41, 0x30, 0x1e, 0x17,
    0x0d, 0x31, 0x37, 0x30, 0x31, 0x30, 0x31, 0x31, 0x32, 0x30, 0x31, 0x30,
    0x30, 0x5a, 0x17, 0x0d, 0x33, 0x38, 0x31, 0x32, 0x33, 0x31, 0x30, 0x38,
    0x33, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x27, 0x31, 0x0b, 0x30, 0x09, 0x06,
    0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x18, 0x30, 0x16,
    0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x0f, 0x63, 0x72, 0x79, 0x70, 0x74,
    0x6f, 0x67, 0x72, 0x61, 0x70, 0x68, 0x79, 0x20, 0x43, 0x41, 0x30, 0x59,
    0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06,
    0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00,
    0x04, 0x18, 0xff, 0xcf, 0xbb, 0xf9, 0x39, 0xb8, 0xf5, 0xdd, 0xc3, 0xee,
    0xc0, 0x40, 0x8b, 0x06, 0x75, 0x06, 0xab, 0x4f, 0xcd, 0xd8, 0x2c, 0x52,
    0x24, 0x4e, 0x1f, 0xe0, 0x10, 0x46, 0x67, 0xb5, 0x5f, 0x15, 0xb9, 0x62,
    0xbd, 0x3b, 0xcf, 0x0c, 0x6f, 0xbe, 0x1a, 0xf7, 0xb4, 0xa1, 0x0f, 0xb4,
    0xb9, 0xcb, 0x6e, 0x86, 0xb3, 0x50, 0xf9, 0x6c, 0x51, 0xbf, 0xc1, 0x82,
    0xd7, 0xbe, 0xc5, 0xf9, 0x05, 0xa3, 0x13, 0x30, 0x11, 0x30, 0x0f, 0x06,
    0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05, 0x30, 0x03, 0x01,
    0x01, 0xff, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04,
    0x03, 0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21, 0x00, 0xd1, 0x12,
    0xef, 0x8d, 0x97, 0x5a, 0x6e, 0xb8, 0xb6, 0x41, 0xa7, 0xcf, 0xc0, 0xe7,
    0xa4, 0x6e, 0xae, 0xda, 0x51, 0xe4, 0x64, 0x54, 0x2b, 0xde, 0x86, 0x95,
    0xbc, 0xf7, 0x1e, 0x9a, 0xf9, 0x5b, 0x02, 0x21, 0x00, 0xd1, 0x61, 0x86,
    0xce, 0x66, 0x31, 0xe4, 0x2f, 0x54, 0xbd, 0xf5, 0xc8, 0x2b, 0xb3, 0x44,
    0xce, 0x24, 0xf8, 0xa5, 0x0b, 0x72, 0x11, 0x21, 0x34, 0xb9, 0x15, 0x4a,
    0x5f, 0x0e, 0x27, 0x32, 0xa9
};

namespace top {
class test_xca_auth : public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

public:
};

TEST_F(test_xca_auth, ca_read_right_json_test)
{

    xJson::Reader reader;
    Json::Value json_root;

    bool ret = reader.parse(g_ci_genesis_config, json_root);
    const auto members = json_root.getMemberNames();
    std::string z_ca_root_conent = json_root["ca_root_conent"].asString();

    std::string z_ca_right_content = json_root["ca_right_content"].asString();

    xtop_ca_auth ca_auth;
    EXPECT_EQ(1, ca_auth.add_root_cert(z_ca_root_conent.c_str()));
    EXPECT_EQ(1, ca_auth.verify_leaf_cert(z_ca_right_content.c_str()));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() > 0);

    EXPECT_EQ(1, ca_auth.verify_leaf_cert(ca_right_other_content.c_str()));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() > 0);
}

// test root verify level_1
TEST_F(test_xca_auth, ca_read_right_test)
{
    xtop_ca_auth ca_auth;
    EXPECT_EQ(1, ca_auth.add_root_cert(ca_root_conent.c_str()));
    EXPECT_EQ(1, ca_auth.verify_leaf_cert(ca_right_content.c_str()));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() > 0);

    EXPECT_EQ(1, ca_auth.verify_leaf_cert(ca_right_other_content.c_str()));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() > 0);
}

TEST_F(test_xca_auth, ca_read_error_test)
{
    xtop_ca_auth ca_auth;
    EXPECT_EQ(1, ca_auth.add_root_cert(ca_root_conent.c_str()));
    EXPECT_NE(1, ca_auth.verify_leaf_cert(ca_error_content.c_str()));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() == 0);
}

TEST_F(test_xca_auth, ca_read_null_test)
{
    xtop_ca_auth ca_auth;
    EXPECT_NE(1, ca_auth.add_root_cert(""));
    EXPECT_NE(1, ca_auth.verify_leaf_cert(""));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() == 0);
}

TEST_F(test_xca_auth, ca_read_der_format_test)
{
    xtop_ca_auth ca_auth;
    EXPECT_EQ(1, ca_auth.add_root_cert(cert_der, 2, sizeof(cert_der)));
    EXPECT_EQ(1, ca_auth.verify_leaf_cert(cert_der, 2, sizeof(cert_der)));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() > 0);
}

TEST_F(test_xca_auth, ca_read_der_format_no_len_test)
{
    xtop_ca_auth ca_auth;
    EXPECT_NE(1, ca_auth.add_root_cert(cert_der, 2));
    EXPECT_NE(1, ca_auth.verify_leaf_cert(cert_der, 2));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() == 0);
}

TEST_F(test_xca_auth, ca_read_der_error_format_test)
{
    xtop_ca_auth ca_auth;
    EXPECT_NE(1, ca_auth.add_root_cert(cert_der, 1, sizeof(cert_der)));
    EXPECT_NE(1, ca_auth.verify_leaf_cert(cert_der, 1, sizeof(cert_der)));
    EXPECT_TRUE(ca_auth.get_cert_expiry_time() == 0);
}

}
