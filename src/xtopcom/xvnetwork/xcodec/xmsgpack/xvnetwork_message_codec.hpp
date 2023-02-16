// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xcodec/xmsgpack/xnode_address_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xmessage_codec.hpp"
#include "xvnetwork/xvnetwork_message.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xvnetwork_message_field_count{ 4 };
XINLINE_CONSTEXPR std::size_t xvnetwork_message_sender_index{ 0 };
XINLINE_CONSTEXPR std::size_t xvnetwork_message_receiver_index{ 1 };
XINLINE_CONSTEXPR std::size_t xvnetwork_message_message_index{ 2 };
XINLINE_CONSTEXPR std::size_t xvnetwork_message_logic_time_index{ 3 };

template <>
struct convert<top::vnetwork::xvnetwork_message_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::vnetwork::xvnetwork_message_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (o.via.array.size == 0) {
            return o;
        }

        top::common::xnode_address_t sender, receiver;
        top::vnetwork::xmessage_t msg;
        top::common::xlogic_time_t logic_time{top::common::xjudgement_day};

        switch (o.via.array.size - 1) {
            default: {
                XATTRIBUTE_FALLTHROUGH;
            }

            case xvnetwork_message_logic_time_index: {
                logic_time = o.via.array.ptr[xvnetwork_message_logic_time_index].as<top::common::xlogic_time_t>();
                XATTRIBUTE_FALLTHROUGH;
            }

            case xvnetwork_message_message_index: {
                msg = o.via.array.ptr[xvnetwork_message_message_index].as<top::vnetwork::xmessage_t>();
                XATTRIBUTE_FALLTHROUGH;
            }

            case xvnetwork_message_receiver_index: {
                receiver = o.via.array.ptr[xvnetwork_message_receiver_index].as<top::common::xnode_address_t>();
                XATTRIBUTE_FALLTHROUGH;
            }

            case xvnetwork_message_sender_index: {
                sender = o.via.array.ptr[xvnetwork_message_sender_index].as<top::common::xnode_address_t>();
                break;
            }
        }

        v = top::vnetwork::xvnetwork_message_t{ sender, receiver, msg, logic_time };

        return o;
    }
};

template <>
struct pack<top::vnetwork::xvnetwork_message_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::vnetwork::xvnetwork_message_t const & message) const {
        o.pack_array(xvnetwork_message_field_count);
        o.pack(message.sender());
        o.pack(message.receiver());
        o.pack(message.message());
        o.pack(message.logic_time());

        return o;
    }
};

template <>
struct object_with_zone<top::vnetwork::xvnetwork_message_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::vnetwork::xvnetwork_message_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xvnetwork_message_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xvnetwork_message_sender_index]     = msgpack::object{ message.sender(), o.zone };
        o.via.array.ptr[xvnetwork_message_receiver_index]   = msgpack::object{ message.receiver(), o.zone };
        o.via.array.ptr[xvnetwork_message_message_index]    = msgpack::object{ message.message(), o.zone };
        o.via.array.ptr[xvnetwork_message_logic_time_index] = msgpack::object{ message.logic_time(), o.zone };
    }
};

NS_END1
}
NS_END1
