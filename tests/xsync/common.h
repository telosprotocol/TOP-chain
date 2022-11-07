#pragma once

#include "xvnetwork/xaddress.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "xvnetwork/xmessage.h"
#include "xcommon/xip.h"

extern
top::vnetwork::xvnode_address_t
get_beacon_address(uint32_t nid, uint64_t version, const std::string& node_id);

extern
top::vnetwork::xvnode_address_t
get_zec_address(uint32_t nid, uint64_t version, const std::string& node_id);

extern
top::vnetwork::xvnode_address_t
get_archive_address(uint32_t nid, uint64_t version, const std::string& node_id);

extern
top::vnetwork::xvnode_address_t
get_auditor_address(uint32_t nid, uint64_t version, const std::string& node_id, int offset);

extern
top::vnetwork::xvnode_address_t
get_validator_address(uint32_t nid, uint64_t version, const std::string& node_id, int offset);

extern
std::vector<top::vnetwork::xvnode_address_t>
get_beacon_addresses(uint32_t nid, uint64_t version, int count);

extern
std::vector<top::vnetwork::xvnode_address_t>
get_zec_addresses(uint32_t nid, uint64_t version, int count);

extern
std::vector<top::vnetwork::xvnode_address_t>
get_archive_addresses(uint32_t nid, uint64_t version, int count);

extern
std::vector<top::vnetwork::xvnode_address_t>
get_auditor_addresses(uint32_t nid, uint64_t version, int offset, int count);

extern
std::vector<top::vnetwork::xvnode_address_t>
get_validator_addresses(uint32_t nid, uint64_t version, int offset, int count);


extern top::vnetwork::xvnode_address_t create_beacon_addr(uint16_t _slot_id, const std::string & name, std::size_t id);
extern top::vnetwork::xvnode_address_t create_zec_addr(uint16_t _slot_id, const std::string & name, std::size_t id);
extern top::vnetwork::xvnode_address_t create_archive_addr(uint16_t _slot_id, const std::string & name, std::size_t id);
extern top::vnetwork::xvnode_address_t create_auditor_addr(uint16_t _slot_id, const std::string & name, std::size_t id);
extern top::vnetwork::xvnode_address_t create_validator_addr(uint16_t _slot_id, const std::string & name, std::size_t id);


class xmsg_item_t {
public:
    top::vnetwork::xmessage_t m_message;
    top::vnetwork::xvnode_address_t m_src;
    top::vnetwork::xvnode_address_t m_dst;

    xmsg_item_t() {
    }

    xmsg_item_t(const xmsg_item_t &other) {
        m_message = other.m_message;
        m_src = other.m_src;
        m_dst = other.m_dst;
    }
};

class xmsg_item_frozen_t {
public:
    top::vnetwork::xmessage_t m_message;
    top::vnetwork::xvnode_address_t m_src;
    top::common::xip2_t m_dst;

    xmsg_item_frozen_t() {
    }

    xmsg_item_frozen_t(const xmsg_item_frozen_t &other) {
        m_message = other.m_message;
        m_src = other.m_src;
        m_dst = other.m_dst;
    }
};

class xmock_vhost_sync_t : public top::tests::vnetwork::xtop_dummy_vhost {
public:
    bool read_msg(top::vnetwork::xmessage_t & message,
                      top::vnetwork::xvnode_address_t & src,
                      top::vnetwork::xvnode_address_t & dst) {
        if (m_items.size() == 0)
            return false;

        message = m_items.front().m_message;
        src = m_items.front().m_src;
        dst = m_items.front().m_dst;
        m_items.pop_front();
        return true;
    }

/*    void broadcast(top::common::xnode_address_t const & src, top::common::xip2_t const & dst, xmessage_t const & message, std::error_code & ec) override {
        xmsg_item_frozen_t item;
        item.m_message = message;
        item.m_src = src;
        item.m_dst = dst;
        m_frozen_items.push_back(item);
    }*/
    void broadcast(top::common::xnode_address_t const & src, top::common::xnode_address_t const & dst, xmessage_t const & message, std::error_code & ec) override {
    }

    bool read_frozen_broadcast_msg(top::vnetwork::xmessage_t & message,
                      top::vnetwork::xvnode_address_t & src,
                      top::common::xip2_t & dst) {
        if (m_frozen_items.size() == 0)
            return false;

        message = m_frozen_items.front().m_message;
        src = m_frozen_items.front().m_src;
        dst = m_frozen_items.front().m_dst;
        m_frozen_items.pop_front();
        return true;
    }

private:
    std::list<xmsg_item_t> m_items;
    std::list<xmsg_item_frozen_t> m_frozen_items;
};
