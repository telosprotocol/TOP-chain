#pragma once

#include <map>
#include <string>
#include "xbasic/xns_macro.h"




NS_BEG2(top, xChainRPC)

class xrpc_signature {

public:
    using ParamMap = std::map<std::string, std::string>;

    xrpc_signature() {}
    ~xrpc_signature() {}

    bool check(const std::string& content, const std::string& key);
    bool check_parsed_params(const std::string& key);
    void insert_param(const std::string& key, const std::string& value);

private:
    bool check_signature(const std::string& msg,
        const std::string& key, const std::string& sign);
    void encode(std::string& body);
    void parse_params(const std::string& content);
    void parse_keyvalue(const std::string& content);

private:
    ParamMap params_;
    bool is_optional_{ true };

public:
    static const std::string signature_key_;
    static const std::string method_key_;
    static const std::string version_key_;
    static const std::string secretkey_key_;
    static const std::string keyid_key_;
    static const std::string method_value_;
    static const std::string version_value_;

};

NS_END2
