#pragma once

#include "xbase/xns_macro.h"

#include <msgpack.hpp>

#include <netinet/in.h>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xin_addr_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xin_addr_in_addr_t_index{ 0 };

template <>
struct convert<::in_addr> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, ::in_addr & v) const {
        if (o.type != msgpack::type::ARRAY || o.via.array.size != xin_addr_field_count) {
            throw msgpack::type_error{};
        }

        v.s_addr = o.via.array.ptr[0].as<::in_addr_t>();

        return o;
    }
};

template <>
struct pack<::in_addr>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, ::in_addr const & message) const {
        o.pack_array(xin_addr_field_count);
        o.pack(message.s_addr);

        return o;
    }
};

template <>
struct object_with_zone<::in_addr>
{
    void
    operator()(msgpack::object::with_zone & o, ::in_addr const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xin_addr_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[0] = msgpack::object{ message.s_addr, o.zone };
    }
};

NS_END1
}
NS_END1
