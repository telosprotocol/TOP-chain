//
//  platform_kadmlia_key.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_utils.h"

namespace top {

namespace base {
class XipParser;

#pragma pack(push) 
#pragma pack(1)

#define TOP_KADMLIA_XIP_PARSER_XIP_PLATFORM_NODE_FIELDS \
        uint32_t xnetwork_id : 24; \
        uint8_t network_type : 5; \
        uint8_t xaddress_domain_xip : 1; \
        uint8_t xip_type : 2; \
        uint8_t zone_id : 8; \
        uint8_t cluster_id : 8; \
        uint8_t group_id : 8; \
        uint8_t node_id : 8; \
        uint8_t xnetwork_version : 8; \
        uint32_t xinterface_id : 32

typedef union PlatformKadmiliaKeyItem {
    PlatformKadmiliaKeyItem() {
        memset(c_str, 0xFF, sizeof(c_str));
    }
    struct {
        TOP_KADMLIA_XIP_PARSER_XIP_PLATFORM_NODE_FIELDS;
        uint8_t process_id : 4;
        uint8_t router_id : 4;
        uint8_t switch_id : 8;
        uint8_t local_id : 8;
        uint32_t reserve : 32;
        char sha128[16];
    } kad_key_detail;

    struct {
        uint64_t high;
        uint64_t low;
        uint32_t reserve : 32;
        char sha128[16];
    } kad_key_high_low;

    struct {
        TOP_KADMLIA_XIP_PARSER_XIP_PLATFORM_NODE_FIELDS;
        uint32_t client_id : 24;  // node: 0, client: !0
        uint32_t reserve : 32;
        char sha128[16];
    } kad_key_client;

    struct {
        uint32_t xnetwork_id : 24;
        uint8_t zone_id : 8;
        char pub_key[32];
    } xid;

    char c_str[36];
} PlatformKadmiliaKeyItem_;

union UnionPlatformServiceType {
    struct {
        uint32_t network_id : 24;
        uint8_t zone_id : 8;
        uint8_t cluster_id : 8;
        uint8_t group_id : 8;
        uint8_t node_id : 8;
        uint8_t network_type : 8;
    } type_detail;
    uint64_t service_type;
};

#pragma pack(pop)

class PlatformKadmliaKey : public KadmliaKey {
public:
    PlatformKadmliaKey();
    PlatformKadmliaKey(uint32_t network_id, uint8_t network_type);
    PlatformKadmliaKey(const std::string& str_for_hash, uint64_t service_type);
    explicit PlatformKadmliaKey(uint64_t service_type);
    explicit PlatformKadmliaKey(const std::string& str_key);
    explicit PlatformKadmliaKey(const base::XipParser& xip);
    PlatformKadmliaKey(const base::XipParser& xip, const std::string& node_id);
    PlatformKadmliaKey(const std::string& node_id, bool hash_tag);
    virtual ~PlatformKadmliaKey() override;
    virtual const std::string& Get() override;
    virtual std::string GetPrivateKey() override { return private_key_; }
    virtual std::string GetPublicKey() override { return public_key_; }
    virtual uint64_t GetServiceType() override;
    virtual uint64_t GetServiceType(uint8_t network_type) override;
    virtual const std::string& GetNodeIdHash() override;

private:
    bool CreatePrivateAndPublicKey();
    void InitWithStringKey(const std::string& str_key);
    void SetDefaultXipValue();

    PlatformKadmiliaKeyItem kadmlia_key_;
    uint32_t reserve_;
    std::string private_key_;
    std::string public_key_;
    std::string str_;
    std::string node_id_hash_;

    DISALLOW_COPY_AND_ASSIGN(PlatformKadmliaKey);
};

}  // namespase base

}  // namespace top
