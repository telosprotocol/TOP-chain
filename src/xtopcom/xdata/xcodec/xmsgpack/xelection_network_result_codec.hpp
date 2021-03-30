// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xcodec/xmsgpack/xnode_type_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_result_codec.hpp"
#include "xdata/xelection/xelection_network_result.h"

#include <cstdint>
#include <string>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xelection_type_result_field_count{ 1 };
XINLINE_CONSTEXPR std::size_t xelection_type_result_results_index{ 0 };

template <>
struct convert<top::data::election::xelection_network_result_t> final
{
    msgpack::object const &
    operator()(msgpack::object const & o, top::data::election::xelection_network_result_t & result) const {
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

            case xelection_type_result_results_index: {
                result.results(o.via.array.ptr[xelection_type_result_results_index].as<std::map<top::common::xnode_type_t, top::data::election::xelection_result_t>>());
                XATTRIBUTE_FALLTHROUGH;
            }
        }

        return o;
    }
};

template <>
struct pack<::top::data::election::xelection_network_result_t>
{
    template <typename StreamT>
    msgpack::packer<StreamT> &
    operator()(msgpack::packer<StreamT> & o, top::data::election::xelection_network_result_t const & result) const {
        o.pack_array(xelection_type_result_field_count);
        o.pack(result.results());
        return o;
    }
};

template <>
struct object_with_zone<top::data::election::xelection_network_result_t>
{
    void
    operator()(msgpack::object::with_zone & o, top::data::election::xelection_network_result_t const & result) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xelection_type_result_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xelection_type_result_results_index] = msgpack::object{ result.results(), o.zone };
    }
};

NS_END1
}
NS_END1
