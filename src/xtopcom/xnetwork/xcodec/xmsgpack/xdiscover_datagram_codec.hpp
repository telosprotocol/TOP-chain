// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xhash_codec.hpp"
#include "xcodec/xcodec_error.h"
#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xnode_address_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xnode_codec.hpp"
#include "xnetwork/xp2p/xdiscover_datagram.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xping_node_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xping_node_target_node_index{ 0 };
//XINLINE_CONSTEXPR std::uint16_t xping_node_app_port_index{ 1 };

template <>
struct convert<top::network::p2p::xping_node_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o,
               top::network::p2p::xping_node_t & node) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xping_node_field_count <= o.via.array.size) {
            node = top::network::p2p::xping_node_t
            {
                o.via.array.ptr[xping_node_target_node_index].as<top::network::xdht_node_t>()// ,
                // o.via.array.ptr[xping_node_app_port_index].as<std::uint16_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::p2p::xping_node_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::network::p2p::xping_node_t const & node) const {
        o.pack_array(xping_node_field_count);
        o.pack(node.target_node());
        //o.pack(node.app_port());

        return o;
    }
};

template <>
struct object_with_zone<top::network::p2p::xping_node_t>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::network::p2p::xping_node_t const & node) const {
        o.type = type::ARRAY;
        o.via.array.size = xping_node_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xping_node_target_node_index] = msgpack::object{ node.target_node(), o.zone };
        // o.via.array.ptr[xping_node_app_port_index] = msgpack::object{ node.app_port(), o.zone };
    }
};

XINLINE_CONSTEXPR std::size_t xpong_field_count{ 2 };
XINLINE_CONSTEXPR std::size_t xpong_ping_address_index{ 0 };
XINLINE_CONSTEXPR std::size_t xpong_ping_hash_index{ 1 };
// XINLINE_CONSTEXPR std::size_t xpong_app_port_index{ 2 };

template <>
struct convert<top::network::p2p::xpong_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o,
               top::network::p2p::xpong_t & node) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xpong_field_count <= o.via.array.size) {
            node = top::network::p2p::xpong_t
            {
                o.via.array.ptr[xpong_ping_address_index].as<top::network::xnode_endpoint_t>(),
                o.via.array.ptr[xpong_ping_hash_index].as<top::xhash256_t>(),
                // o.via.array.ptr[xpong_app_port_index].as<std::uint16_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::p2p::xpong_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::network::p2p::xpong_t const & node) const {
        o.pack_array(xpong_field_count);
        o.pack(node.ping_endpoint());
        o.pack(node.ping_hash());
        // o.pack(node.app_port());

        return o;
    }
};

template <>
struct object_with_zone<top::network::p2p::xpong_t>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::network::p2p::xpong_t const & node) const {
        o.type = type::ARRAY;
        o.via.array.size = xpong_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xpong_ping_address_index] = msgpack::object{ node.ping_endpoint() , o.zone };
        o.via.array.ptr[xpong_ping_hash_index] = msgpack::object{ node.ping_hash(), o.zone };
        // o.via.array.ptr[xpong_app_port_index] = msgpack::object{ node.app_port(), o.zone };
    }
};

XINLINE_CONSTEXPR std::size_t xfind_node_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xfind_node_target_id_index{ 0 };

template <>
struct convert<top::network::p2p::xfind_node_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o,
               top::network::p2p::xfind_node_t & node) const {
        if (o.type != msgpack::type::ARRAY) {
            assert(false);
            throw msgpack::type_error{};
        }

        if (xfind_node_field_count <= o.via.array.size) {
            try {
                node = top::network::p2p::xfind_node_t
                {
                    o.via.array.ptr[xfind_node_target_id_index].as<top::common::xnode_id_t>()
                };
            } catch (top::codec::xcodec_error_t const & eh) {
                std::printf("decode xfind_node_t %s\n", eh.what());
                std::fflush(stdout);
                assert(false);
                throw;
            } catch (msgpack::type_error const &) {
                std::printf("decode xfind_node_t type_error\n");
                std::fflush(stdout);
                assert(false);
                throw;
            }
        } else {
            std::printf("decode xfind_node_t size error\n");
            std::fflush(stdout);
            assert(false);
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::p2p::xfind_node_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::network::p2p::xfind_node_t const & node) const {
        o.pack_array(xfind_node_field_count);
        o.pack(node.target_id());

        return o;
    }
};

template <>
struct object_with_zone<top::network::p2p::xfind_node_t>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::network::p2p::xfind_node_t const & node) const {
        o.type = type::ARRAY;
        o.via.array.size = xfind_node_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xfind_node_target_id_index] = msgpack::object{ node.target_id(), o.zone };
    }
};

XINLINE_CONSTEXPR std::size_t xneighbors_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xneighbors_neighbors_index{ 0 };

template <>
struct convert<top::network::p2p::xneighbors_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o,
               top::network::p2p::xneighbors_t & node) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xneighbors_field_count <= o.via.array.size) {
            node = top::network::p2p::xneighbors_t
            {
                o.via.array.ptr[xneighbors_neighbors_index].as<std::unordered_set<top::network::xdht_node_t>>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::network::p2p::xneighbors_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o,
               top::network::p2p::xneighbors_t const & node) const {
        o.pack_array(xneighbors_field_count);
        o.pack(node.neighbors());

        return o;
    }
};

template <>
struct object_with_zone<top::network::p2p::xneighbors_t>
{
    void
    operator()(msgpack::object::with_zone & o,
               top::network::p2p::xneighbors_t const & node) const {
        o.type = type::ARRAY;
        o.via.array.size = xneighbors_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));
        o.via.array.ptr[xneighbors_neighbors_index] = msgpack::object{ node.neighbors(), o.zone };
    }
};

NS_END1
}
NS_END1
