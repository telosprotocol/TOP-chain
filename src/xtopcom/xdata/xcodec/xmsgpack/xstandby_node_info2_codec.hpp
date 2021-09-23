// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xcrypto_key_codec.hpp"
#include "xbasic/xcodec/xmsgpack/xsimple_map_result_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xnode_type_codec.hpp"
#include "xdata/xstandby/xstandby_node_info2.h"

#include <msgpack.hpp>

#include <cstdint>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xtop_standby_node_info_field_count{ 4 };
XINLINE_CONSTEXPR std::size_t xtop_standby_node_info_stake_index{ 0 };
XINLINE_CONSTEXPR std::size_t xtop_standby_node_info_public_key_index{ 1 };
XINLINE_CONSTEXPR std::size_t xtop_standby_node_info_is_genesis_node{ 2 };
XINLINE_CONSTEXPR std::size_t xtop_standby_node_info_program_version{ 3 };

// rec:
template <>
struct convert<top::data::standby::xrec_standby_node_info_t> final {
    msgpack::object const & operator()(msgpack::object const & o, top::data::standby::xrec_standby_node_info_t & node_info) const {
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

        case xtop_standby_node_info_program_version: {
            node_info.program_version = o.via.array.ptr[xtop_standby_node_info_program_version].as<std::string>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xtop_standby_node_info_is_genesis_node: {
            node_info.is_genesis_node = o.via.array.ptr[xtop_standby_node_info_is_genesis_node].as<bool>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xtop_standby_node_info_public_key_index: {
            node_info.public_key = o.via.array.ptr[xtop_standby_node_info_public_key_index].as<top::xpublic_key_t>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xtop_standby_node_info_stake_index: {
            node_info.ec_stake = o.via.array.ptr[xtop_standby_node_info_stake_index].as<uint64_t>();
            XATTRIBUTE_FALLTHROUGH;
        }
        }

        return o;
    }
};

template <>
struct pack<::top::data::standby::xrec_standby_node_info_t> {
    template <typename StreamT>
    msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::data::standby::xrec_standby_node_info_t const & node_info) const {
        o.pack_array(xtop_standby_node_info_field_count);
        o.pack(node_info.ec_stake);
        o.pack(node_info.public_key);
        o.pack(node_info.is_genesis_node);
        o.pack(node_info.program_version);

        return o;
    }
};

template <>
struct object_with_zone<::top::data::standby::xrec_standby_node_info_t> {
    void operator()(msgpack::object::with_zone & o, top::data::standby::xrec_standby_node_info_t const & node_info) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xtop_standby_node_info_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(::msgpack::object) * o.via.array.size));
        o.via.array.ptr[xtop_standby_node_info_stake_index] = msgpack::object{node_info.ec_stake, o.zone};
        o.via.array.ptr[xtop_standby_node_info_public_key_index] = msgpack::object{node_info.public_key, o.zone};
        o.via.array.ptr[xtop_standby_node_info_is_genesis_node] = msgpack::object{node_info.is_genesis_node, o.zone};
        o.via.array.ptr[xtop_standby_node_info_program_version] = msgpack::object{node_info.program_version, o.zone};
    }
};

// zec:
template <>
struct convert<top::data::standby::xzec_standby_node_info_t> final {
    msgpack::object const & operator()(msgpack::object const & o, top::data::standby::xzec_standby_node_info_t & node_info) const {
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

        case xtop_standby_node_info_program_version: {
            node_info.program_version = o.via.array.ptr[xtop_standby_node_info_program_version].as<std::string>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xtop_standby_node_info_is_genesis_node: {
            node_info.is_genesis_node = o.via.array.ptr[xtop_standby_node_info_is_genesis_node].as<bool>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xtop_standby_node_info_public_key_index: {
            node_info.public_key = o.via.array.ptr[xtop_standby_node_info_public_key_index].as<top::xpublic_key_t>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xtop_standby_node_info_stake_index: {
            node_info.stake_container = o.via.array.ptr[xtop_standby_node_info_stake_index].as<top::data::standby::xzec_standby_stake_container>();
            XATTRIBUTE_FALLTHROUGH;
        }
        }

        return o;
    }
};

template <>
struct pack<::top::data::standby::xzec_standby_node_info_t> {
    template <typename StreamT>
    msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::data::standby::xzec_standby_node_info_t const & node_info) const {
        o.pack_array(xtop_standby_node_info_field_count);
        o.pack(node_info.stake_container);
        o.pack(node_info.public_key);
        o.pack(node_info.is_genesis_node);
        o.pack(node_info.program_version);

        return o;
    }
};

template <>
struct object_with_zone<::top::data::standby::xzec_standby_node_info_t> {
    void operator()(msgpack::object::with_zone & o, top::data::standby::xzec_standby_node_info_t const & node_info) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xtop_standby_node_info_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(::msgpack::object) * o.via.array.size));
        o.via.array.ptr[xtop_standby_node_info_stake_index] = msgpack::object{node_info.stake_container, o.zone};
        o.via.array.ptr[xtop_standby_node_info_public_key_index] = msgpack::object{node_info.public_key, o.zone};
        o.via.array.ptr[xtop_standby_node_info_is_genesis_node] = msgpack::object{node_info.is_genesis_node, o.zone};
        o.via.array.ptr[xtop_standby_node_info_program_version] = msgpack::object{node_info.program_version, o.zone};
    }
};

NS_END1
}
NS_END1
