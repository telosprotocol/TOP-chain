// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xcrypto_key_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xnode_id_codec.hpp"
#include "xcommon/xcodec/xmsgpack/xrole_type_codec.hpp"
#include "xdata/xregistration/xregistration_node_info.h"

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xregistration_node_info_field_count{4};
XINLINE_CONSTEXPR std::size_t xregistration_node_info_registration_type{3};
XINLINE_CONSTEXPR std::size_t xregistration_node_info_account_mortgage{2};
XINLINE_CONSTEXPR std::size_t xregistration_node_info_public_key{1};
XINLINE_CONSTEXPR std::size_t xregistration_node_info_is_genesis_node{0};

template <>
struct convert<top::data::registration::xrec_registration_node_info_t> final {
    msgpack::object const & operator()(msgpack::object const & o, top::data::registration::xrec_registration_node_info_t & result) const {
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
        case xregistration_node_info_registration_type: {
            result.m_registration_type = o.via.array.ptr[xregistration_node_info_registration_type].as<top::common::xregistration_type_t>();
            XATTRIBUTE_FALLTHROUGH;
        }
        case xregistration_node_info_account_mortgage: {
            result.m_account_mortgage = o.via.array.ptr[xregistration_node_info_account_mortgage].as<uint64_t>();
            XATTRIBUTE_FALLTHROUGH;
        }
        case xregistration_node_info_public_key: {
            result.m_public_key = o.via.array.ptr[xregistration_node_info_public_key].as<top::xpublic_key_t>();
            XATTRIBUTE_FALLTHROUGH;
        }
        case xregistration_node_info_is_genesis_node: {
            result.is_genesis_node = o.via.array.ptr[xregistration_node_info_is_genesis_node].as<bool>();
            XATTRIBUTE_FALLTHROUGH;
        }
        }

        return o;
    }
};

template <>
struct pack<::top::data::registration::xrec_registration_node_info_t> {
    template <typename StreamT>
    msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, top::data::registration::xrec_registration_node_info_t const & result) const {
        o.pack_array(xregistration_node_info_field_count);
        o.pack(result.is_genesis_node);
        o.pack(result.m_public_key);
        o.pack(result.m_account_mortgage);
        o.pack(result.m_registration_type);
        return o;
    }
};

template <>
struct object_with_zone<top::data::registration::xrec_registration_node_info_t> {
    void operator()(msgpack::object::with_zone & o, top::data::registration::xrec_registration_node_info_t const & result) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xregistration_node_info_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xregistration_node_info_is_genesis_node] = msgpack::object{result.is_genesis_node, o.zone};
        o.via.array.ptr[xregistration_node_info_public_key] = msgpack::object{result.m_public_key, o.zone};
        o.via.array.ptr[xregistration_node_info_account_mortgage] = msgpack::object{result.m_account_mortgage, o.zone};
        o.via.array.ptr[xregistration_node_info_registration_type] = msgpack::object{result.m_registration_type, o.zone};
    }
};

NS_END1
}
NS_END1
