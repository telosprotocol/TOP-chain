#pragma once

#include <string>
#include <memory>

#include "xkad/security/security_def.h"
#include "xkad/proto/signature.pb.h"
#include "xkad/xcrypto/xcrypto_util.h"
#include "xkad/xutility/xhash.h"

namespace top {
namespace security {

class SecurityCrypt {
public:
    SecurityCrypt() {}
    ~SecurityCrypt() {}
    bool Init();
    void UnInit();
    //aes simple
    bool CreateAESKey(
        std::string&);
    bool AESEncrypt(
        const std::string,
        const std::string&,
        std::string&);
    bool AESDecrypt(
        const std::string,
        const std::string&,
        std::string&);
    //aes hard
    bool CreateAESKey(
        top::signature::protobuf::AESKey&);
    bool AESEncrypt(
        const std::string,
        const top::signature::protobuf::AESKey&,
        std::string&);
    bool AESDecrypt(
        const std::string,
        const top::signature::protobuf::AESKey&,
        std::string&);
    uint256_t DigestHash(
        const std::string&);
    std::string DigestSign(
        const uint256_t&);
    bool VerifySign(
        const uint256_t&,
        const std::string&);
private:
    void MakePrivateKey();
    bool inited_;
    std::mutex mutex_;
    uint8_t private_key_[32];
};

typedef std::shared_ptr<SecurityCrypt> SecurityCryptPtr;

}
}