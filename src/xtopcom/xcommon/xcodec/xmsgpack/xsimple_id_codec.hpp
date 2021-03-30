// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xstring_id.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xsimple_id_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xsimple_id_value_index{ 0 };

template <typename IdTagType, typename IdValueType>
struct convert<top::common::xstring_id_t<IdTagType, IdValueType>> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xstring_id_t<IdTagType, IdValueType> & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xsimple_id_field_count < o.via.array.size) {
            v = top::common::xstring_id_t<IdTagType, IdValueType>
            {
                o.via.array.ptr[xsimple_id_value_index].as<typename top::common::xstring_id_t<IdTagType, IdValueType>::id_value_type>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <typename IdTagType, typename IdValueType>
struct pack<top::common::xstring_id_t<IdTagType, IdValueType>>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::common::xstring_id_t<IdTagType, IdValueType> const & message) const {
        o.pack_array(xsimple_id_field_count);
        o.pack(message.value());

        return o;
    }
};

template <typename IdTagType, typename IdValueType>
struct object_with_zone<top::common::xstring_id_t<IdTagType, IdValueType>>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::common::xstring_id_t<IdTagType, IdValueType> const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xsimple_id_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xsimple_id_value_index] = msgpack::object{ message.value(), o.zone };
    }
};

NS_END1
}
NS_END1
