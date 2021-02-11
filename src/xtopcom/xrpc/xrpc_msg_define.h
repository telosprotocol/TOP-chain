// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <sstream>
#include <msgpack.hpp>
#include "xrpc_define.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xcodec/xmsgpack/xvnode_address_codec.hpp"

NS_BEG2(top, xrpc)
using vnetwork::xvnode_address_t;
struct xrpc_msg_request_t
{
    xrpc_msg_request_t() {}
    xrpc_msg_request_t(const enum_xrpc_type type, const enum_xrpc_tx_type tx_type, const xvnode_address_t& addr, const uint64_t uuid, const string& client_id, const string& account, const string& msg_body)
        :m_type(type), m_tx_type(tx_type), m_source_address(addr), m_uuid(uuid), m_client_id(client_id), m_account(account), m_message_body(msg_body) {}

    enum_xrpc_type      m_type;
    enum_xrpc_tx_type   m_tx_type;
    xvnode_address_t    m_source_address;
    uint64_t            m_uuid;
    string              m_client_id;
    string              m_account;
    string              m_message_body;
    xvnode_address_t    m_advance_address{}; /*record advance address, which router msg*/
    uint64_t            m_timer_height{ 0 }; /*tx specific clock timer height*/
    MSGPACK_DEFINE(m_type, m_tx_type, m_source_address, m_uuid, m_client_id, m_account, m_message_body);
};

struct xrpc_msg_response_t
{
    xrpc_msg_response_t() {}
    xrpc_msg_response_t(const xrpc_msg_request_t request)
        :m_type(request.m_type),
        m_source_address(request.m_source_address),
        m_uuid(request.m_uuid) {}

    enum_xrpc_type      m_type;
    common::xnode_address_t    m_source_address;
    common::xnode_address_t    m_signature_address;
    uint64_t            m_uuid;
    string              m_message_body;
    std::string to_string() const {
        return std::to_string(m_uuid) + ":" + m_signature_address.to_string() + ":" + m_message_body;
    }
    MSGPACK_DEFINE(m_type, m_source_address, m_signature_address, m_uuid, m_message_body);
};
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_request, 0x00000001);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_response, 0x00000002);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_query_request, 0x00000003);
NS_END2
MSGPACK_ADD_ENUM(top::xrpc::enum_xrpc_type);
MSGPACK_ADD_ENUM(top::xrpc::enum_xrpc_tx_type);

