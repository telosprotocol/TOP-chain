#pragma once

#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xnode_address_codec.hpp"
#include "xnetwork/xnode.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xnode_field_count{ 2 };
XINLINE_CONSTEXPR std::size_t xnode_id_index{ 0 };
XINLINE_CONSTEXPR std::size_t xnode_address_index{ 1 };

template <>
struct convert<top::network::xnode_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::network::xnode_t & node) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xnode_field_count <= o.via.array.size) {
            node = top::network::xnode_t
            {
                o.via.array.ptr[xnode_id_index].as<top::common::xnode_id_t>(),
                o.via.array.ptr[xnode_address_index].as<top::network::xnode_endpoint_t>(),
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xnode_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::network::xnode_t const & node) const {
        o.pack_array(xnode_field_count);
        o.pack(node.id());
        o.pack(node.endpoint());

        return o;
    }
};

template <>
struct object_with_zone<top::network::xnode_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xnode_t const & node) const {
        o.type = type::ARRAY;
        o.via.array.size = xnode_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xnode_id_index] = msgpack::object{ node.id(), o.zone };
        o.via.array.ptr[xnode_address_index] = msgpack::object{ node.endpoint(), o.zone };
    }
};

template <>
struct convert<top::network::xdht_node_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::network::xdht_node_t & node) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xnode_field_count <= o.via.array.size) {
            node = top::network::xdht_node_t
            {
                o.via.array.ptr[xnode_id_index].as<top::common::xnode_id_t>(),
                o.via.array.ptr[xnode_address_index].as<top::network::xnode_endpoint_t>(),
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::xdht_node_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::network::xdht_node_t const & node) const {
        o.pack_array(xnode_field_count);
        o.pack(node.id());
        o.pack(node.endpoint());

        return o;
    }
};

template <>
struct object_with_zone<top::network::xdht_node_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::network::xdht_node_t const & node) const {
        o.type = type::ARRAY;
        o.via.array.size = xnode_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xnode_id_index] = msgpack::object{ node.id(), o.zone };
        o.via.array.ptr[xnode_address_index] = msgpack::object{ node.endpoint(), o.zone };
    }
};

NS_END1
}
NS_END1
