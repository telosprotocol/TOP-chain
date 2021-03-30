#pragma once

#include "xnetwork/xendpoint.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xendpoint_field_count{ 2 };
XINLINE_CONSTEXPR std::size_t xendpoint_address_index{ 0 };
XINLINE_CONSTEXPR std::size_t xendpoint_port_index{ 1 };

template <>
struct convert<top::network::xendpoint_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::network::xendpoint_t & endpoint) const
    {
        if (o.type != msgpack::type::ARRAY)
        {
            throw msgpack::type_error{};
        }

        if (xendpoint_field_count <= o.via.array.size)
        {
            endpoint = top::network::xendpoint_t
            {
                o.via.array.ptr[xendpoint_address_index].as<std::string>(),
                o.via.array.ptr[xendpoint_port_index].as<std::uint16_t>()
            };
        }
        else
        {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xendpoint_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::network::xendpoint_t const & endpoint) const
    {
        o.pack_array(xendpoint_field_count);
        o.pack(endpoint.address());
        o.pack(endpoint.port());

        return o;
    }
};

template <>
struct object_with_zone<top::network::xendpoint_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xendpoint_t const & endpoint) const
    {
        o.type = type::ARRAY;
        o.via.array.size = xendpoint_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xendpoint_address_index] = msgpack::object{ endpoint.address(), o.zone };
        o.via.array.ptr[xendpoint_port_index] = msgpack::object{ endpoint.port(), o.zone };
    }
};

NS_END1
}
NS_END1
