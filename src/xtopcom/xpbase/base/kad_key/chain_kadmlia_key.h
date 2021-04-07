//
//  chain_kadmlia_key.h
//
//  Created by Charlie Xie on 04/01/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include "xpbase/base/kad_key/kadmlia_key.h"

namespace top {

namespace base {

#pragma pack(push) 
#pragma pack(1)

#define TOP_KADMLIA_XIP_PARSER_XIP_CHAIN_NODE_FIELDS \
        uint32_t xnetwork_id : 24; \
        uint8_t zone_id : 8; \
        uint8_t cluster_id : 8; \
        uint8_t group_id : 8; \
        uint8_t node_id : 8; \
        uint8_t network_type : 5; \
        uint8_t xaddress_domain_xip : 1; \
        uint8_t xip_type : 2; \
        uint8_t xnetwork_version : 8; \
        uint32_t xinterface_id : 32 \

typedef union ChainKadmiliaKeyItem {

    struct {
        TOP_KADMLIA_XIP_PARSER_XIP_CHAIN_NODE_FIELDS;
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
        TOP_KADMLIA_XIP_PARSER_XIP_CHAIN_NODE_FIELDS;
        uint32_t client_id : 24;  // node: 0, client: !0
        uint32_t reserve : 32;
        char sha128[16];
    } kad_key_client;

    char c_str[36];
} ChainKadmiliaKeyItem_;

#pragma pack(pop)

class ChainKadmliaKey : public KadmliaKey {
public:
    ChainKadmliaKey();
    explicit ChainKadmliaKey(uint32_t reserve);
    explicit ChainKadmliaKey(uint64_t service_type);
    explicit ChainKadmliaKey(const base::XipParser& xip);
    ChainKadmliaKey(const base::XipParser& xip, const std::string& node_id);
    ChainKadmliaKey(const std::string& node_id, bool hash_tag);
    explicit ChainKadmliaKey(const std::string& str_key);
    virtual ~ChainKadmliaKey() override;
    virtual const std::string& Get() override;
    virtual std::string GetPrivateKey() override;
    virtual std::string GetPublicKey() override;
    virtual uint64_t GetServiceType() override;
    virtual uint64_t GetServiceType(uint8_t network_type) override;
    virtual const std::string& GetNodeIdHash() override;

private:
    void SetDefaultXipValue();
    void InitWithStringKey(const std::string& str_key);

    ChainKadmiliaKeyItem kadmlia_key_;
    uint32_t reserve_;
    std::string str_;
    std::string node_id_hash_;

    DISALLOW_COPY_AND_ASSIGN(ChainKadmliaKey);
};

}  // namespase base

}  // namespace top
