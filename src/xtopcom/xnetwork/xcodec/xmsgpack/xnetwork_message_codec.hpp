#pragma once

#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xmessage_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xmessage_transmission_property_codec.hpp"
#include "xnetwork/xnetwork_message.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xnetwork_message_field_count{ 4 };
XINLINE_CONSTEXPR std::size_t xnetwork_message_message_index{ 0 };
XINLINE_CONSTEXPR std::size_t xnetwork_message_sender_index{ 1 };
XINLINE_CONSTEXPR std::size_t xnetwork_message_receiver_index{ 2 };
XINLINE_CONSTEXPR std::size_t xnetwork_message_transmission_property_index{ 3 };

template <>
struct convert<top::network::xnetwork_message_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::network::xnetwork_message_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xnetwork_message_field_count <= o.via.array.size) {
            v = top::network::xnetwork_message_t
            {
                o.via.array.ptr[xnetwork_message_message_index].as<top::network::xmessage_t>(),
                o.via.array.ptr[xnetwork_message_sender_index].as<top::common::xnode_id_t>(),
                o.via.array.ptr[xnetwork_message_receiver_index].as<top::common::xnode_id_t>(),
                o.via.array.ptr[xnetwork_message_transmission_property_index].as<top::network::xtransmission_property_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xnetwork_message_t>
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::network::xnetwork_message_t const & message) const {
        o.pack_array(xnetwork_message_field_count);

        o.pack(message.message());
        o.pack(message.sender_id());
        o.pack(message.receiver_id());
        o.pack(message.transmission_property());

        return o;
    }
};

template <>
struct object_with_zone<top::network::xnetwork_message_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xnetwork_message_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xnetwork_message_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xnetwork_message_message_index]               = msgpack::object{ message.message(),               o.zone };
        o.via.array.ptr[xnetwork_message_sender_index]                = msgpack::object{ message.sender_id(),             o.zone };
        o.via.array.ptr[xnetwork_message_receiver_index]              = msgpack::object{ message.receiver_id(),           o.zone };
        o.via.array.ptr[xnetwork_message_transmission_property_index] = msgpack::object{ message.transmission_property(), o.zone };
    }
};

NS_END1
}
NS_END1
