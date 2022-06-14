// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xcodec/xmsgpack/xaccount_address_codec.hpp"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/xcodec/xmsgpack/xboost_u256_codec.h"

#include <msgpack.hpp>

NS_BEG1(msgpack)
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    NS_BEG1(adaptor)

#if !defined(MSGPACK_USE_CPP03)

    template <>
    struct as<top::data::system_contract::xallowance_t> {
        top::data::system_contract::xallowance_t operator()(msgpack::object const & o) const {
            return top::data::system_contract::xallowance_t{o.as<top::data::system_contract::xallowance_t::data_type>()};
        }
    };

#endif  // !defined (MSGPACK_USE_CPP03)

    template <>
    struct convert<top::data::system_contract::xallowance_t> final {
        msgpack::object const & operator()(msgpack::object const & o, top::data::system_contract::xallowance_t & allowance) const {
            top::data::system_contract::xallowance_t::data_type allowance_data;
            msgpack::adaptor::convert<top::data::system_contract::xallowance_t::data_type>()(o, allowance_data);
            allowance = top::data::system_contract::xallowance_t{std::move(allowance_data)};

            return o;
        }
    };

    template <>
    struct pack<top::data::system_contract::xallowance_t> {
        template <typename Stream>
        msgpack::packer<Stream> & operator()(msgpack::packer<Stream> & o, top::data::system_contract::xallowance_t const & allowance) const {
            o.pack(allowance.raw_data());

            return o;
        }
    };

    NS_END1
}
NS_END1
