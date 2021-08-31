// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xversion_codec.hpp"
#include "xcommon/xaddress.h"
#include "xcommon/xcodec/xmsgpack/xaccount_election_address_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xcluster_address_codec.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xnode_address_field_count{ 5 };
XINLINE_CONSTEXPR std::size_t xnode_address_cluster_address_index{ 0 };
XINLINE_CONSTEXPR std::size_t xnode_address_account_election_address_index{ 1 };
XINLINE_CONSTEXPR std::size_t xnode_address_election_round_index{ 2 };
XINLINE_CONSTEXPR std::size_t xnode_address_sharding_size_index{ 3 };
XINLINE_CONSTEXPR std::size_t xnode_address_associated_blk_height_index{ 4 };

template <>
struct convert<top::common::xnode_address_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::common::xnode_address_t & v) const {
        if (o.type != msgpack::type::ARRAY) {
            throw msgpack::type_error{};
        }

        if (o.via.array.size == 0) {
            return o;
        }

        top::common::xelection_round_t election_round;
        top::common::xaccount_election_address_t account_election_address;
        top::common::xsharding_address_t sharding_address;
        uint16_t sharding_size{ std::numeric_limits<std::uint16_t>::max() };
        uint64_t associated_blk_height{ std::numeric_limits<std::uint64_t>::max() };
        switch (o.via.array.size - 1) {
            default: {
                XATTRIBUTE_FALLTHROUGH;
            }

            case xnode_address_associated_blk_height_index: {
                associated_blk_height = o.via.array.ptr[xnode_address_associated_blk_height_index].as<uint64_t>();
            }

            case xnode_address_sharding_size_index: {
                sharding_size = o.via.array.ptr[xnode_address_sharding_size_index].as<uint16_t>();
            }

            case xnode_address_election_round_index: {
                election_round = o.via.array.ptr[xnode_address_election_round_index].as<top::common::xelection_round_t>();
            }

            case xnode_address_account_election_address_index: {
                account_election_address = o.via.array.ptr[xnode_address_account_election_address_index].as<top::common::xaccount_election_address_t>();
            }

            case xnode_address_cluster_address_index: {
                sharding_address = o.via.array.ptr[xnode_address_cluster_address_index].as<top::common::xcluster_address_t>();
            }
        }

        if (!account_election_address.empty() && !election_round.empty()) {
            v = top::common::xnode_address_t{
                std::move(sharding_address),
                std::move(account_election_address),
                std::move(election_round),
                sharding_size,
                associated_blk_height
            };
        } else if (account_election_address.empty() && election_round.empty()) {
            v = top::common::xnode_address_t{
                std::move(sharding_address)
            };
        } else if (election_round.empty()) {
            v = top::common::xnode_address_t{
                std::move(sharding_address),
                std::move(account_election_address)
            };
        } else {
            v = top::common::xnode_address_t{
                std::move(sharding_address),
                std::move(election_round),
                sharding_size,
                associated_blk_height
            };
        }

        return o;
    }
};

template <>
struct pack<top::common::xnode_address_t>
{
    template <typename Stream>
    msgpack::packer<Stream> &
    operator()(msgpack::packer<Stream> & o, top::common::xnode_address_t const & message) const {
        o.pack_array(xnode_address_field_count);
        o.pack(message.cluster_address());
        o.pack(message.account_election_address());
        o.pack(message.election_round());
        o.pack(message.group_size());
        o.pack(message.associated_blk_height());

        return o;
    }
};

template <>
struct object_with_zone<top::common::xnode_address_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::common::xnode_address_t const & message) const {
        o.type = type::ARRAY;
        o.via.array.size = xnode_address_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xnode_address_cluster_address_index] = msgpack::object{ message.cluster_address(), o.zone };
        o.via.array.ptr[xnode_address_account_election_address_index] = msgpack::object{ message.account_election_address(), o.zone };
        o.via.array.ptr[xnode_address_election_round_index] = msgpack::object{ message.election_round(), o.zone };
        o.via.array.ptr[xnode_address_sharding_size_index] = msgpack::object{ message.group_size(), o.zone };
        o.via.array.ptr[xnode_address_associated_blk_height_index] = msgpack::object{ message.associated_blk_height(), o.zone };
    }
};

NS_END1
}
NS_END1
