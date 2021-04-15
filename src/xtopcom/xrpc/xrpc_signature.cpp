#include "xrpc_signature.h"
#include "xbase/xns_macro.h"
#include "xrpc_define.h"
#include <iostream>

extern "C" {
#include "trezor-crypto/hmac.h"
#include "trezor-crypto/base32.h"
}

NS_BEG2(top, xChainRPC)

const std::string xrpc_signature::signature_key_    { "signature" };
const std::string xrpc_signature::method_key_       { "signature_method" };
const std::string xrpc_signature::version_key_      { "signature_ver_code" };
const std::string xrpc_signature::secretkey_key_    { "secret_key" };
const std::string xrpc_signature::keyid_key_        { "identity_token" };
const std::string xrpc_signature::method_value_     { "hmac_sha2" };
const std::string xrpc_signature::version_value_    { "1.0" };

bool xrpc_signature::check(const std::string& content, const std::string& key) {
    parse_params(content);
    return check_parsed_params(key);
}

bool xrpc_signature::check_parsed_params(const std::string& key) {
    std::string msg;
    encode(msg);

    std::string sign;
    auto it = params_.find(signature_key_);
    sign = it != params_.end() ? it->second : "";

    if (is_optional_ && sign.empty()) {
        xinfo_rpc("xrpc_signature: no signature, skip check.");
        return true;
    }

    return check_signature(msg, key, sign);
}

void xrpc_signature::insert_param(const std::string& key, const std::string& value) {
    params_.insert(ParamMap::value_type(key, value));
}

bool xrpc_signature::check_signature(const std::string& msg, const std::string& key, const std::string& sign) {
    uint8_t hmac[32];
    hmac_sha256(reinterpret_cast<const uint8_t*>(key.c_str()), key.length(),
        reinterpret_cast<const uint8_t*>(msg.c_str()), msg.length(), hmac);

    char buffer[64] = { 0 };
    if (NULL == base32_encode(hmac, 32, buffer,
        sizeof(buffer), BASE32_ALPHABET_RFC4648)) {
        return false;
    }

    std::string hmac_str = std::string(buffer);
    return sign == hmac_str;
}

void xrpc_signature::encode(std::string& body) {
    for (auto it : params_) {
        if (it.first != signature_key_) {
            body += (body.length() == 0 ? "" : "&");
            body += it.first;
            body += "=";
            body += it.second;
        }
    }
}

void xrpc_signature::parse_params(const std::string& content) {
    size_t len = content.length();
    size_t pos_beg{ 0 };
    size_t pos_end = pos_beg;
    while (pos_end != std::string::npos && pos_beg < len) {
        pos_end = content.find("&", pos_beg);
        if (pos_end == std::string::npos) {
            parse_keyvalue(content.substr(pos_beg, len - pos_beg));
        }
        else {
            parse_keyvalue(content.substr(pos_beg, pos_end - pos_beg));
            pos_beg = pos_end + 1;
        }
    };
}

void xrpc_signature::parse_keyvalue(const std::string& content) {
    auto len = content.length();
    auto pos = content.find("=", 0);
    if (pos != std::string::npos &&
        pos != 0) {
        params_.insert(ParamMap::value_type(
            content.substr(0, pos),
            content.substr(pos + 1, len - pos - 1)));
    }
}



NS_END2
