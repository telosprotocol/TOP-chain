// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xversion_codec.hpp"

//#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
//#include "xvnetwork/xversion.h"

//#include <msgpack.hpp>

//NS_BEG1(msgpack)
//MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
//NS_BEG1(adaptor)
//
//XINLINE_CONSTEXPR std::size_t xversion_field_count{ 1 };
//XINLINE_CONSTEXPR std::size_t xversion_value_index{ 0 };
//
//template <>
//struct convert<top::vnetwork::xversion_t> final
//{
//    msgpack::object const &
//    operator()(msgpack::object const & o, top::vnetwork::xversion_t & v) const {
//        if (o.type != msgpack::type::ARRAY) {
//            throw msgpack::type_error{};
//        }
//
//        if (xversion_field_count <= o.via.array.size) {
//            v = top::vnetwork::xversion_t
//            {
//                o.via.array.ptr[xversion_value_index].as<top::xid_t<top::vnetwork::xversion_t>>(),
//            };
//        } else {
//            throw msgpack::type_error{};
//        }
//
//        return o;
//    }
//};
//
//template <>
//struct pack<top::vnetwork::xversion_t>
//{
//    template <typename Stream>
//    msgpack::packer<Stream> &
//    operator()(msgpack::packer<Stream> & o, top::vnetwork::xversion_t const & message) const {
//        o.pack_array(xversion_field_count);
//        o.pack(message.value());
//
//        return o;
//    }
//};
//
//template <>
//struct object_with_zone<top::vnetwork::xversion_t>
//{
//    void
//    operator()(msgpack::object::with_zone & o, top::vnetwork::xversion_t const & message) const {
//        o.type = type::ARRAY;
//        o.via.array.size = xversion_field_count;
//        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
//
//        o.via.array.ptr[xversion_value_index] = msgpack::object{ message.value(), o.zone };
//    }
//};
//
//NS_END1
//}
//NS_END1
