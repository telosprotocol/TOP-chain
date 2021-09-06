// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xdata/xparachain/xparachain_chain_info.h"

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xparachain_chain_info_field_count{ 3 };
XINLINE_CONSTEXPR std::size_t xparachain_chain_info_genesis_node_info{ 2 };
XINLINE_CONSTEXPR std::size_t xparachain_chain_info_chain_name{ 1 };
XINLINE_CONSTEXPR std::size_t xparachain_chain_info_chain_id{ 0 };

template <>
struct convert<top::data::parachain::xparachain_chain_info_t> final {
    msgpack::object const & operator()(msgpack::object const & o, top::data::parachain::xparachain_chain_info_t & result) const {
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

        case xparachain_chain_info_genesis_node_info: {
            result.genesis_node_info = o.via.array.ptr[xparachain_chain_info_genesis_node_info].as<std::vector<top::common::xnode_id_t>>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xparachain_chain_info_chain_name: {
            result.chain_name = o.via.array.ptr[xparachain_chain_info_chain_name].as<std::string>();
            XATTRIBUTE_FALLTHROUGH;
        }

        case xparachain_chain_info_chain_id: {
            result.chain_id = o.via.array.ptr[xparachain_chain_info_chain_id].as<uint32_t>();
            XATTRIBUTE_FALLTHROUGH;
        }
        }

        return o;
    }
};

template <>
struct pack<::top::data::parachain::xparachain_chain_info_t> {
    template <typename StreamT>
    msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::data::parachain::xparachain_chain_info_t const & result) const {
        o.pack_array(xparachain_chain_info_field_count);
        o.pack(result.chain_id);
        o.pack(result.chain_name);
        o.pack(result.genesis_node_info);
        return o;
    }
};

template <>
struct object_with_zone<top::data::parachain::xparachain_chain_info_t> {
    void operator()(msgpack::object::with_zone & o, top::data::parachain::xparachain_chain_info_t const & result) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xparachain_chain_info_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xparachain_chain_info_chain_id] = msgpack::object{result.chain_id, o.zone};
        o.via.array.ptr[xparachain_chain_info_chain_name] = msgpack::object{result.chain_name, o.zone};
        o.via.array.ptr[xparachain_chain_info_genesis_node_info] = msgpack::object{result.genesis_node_info, o.zone};
    }
};

NS_END1
}
NS_END1
