// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xproper_fraction.hpp"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
NS_BEG1(adaptor)

XINLINE_CONSTEXPR std::size_t xproper_fraction_field_count{2};
XINLINE_CONSTEXPR std::size_t xproper_fraction_m_den{1};
XINLINE_CONSTEXPR std::size_t xproper_fraction_m_num{0};

template <typename IntergerT, typename ValueT>
struct convert<::top::xproper_fraction_t<IntergerT, ValueT>> final {
    msgpack::object const & operator()(msgpack::object const & o, ::top::xproper_fraction_t<IntergerT, ValueT> & result) const {
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
        case xproper_fraction_m_den: {
            result = ::top::xproper_fraction_t<IntergerT, ValueT>(0, o.via.array.ptr[xproper_fraction_m_den].as<IntergerT>());
            XATTRIBUTE_FALLTHROUGH;
        }
        case xproper_fraction_m_num: {
            result.set_num(o.via.array.ptr[xproper_fraction_m_num].as<IntergerT>());
            XATTRIBUTE_FALLTHROUGH;
        }
        }

        return o;
    }
};

template <typename IntergerT, typename ValueT>
struct pack<::top::xproper_fraction_t<IntergerT, ValueT>> {
    template <typename StreamT>
    msgpack::packer<StreamT> & operator()(msgpack::packer<StreamT> & o, ::top::xproper_fraction_t<IntergerT, ValueT> const & result) const {
        o.pack_array(xproper_fraction_field_count);
        o.pack(result.num());
        o.pack(result.den());
        return o;
    }
};

template <typename IntergerT, typename ValueT>
struct object_with_zone<::top::xproper_fraction_t<IntergerT, ValueT>> {
    void operator()(msgpack::object::with_zone & o, ::top::xproper_fraction_t<IntergerT, ValueT> const & result) const {
        o.type = msgpack::type::ARRAY;
        o.via.array.size = xproper_fraction_field_count;
        o.via.array.ptr = static_cast<msgpack::object *>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size));

        o.via.array.ptr[xproper_fraction_m_num] = msgpack::object{result.num(), o.zone};
        o.via.array.ptr[xproper_fraction_m_den] = msgpack::object{result.den(), o.zone};
    }
};

NS_END1
}
NS_END1
