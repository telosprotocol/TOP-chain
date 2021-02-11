// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xip_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xip_value_index{ 0 };

template <>
struct convert<top::common::xip_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xip_t & v) const {
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

            case xip_value_index: {
                v = top::common::xip_t{ o.via.array.ptr[xip_value_index].as<top::common::xip_t::value_type>() };
                break;
            }
        }

        return o;
    }
};

template <>
struct pack<top::common::xip_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::common::xip_t const & message) const {
        o.pack_array(xip_field_count);
        o.pack(message.value());

        return o;
    }
};

template <>
struct object_with_zone<top::common::xip_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::common::xip_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xip_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xip_value_index]  = msgpack::object{ message.value(), o.zone };
    }
};

NS_END1
}
NS_END1
