// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xcodec/xmsgpack/xip_codec.hpp"

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xcluster_address_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xcluster_address_xip_field_index{ 0 };

template <>
struct convert<top::common::xcluster_address_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xcluster_address_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }


        if (o.via.array.size == 0) {
            return o;
        }

        switch (o.via.array.size - 1) {
            default: {
                XATTRIBUTE_FALLTHROUGH;
            }

            case xcluster_address_xip_field_index: {
                v = top::common::xcluster_address_t{ o.via.array.ptr[xcluster_address_xip_field_index].as<top::common::xip_t>() };
                break;
            }
        }

        return o;
    }
};

template <>
struct pack<top::common::xcluster_address_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::common::xcluster_address_t const & message) const {
        o.pack_array(xcluster_address_field_count);
        o.pack(message.xip());

        return o;
    }
};

template <>
struct object_with_zone<top::common::xcluster_address_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::common::xcluster_address_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xcluster_address_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xcluster_address_xip_field_index]  = msgpack::object{ message.xip(), o.zone };
    }
};

NS_END1
}
NS_END1
