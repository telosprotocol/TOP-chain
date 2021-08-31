// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xcrypto_key_codec.hpp"
#include "xbasic/xcodec/xmsgpack/xversion_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xnode_type_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_node_info_codec.hpp"
#include "xdata/xelection/xelection_info.h"

#include <msgpack.hpp>

#include <cstdint>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    NS_BEG1(adaptor)

    XINLINE_CONSTEXPR std::size_t xelection_info_field_count{4};
    // XINLINE_CONSTEXPR std::size_t xelection_info_standby_node_info_index{0};
    XINLINE_CONSTEXPR std::size_t xelection_info_joined_version_index{0};
    XINLINE_CONSTEXPR std::size_t xelection_info_stake_index{1};
    XINLINE_CONSTEXPR std::size_t xelection_info_comprehensive_stake_index{2};
    XINLINE_CONSTEXPR std::size_t xelection_info_consensus_public_key_index{3};

    template <>
    struct convert<top::data::election::xelection_info_t> final {
        msgpack::object const & operator()(msgpack::object const & o, top::data::election::xelection_info_t & election_info) const {
            if (o.type != msgpack::type::ARRAY) {
                throw msgpack::type_error{};
            }

            if (o.via.array.size == 0) {
                return o;
            }

            switch (o.via.array.size - 1) {
            default: {
                XATTRIBUTE_FALLTHROUGH;
            }

            case xelection_info_consensus_public_key_index: {
                election_info.consensus_public_key = o.via.array.ptr[xelection_info_consensus_public_key_index].as<top::xpublic_key_t>();
                XATTRIBUTE_FALLTHROUGH;
            }

            case xelection_info_comprehensive_stake_index: {
                election_info.comprehensive_stake = o.via.array.ptr[xelection_info_comprehensive_stake_index].as<uint64_t>();
                XATTRIBUTE_FALLTHROUGH;
            }

            case xelection_info_stake_index: {
                election_info.stake = o.via.array.ptr[xelection_info_stake_index].as<uint64_t>();
                XATTRIBUTE_FALLTHROUGH;
            }

            case xelection_info_joined_version_index: {
                election_info.joined_version = o.via.array.ptr[xelection_info_joined_version_index].as<top::common::xelection_round_t>();
                XATTRIBUTE_FALLTHROUGH;
            }
            }
            return o;
        }
    };

    template <>
    struct pack<::top::data::election::xelection_info_t> {
        template <typename StreamT>
        msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::data::election::xelection_info_t const & election_info) const {
            o.pack_array(xelection_info_field_count);
            o.pack(election_info.joined_version);
            o.pack(election_info.stake);
            o.pack(election_info.comprehensive_stake);
            o.pack(election_info.consensus_public_key);

            return o;
        }
    };

    template <>
    struct object_with_zone<::top::data::election::xelection_info_t> {
        void operator()(msgpack::object::with_zone & o, top::data::election::xelection_info_t const & election_info) const {
            o.type = msgpack::type::ARRAY;
            o.via.array.size = xelection_info_field_count;
            o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(::msgpack::object) * o.via.array.size));
            o.via.array.ptr[xelection_info_joined_version_index] = msgpack::object{election_info.joined_version, o.zone};
            o.via.array.ptr[xelection_info_stake_index] = msgpack::object{election_info.stake, o.zone};
            o.via.array.ptr[xelection_info_comprehensive_stake_index] = msgpack::object{election_info.comprehensive_stake, o.zone};
            o.via.array.ptr[xelection_info_consensus_public_key_index] = msgpack::object{election_info.consensus_public_key, o.zone};
        }
    };

    NS_END1
}
NS_END1
