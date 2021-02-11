#pragma once

#include "xcommon/xcodec/xmsgpack/xmessage_id_codec.hpp"
//#include "xvnetwork/xcodec/xmsgpack/xvnode_address_codec.hpp"
//#include "xvnetwork/xmessage.h"
#include "xbasic/xcodec/xmsgpack/xsimple_message_codec.hpp"

//#include <msgpack.hpp>

//NS_BEG1(msgpack)
//MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
//NS_BEG1(adaptor)
//
//XINLINE_CONSTEXPR std::size_t xmessage_field_count{ 4 };
//XINLINE_CONSTEXPR std::size_t xmessage_payload_index{ 0 };
//XINLINE_CONSTEXPR std::size_t xmessage_sender_index{ 1 };
//XINLINE_CONSTEXPR std::size_t xmessage_receiver_index{ 2 };
//XINLINE_CONSTEXPR std::size_t xmessage_id_index{ 3 };
//
//template <>
//struct convert<top::vnetwork::xmessage_t> final
//{
//    msgpack::object const &
//    operator()(msgpack::object const & o, top::vnetwork::xmessage_t & v) const
//    {
//        if (o.type != msgpack::type::ARRAY)
//        {
//            throw msgpack::type_error{};
//        }
//
//        if (xmessage_field_count <= o.via.array.size)
//        {
//            v = top::vnetwork::xmessage_t
//            {
//                o.via.array.ptr[xmessage_payload_index].as<top::xbyte_buffer_t>(),
//                o.via.array.ptr[xmessage_id_index].as<top::common::xmessage_id_t>(),
//                o.via.array.ptr[xmessage_receiver_index].as<top::vnetwork::xvnode_address_t>(),
//                o.via.array.ptr[xmessage_sender_index].as<top::vnetwork::xvnode_address_t>()
//            };
//        }
//        else
//        {
//            throw msgpack::type_error{};
//        }
//
//        return o;
//    }
//};
//
//template <>
//struct pack<top::vnetwork::xmessage_t>
//{
//    template <typename Stream>
//    msgpack::packer<Stream> &
//    operator()(msgpack::packer<Stream> & o, top::vnetwork::xmessage_t const & message) const
//    {
//        o.pack_array(xmessage_field_count);
//        o.pack(message.payload());
//        o.pack(message.receiver_address());
//        o.pack(message.sender_address());
//        o.pack(message.id());
//
//        return o;
//    }
//};
//
//template <>
//struct object_with_zone<top::vnetwork::xmessage_t>
//{
//    void
//    operator()(msgpack::object::with_zone & o, top::vnetwork::xmessage_t const & message) const
//    {
//        o.type = type::ARRAY;
//        o.via.array.size = xmessage_field_count;
//        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
//
//        o.via.array.ptr[xmessage_payload_index] = msgpack::object{ message.payload(), o.zone };
//        o.via.array.ptr[xmessage_sender_index] = msgpack::object{ message.sender_address(), o.zone };
//        o.via.array.ptr[xmessage_receiver_index] = msgpack::object{ message.receiver_address(), o.zone };
//        o.via.array.ptr[xmessage_id_index] = msgpack::object{ message.id(), o.zone };
//    }
//};
//
//NS_END1
//}
//NS_END1
