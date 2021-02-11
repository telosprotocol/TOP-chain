// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xsimple_address.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xsimple_address_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xsimple_address_value_index{ 0 };

template <typename AddressTagType, typename AddressValueType>
struct convert<top::common::xsimple_address_t<AddressTagType, AddressValueType>> final
{
    msgpack::object const &
    operator()(msgpack::object const & o,
               top::common::xsimple_address_t<AddressTagType, AddressValueType> & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xsimple_address_field_count <= o.via.array.size) {
            v = top::common::xsimple_address_t<AddressTagType, AddressValueType>
            {
                o.via.array.ptr[xsimple_address_value_index].as<typename top::common::xsimple_address_t<AddressTagType, AddressValueType>::value_type>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <typename AddressTagType, typename AddressValueType>
struct pack<top::common::xsimple_address_t<AddressTagType, AddressValueType>>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::common::xsimple_address_t<AddressTagType, AddressValueType> const & message) const {
        o.pack_array(xsimple_address_field_count);
        o.pack(message.value());

        return o;
    }
};

template <typename AddressTagType, typename AddressValueType>
struct object_with_zone<top::common::xsimple_address_t<AddressTagType, AddressValueType>>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::common::xsimple_address_t<AddressTagType, AddressValueType> const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xsimple_address_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xsimple_address_value_index] = msgpack::object{ message.value(), o.zone };
    }
};

NS_END1
}
NS_END1
