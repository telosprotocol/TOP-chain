// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
#include "xcommon/xsharding_info.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xsharding_info_field_count{ 4 };
XINLINE_CONSTEXPR std::size_t xsharding_info_nid_index{ 0 };
XINLINE_CONSTEXPR std::size_t xsharding_info_zid_index{ 1 };
XINLINE_CONSTEXPR std::size_t xsharding_info_cid_index{ 2 };
XINLINE_CONSTEXPR std::size_t xsharding_info_gid_index{ 3 };

template <>
struct convert<top::common::xsharding_info_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xsharding_info_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (xsharding_info_field_count <= o.via.array.size) {
            v = top::common::xsharding_info_t
            {
                o.via.array.ptr[xsharding_info_nid_index].as<top::common::xnetwork_id_t>(),
                o.via.array.ptr[xsharding_info_zid_index].as<top::common::xzone_id_t>(),
                o.via.array.ptr[xsharding_info_cid_index].as<top::common::xcluster_id_t>(),
                o.via.array.ptr[xsharding_info_gid_index].as<top::common::xgroup_id_t>()
            };
        } else {
            throw msgpack::type_error{};
        }

        return o;
    }
};

template <>
struct pack<top::common::xsharding_info_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::common::xsharding_info_t const & message) const {
        o.pack_array(xsharding_info_field_count);
        o.pack(message.network_id());
        o.pack(message.zone_id());
        o.pack(message.cluster_id());
        o.pack(message.group_id());

        return o;
    }
};

template <>
struct object_with_zone<top::common::xsharding_info_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::common::xsharding_info_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xsharding_info_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xsharding_info_nid_index] = msgpack::object{ message.network_id(), o.zone };
        o.via.array.ptr[xsharding_info_zid_index] = msgpack::object{ message.zone_id(),    o.zone };
        o.via.array.ptr[xsharding_info_cid_index] = msgpack::object{ message.cluster_id(), o.zone };
        o.via.array.ptr[xsharding_info_gid_index] = msgpack::object{ message.group_id(),   o.zone };
    }
};

NS_END1
}
NS_END1
