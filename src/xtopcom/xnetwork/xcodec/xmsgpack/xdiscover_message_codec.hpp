// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xnetwork/xp2p/xdiscover_message.h"

#include <msgpack.hpp>

MSGPACK_ADD_ENUM(top::network::p2p::xdiscover_message_type_t)

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xdiscover_message_field_count{ 3 };
XINLINE_CONSTEXPR std::size_t xdiscover_message_payload_index{ 0 };
XINLINE_CONSTEXPR std::size_t xdiscover_message_sender_id_index{ 1 };
XINLINE_CONSTEXPR std::size_t xdiscover_message_type_index{ 2 };

template <>
struct convert<top::network::p2p::xdiscover_message_t> final
{
    ::msgpack::object const &
    operator()(msgpack::object const & o,
               top::network::p2p::xdiscover_message_t & discover_message) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xdiscover_message_field_count <= o.via.array.size) {
            discover_message = top::network::p2p::xdiscover_message_t
            {
                o.via.array.ptr[xdiscover_message_payload_index].as<top::xbyte_buffer_t>(),
                o.via.array.ptr[xdiscover_message_sender_id_index].as<top::common::xnode_id_t>(),
                o.via.array.ptr[xdiscover_message_type_index].as<top::network::p2p::xdiscover_message_type_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::p2p::xdiscover_message_t>
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::network::p2p::xdiscover_message_t const & discover_message) const {
        o.pack_array(xdiscover_message_field_count);
        o.pack(discover_message.payload());
        o.pack(discover_message.sender_node_id());
        o.pack(discover_message.type());

        return o;
    }
};

template <>
struct object_with_zone<top::network::p2p::xdiscover_message_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::network::p2p::xdiscover_message_t const & discover_message) const {
        o.type = type::ARRAY;

        o.via.array.size = xdiscover_message_field_count;

        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xdiscover_message_payload_index] = msgpack::object{ discover_message.payload(), o.zone };
        o.via.array.ptr[xdiscover_message_sender_id_index] = msgpack::object{ discover_message.sender_node_id(), o.zone };
        o.via.array.ptr[xdiscover_message_type_index] = msgpack::object{ discover_message.type(), o.zone };
    }
};

NS_END1
}
NS_END1
