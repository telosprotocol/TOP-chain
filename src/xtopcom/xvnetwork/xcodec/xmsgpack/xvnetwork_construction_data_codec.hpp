// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xcluster_address_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xversion_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xvnode_type_codec.hpp"
#include "xvnetwork/xvnetwork_construction_data.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

///////////////////////////////////////////////////////////////////////////////
// xcluster_data_t codec
///////////////////////////////////////////////////////////////////////////////

XINLINE_CONSTEXPR std::size_t xcluster_data_field_count{ 3 };
XINLINE_CONSTEXPR std::size_t xcluster_data_cluster_id_index{ 0 };
XINLINE_CONSTEXPR std::size_t xcluster_data_child_node_type_index{ 1 };
XINLINE_CONSTEXPR std::size_t xcluster_data_children_id_index{ 2 };

template <>
struct convert<top::vnetwork::xcluster_data_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::vnetwork::xcluster_data_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xcluster_data_field_count <= o.via.array.size) {
            v = top::vnetwork::xcluster_data_t
            {
                o.via.array.ptr[xcluster_data_cluster_id_index].as<top::common::xcluster_id_t>(),
                o.via.array.ptr[xcluster_data_child_node_type_index].as<top::common::xnode_type_t>(),
                o.via.array.ptr[xcluster_data_children_id_index].as<std::vector<top::common::xnode_id_t>>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::vnetwork::xcluster_data_t>
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::vnetwork::xcluster_data_t const & message) const {
        o.pack_array(xcluster_data_field_count);

        o.pack(message.cluster_id);
        o.pack(message.child_node_type);
        o.pack(message.children_id);

        return o;
    }
};

template <>
struct object_with_zone<top::vnetwork::xcluster_data_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::vnetwork::xcluster_data_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xcluster_data_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xcluster_data_cluster_id_index]      = msgpack::object{ message.cluster_id,      o.zone };
        o.via.array.ptr[xcluster_data_child_node_type_index] = msgpack::object{ message.child_node_type, o.zone };
        o.via.array.ptr[xcluster_data_children_id_index]     = msgpack::object{ message.children_id,     o.zone };
    }
};

///////////////////////////////////////////////////////////////////////////////
// xzone_data_t codec
///////////////////////////////////////////////////////////////////////////////

XINLINE_CONSTEXPR std::size_t xzone_data_field_count{ 4 };
XINLINE_CONSTEXPR std::size_t xzone_data_version_index{ 0 };
XINLINE_CONSTEXPR std::size_t xzone_data_zone_id_index{ 1 };
XINLINE_CONSTEXPR std::size_t xzone_data_cluster_data_index{ 2 };
XINLINE_CONSTEXPR std::size_t xzone_data_cluster_relationship_index{ 3 };

template <>
struct convert<top::vnetwork::xzone_data_t> final
{
    ::msgpack::object const &
    operator()(::msgpack::object const & o, top::vnetwork::xzone_data_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xzone_data_field_count <= o.via.array.size) {
            v = top::vnetwork::xzone_data_t
            {
                o.via.array.ptr[xzone_data_version_index].as<top::vnetwork::xversion_t>(),
                o.via.array.ptr[xzone_data_zone_id_index].as<top::common::xzone_id_t>(),
                o.via.array.ptr[xzone_data_cluster_data_index].as<std::vector<top::vnetwork::xcluster_data_t>>(),
                o.via.array.ptr[xzone_data_cluster_relationship_index].as<std::unordered_map<top::common::xcluster_id_t, top::common::xcluster_id_t>>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::vnetwork::xzone_data_t>
{
    template <typename Stream>
    ::msgpack::packer<Stream> &
    operator()(::msgpack::packer<Stream> & o, top::vnetwork::xzone_data_t const & message) const {
        o.pack_array(xzone_data_field_count);

        o.pack(message.version);
        o.pack(message.zone_id);
        o.pack(message.cluster_data);
        o.pack(message.cluster_relationship);

        return o;
    }
};

template <>
struct object_with_zone<top::vnetwork::xzone_data_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::vnetwork::xzone_data_t const & message) const {
        o.type = type::ARRAY;

        o.via.array.size = xzone_data_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xzone_data_version_index]              = msgpack::object{ message.version,              o.zone };
        o.via.array.ptr[xzone_data_zone_id_index]              = msgpack::object{ message.zone_id,              o.zone };
        o.via.array.ptr[xzone_data_cluster_data_index]         = msgpack::object{ message.cluster_data,         o.zone };
        o.via.array.ptr[xzone_data_cluster_relationship_index] = msgpack::object{ message.cluster_relationship, o.zone };
    }
};

NS_END1
}
NS_END1
