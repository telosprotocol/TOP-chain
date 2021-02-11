//
//  xip_parser.h
//
//  Created by @Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <string>
#include <string.h>
#include <memory>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/xip_def.h"

namespace top {

namespace base {

class XipParser {
public:
    static uint32_t XipLen() {
        return sizeof(xip2_high_) + sizeof(xip_);
    }

    XipParser() : xip2_high_(0xFFFFFFFFFFFFFFFFULL), xip_(0xFFFFFFFFFFFFFFFFULL) {}
    explicit XipParser(const std::string& xip) {
        memcpy(&xip2_high_, xip.c_str(), sizeof(xip2_high_));  // NOLINT
        memcpy(&xip_, xip.c_str() + sizeof(xip2_high_), sizeof(xip_));  // NOLINT
    }

    XipParser(uint64_t xip2_high, uint64_t xip) {
        xip2_high_ = xip2_high;
        xip_ = xip;
    }

    XipParser(const XipParser& other) {
        xip2_high_ = other.xip2_high_;
        xip_ = other.xip_;
    }

    XipParser& operator=(const XipParser& other) {
        if (this != &other) {
            xip2_high_ = other.xip2_high_;
            xip_ = other.xip_;
        }
        return *this;
    }

    ~XipParser() {}

    bool valid() {
        return (xip2_high_ != 0xFFFFFFFFFFFFFFFFULL) && (xip_ != 0xFFFFFFFFFFFFFFFFULL);
    }

    inline std::string xip() const {
        return std::string((char*)&xip2_high_, sizeof(xip2_high_)) +  // NOLINT
            std::string((char*)&xip_, sizeof(xip_));  // NOLINT
    }

    inline void xip(uint64_t& high, uint64_t& low) const {
        high = xip2_high_;
        low = xip_;
    }

    inline bool valid_xnetwork_version() const {
        return (get_network_ver_from_xip(xip_) & 0xFF) != 0xFF;
    }

    inline bool valid_xnetwork_id() const {
        return (get_network_id_from_xip(xip_) & 0xFFFFFF) != 0xFFFFFF;
    }

    inline bool valid_zone_id() const {
        return (get_zone_id_from_xip(xip_) & 0x7F) != 0x7F;
    }

    inline bool valid_cluster_id() const {
        return (get_cluster_id_from_xip(xip_) & 0x7F) != 0x7F;
    }

    inline bool valid_group_id() const {
        return (get_group_id_from_xip(xip_) & 0xFF) != 0xFF;
    }

    inline bool valid_node_id() const {
        return (get_node_id_from_xip(xip_) & 0x3FF) != 0x3FF;
    }

    inline bool valid_server_id() const {
        return (get_server_id_from_xip(xip_) & 0xFFFFFFFF) != 0xFFFFFFFF;
    }
    // end xip_

    inline bool valid_xaddress_domain_xip() const {
        return (get_address_domain_from_xip(xip2_high_) & 0x01) != 0x01;
    }

    inline bool valid_xip_type() const {
        return (get_address_type_from_xip(xip2_high_) & 0x03) != 0x03;
    }

    inline bool valid_network_type() const {
        return (get_network_type_from_xip(xip2_high_) & 0x1F) != 0x1F;
    }

    inline bool valid_xinterface_id() const {
        return (get_interface_id_from_xip(xip2_high_) & 0xFFFFFFFF) != 0xFFFFFFFF;
    }

    inline bool valid_process_id() const {
        return (get_process_id_from_xip(xip2_high_) & 0x0F) != 0x0F;
    }

    inline bool valid_router_id() const {
        return (get_router_id_from_xip(xip2_high_) & 0x0F) != 0x0F;
    }

    inline bool valid_switch_id() const {
        return (get_switch_id_from_xip(xip2_high_) & 0xFF) != 0xFF;
    }

    inline bool valid_local_id() const {
        return (get_local_id_from_xip(xip2_high_) & 0xFF) != 0xFF;
    }
    // end xip2_high_

    inline void reset_xnetwork_version() {
        xip_ = reset_network_ver_to_xip(xip_);
    }

    inline void reset_xnetwork_id() {
        xip_ = reset_network_id_to_xip(xip_);
    }

    inline void reset_zone_id() {
        xip_ = reset_zone_id_to_xip(xip_);
    }

    inline void reset_cluster_id() {
        xip_ = reset_cluster_id_to_xip(xip_);
    }

    inline void reset_group_id() {
        xip_ = reset_group_id_to_xip(xip_);
    }

    inline void reset_node_id() {
        xip_ = reset_node_id_to_xip(xip_);
    }

    inline void reset_server_id() {
        xip_ = reset_server_id_to_xip(xip_);
    }
    // end xip_

    inline void reset_xaddress_domain_xip() {
        xip2_high_ = reset_address_domain_to_xip(xip2_high_);
    }

    inline void reset_xip_type() {
        xip2_high_ = reset_address_type_to_xip(xip2_high_);
    }

    inline void reset_network_type() {
        xip2_high_ = reset_network_type_to_xip(xip2_high_);
    }

    inline void reset_xinterface_id() {
        xip2_high_ = reset_interface_id_to_xip(xip2_high_);
    }

    inline void reset_process_id() {
        xip2_high_ = reset_process_id_to_xip(xip2_high_);
    }

    inline void reset_router_id() {
        xip2_high_ = reset_router_id_to_xip(xip2_high_);
    }

    inline void reset_switch_id() {
        xip2_high_ = reset_switch_id_to_xip(xip2_high_);
    }

    inline void reset_local_id() {
        xip2_high_ = reset_local_id_to_xip(xip2_high_);
    }
    // end xip2_high_

    inline uint8_t xnetwork_version() const {
        return get_network_ver_from_xip(xip_);
    }

    inline uint32_t xnetwork_id() const {
        return get_network_id_from_xip(xip_);
    }

    inline uint8_t zone_id() const {
        return get_zone_id_from_xip(xip_);
    }

    inline uint8_t cluster_id() const {
        return get_cluster_id_from_xip(xip_);
    }

    inline uint8_t group_id() const {
        return get_group_id_from_xip(xip_);
    }

    inline uint32_t node_id() const {
        return get_node_id_from_xip(xip_);
    }

    inline uint32_t server_id() const {
        return get_server_id_from_xip(xip_);
    }

    inline uint8_t xaddress_domain_xip() const {
        return get_address_domain_from_xip(xip2_high_);
    }

    inline uint8_t xip_type() const {
        return get_address_type_from_xip(xip2_high_);
    }

    inline uint8_t network_type() const {
        return get_network_type_from_xip(xip2_high_);
    }

    inline uint32_t xinterface_id() const {
        return get_interface_id_from_xip(xip2_high_);
    }

    inline uint8_t process_id() const {
        return get_process_id_from_xip(xip2_high_);
    }

    inline uint8_t router_id() const {
        return get_router_id_from_xip(xip2_high_);
    }

    inline uint8_t switch_id() const {
        return get_switch_id_from_xip(xip2_high_);
    }

    inline uint8_t local_id() const {
        return get_local_id_from_xip(xip2_high_);
    }

    // set value
    inline void set_xnetwork_version(uint8_t xnetwork_version) {
        reset_xnetwork_version();
        xip_ = set_network_ver_to_xip(xip_, xnetwork_version);
    }

    inline void set_xnetwork_id(uint32_t xnetwork_id) {
        reset_xnetwork_id();
        xip_ = set_network_id_to_xip(xip_, xnetwork_id);
    }

    inline void set_zone_id(uint8_t zone_id) {
        reset_zone_id();
        xip_ = set_zone_id_to_xip(xip_, zone_id);
    }

    inline void set_cluster_id(uint8_t cluster_id) {
        reset_cluster_id();
        xip_ = set_cluster_id_to_xip(xip_, cluster_id);
    }

    inline void set_group_id(uint8_t group_id) {
        reset_group_id();
        xip_ = set_group_id_to_xip(xip_, group_id);
    }

    inline void set_node_id(uint8_t node_id) {
        reset_node_id();
        xip_ = set_node_id_to_xip(xip_, node_id);
    }

    inline void set_server_id(uint32_t server_id) {
        reset_server_id();
        xip_ = set_server_id_to_xip(xip_, server_id);
    }

    inline void set_xaddress_domain_xip(char enum_xaddress_domain_xip) {
        reset_xaddress_domain_xip();
        xip2_high_ = set_address_domain_to_xip(xip2_high_, enum_xaddress_domain_xip);
    }

    inline void set_xip_type(uint8_t enum_xip_type) {
        reset_xip_type();
        xip2_high_ = set_address_type_to_xip(xip2_high_, enum_xip_type);
    }

    inline void set_network_type(uint8_t xnetwork_type) {
        reset_network_type();
        xip2_high_ = set_network_type_to_xip(xip2_high_, xnetwork_type);
    }

    inline void set_xinterface_id(uint32_t xinterface_id) {
        reset_xinterface_id();
        xip2_high_ = set_interface_id_to_xip(xip2_high_, xinterface_id);
    }

    inline void set_process_id(uint8_t process_id) {
        reset_process_id();
        xip2_high_ = set_process_id_to_xip(xip2_high_, process_id);
    }

    inline void set_router_id(uint8_t router_id) {
        reset_router_id();
        xip2_high_ = set_router_id_to_xip(xip2_high_, router_id);
    }

    inline void set_switch_id(uint8_t switch_id) {
        reset_switch_id();
        xip2_high_ = set_switch_id_to_xip(xip2_high_, switch_id);
    }

    inline void set_local_id(uint8_t local_id) {
        reset_local_id();
        xip2_high_ = set_local_id_to_xip(xip2_high_, local_id);
    }

private:
    uint64_t xip2_high_;
    uint64_t xip_;
};

typedef std::shared_ptr<XipParser> XipParserPtr;

}  // namespace base

}  // namespace top

