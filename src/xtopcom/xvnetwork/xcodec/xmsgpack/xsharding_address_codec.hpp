// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xcodec/xmsgpack/xsharding_info_codec.hpp"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xcodec/xmsgpack/xversion_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xvnode_type_codec.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xsharding_address_field_count{ 3 };
XINLINE_CONSTEXPR std::size_t xsharding_address_info_index{ 0 };
XINLINE_CONSTEXPR std::size_t xsharding_address_version_index{ 1 };
XINLINE_CONSTEXPR std::size_t xsharding_address_type_index{ 2 };

template <>
struct convert<top::vnetwork::xsharding_address_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::vnetwork::xsharding_address_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xsharding_address_field_count <= o.via.array.size) {
            v = top::vnetwork::xsharding_address_t
            {
                o.via.array.ptr[xsharding_address_info_index].as<top::common::xsharding_info_t>(),
                o.via.array.ptr[xsharding_address_version_index].as<top::vnetwork::xversion_t>(),
                o.via.array.ptr[xsharding_address_type_index].as<top::common::xnode_type_t>(),
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::vnetwork::xsharding_address_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::vnetwork::xsharding_address_t const & message) const {
        o.pack_array(xsharding_address_field_count);
        o.pack(message.sharding_info());
        o.pack(message.version());
        o.pack(message.type());

        return o;
    }
};

template <>
struct object_with_zone<top::vnetwork::xsharding_address_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::vnetwork::xsharding_address_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xsharding_address_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xsharding_address_info_index] = msgpack::object{ message.sharding_info(), o.zone };
        o.via.array.ptr[xsharding_address_version_index] = msgpack::object{ message.version(), o.zone };
        o.via.array.ptr[xsharding_address_type_index] = msgpack::object{ message.type(), o.zone };
    }
};

NS_END1
}
NS_END1
