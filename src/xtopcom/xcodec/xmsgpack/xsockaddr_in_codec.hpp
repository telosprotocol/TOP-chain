#pragma once

#include "xcodec/xmsgpack/xin_addr_codec.hpp"

#include <msgpack.hpp>

#include <netinet/in.h>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xsockaddr_in_field_count{ 2 };
XINLINE_CONSTEXPR std::size_t xsockaddr_in_family{ 0 };
XINLINE_CONSTEXPR std::size_t xsockaddr_in_port{ 0 };
XINLINE_CONSTEXPR std::size_t xsockaddr_in_addr{ 1 };

template <>
struct convert<::sockaddr_in> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, ::sockaddr_in & v) const {
        if (o.type != ::msgpack::type::ARRAY ||
            o.via.array.size != xsockaddr_in_field_count) {
            throw ::msgpack::type_error{};
        }

        v.sin_family = AF_INET;
        v.sin_port   = o.via.array.ptr[xsockaddr_in_port].as<::in_port_t>();
        v.sin_addr   = o.via.array.ptr[xsockaddr_in_addr].as<::in_addr>();

        return o;
    }
};

template <>
struct pack<::sockaddr_in>
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, ::sockaddr_in const & message) const {
        o.pack_array(xsockaddr_in_field_count);
        o.pack(message.sin_port);
        o.pack(message.sin_addr);

        return o;
    }
};

template <>
struct object_with_zone<::sockaddr_in>
{
    void
    operator()(::msgpack::object::with_zone & o, ::sockaddr_in const & message) const {
        o.type           = ::msgpack::type::ARRAY;
        o.via.array.size = xsockaddr_in_field_count;
        o.via.array.ptr  = static_cast<::msgpack::object *>(o.zone.allocate_align(sizeof(::msgpack::object) * o.via.array.size));

        o.via.array.ptr[xsockaddr_in_port]   = ::msgpack::object{ message.sin_port, o.zone };
        o.via.array.ptr[xsockaddr_in_addr]   = ::msgpack::object{ message.sin_addr, o.zone };
    }
};

NS_END1
}
NS_END1
