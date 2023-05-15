// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xtop_event.h"

#include "ethash/keccak.hpp"
#include "xbasic/xhex.h"
#include "xcommon/xeth_address.h"
#include "xcommon/xtop_log.h"

#include <string>
#include <vector>

NS_BEG2(top, common)
xh256_t handle_data(const argument_t & arg);
xh256_t handle_indexed(const argument_t & arg);
xh256_t to_keccak256_32bytes(const std::string & data);
xh256_t to_mapping_top_addr(const xaccount_address_t & top_addr);

xtop_log_t xtop_event_t::pack() {
    common::xtop_log_t log{};
    log.address = address;
    // log topics[0]
    log.topics.emplace_back(xh256_t(to_keccak256_32bytes(name)));
    for (auto const & arg : inputs) {
        if (true == arg.indexed) {
            log.topics.emplace_back(handle_indexed(arg));
        } else {
            log.data = handle_data(arg).to_bytes();
        }
    }
    return log;
}

xh256_t handle_indexed(const argument_t & arg) {
    xh256_t ret = xh256_t{};
    switch (arg.type) {
    case enum_event_data_type::string: {
        ret = xh256_t{to_keccak256_32bytes(arg.str_data)};
        break;
    }
    case enum_event_data_type::uint64: {
        ret = xh256_t(arg.uint64_t_data);
        break;
    }
    case enum_event_data_type::address: {
        auto top_addr = common::xaccount_address_t::build_from(arg.str_data);
        if (top_addr.type() == base::enum_vaccount_addr_type_secp256k1_eth_user_account || top_addr.type() == base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
            auto eth_addr = common::xtop_eth_address::build_from(top_addr);
            ret = xh256_t(eth_addr.to_h256());
        } else {
            ret = xh256_t{to_mapping_top_addr(top_addr)};
        }
        break;
    }
    default:
        xwarn("[xtop_event_t::handle_indexed] invalid type");
        break;
    }
    return ret;
}

xh256_t handle_data(const argument_t & arg) {
    xh256_t ret = xh256_t{};
    switch (arg.type) {
    case enum_event_data_type::string: {
        auto get_append_len = [](int data_len) -> int {
            if (data_len <= 32) {
                return 32 - data_len;
            }
            return 32 - (data_len % 32);
        };
        unsigned int str_data_size = arg.str_data.size();
        xbytes_t head = xh256_t{32}.to_bytes();
        xbytes_t len = xh256_t{str_data_size}.to_bytes();
        xbytes_t data = top::to_bytes(arg.str_data);
        evm_common::append(head, len);
        evm_common::append(head, data);
        evm_common::append(head, xbytes_t(get_append_len(data.size()), 0));
        ret = xh256_t{head};
        break;
    }
    case enum_event_data_type::uint64: {
        ret = xh256_t(arg.uint64_t_data);
        break;
    }
    case enum_event_data_type::address: {
        auto top_addr = common::xaccount_address_t::build_from(arg.str_data);
        if (top_addr.type() == base::enum_vaccount_addr_type_secp256k1_eth_user_account || top_addr.type() == base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
            auto eth_addr = common::xtop_eth_address::build_from(top_addr);
            ret = xh256_t{eth_addr.to_h256()};
        } else {
            ret = to_mapping_top_addr(top_addr);
        }
        break;
    }
    default:
        xwarn("[xtop_event_t::handle_data] invalid type");
        break;
    }
    return ret;
}

xh256_t to_mapping_top_addr(const xaccount_address_t & top_addr) {
    // addr = ML7oBZbitBCcXhrJwqBhha2MUimd6SM9Z6
    // hex("rJwqBhha2MUimd6SM9Z6") -> 724a777142686861324d55696d6436534d395a36
    auto base = top_addr.base_address().to_string().substr(6);
    auto size = base.size();
    std::string new_addr;
    if (size >= 20) {
        new_addr = base.substr(size - 20);
    }
    auto ret = xbytes_t(12, 0);
    evm_common::append(ret, to_bytes(new_addr));
    return xh256_t{ret};
}

xh256_t to_keccak256_32bytes(const std::string & data) {
    auto b = top::to_bytes(data);
    auto v = ethash::keccak256(b.data(), b.size());
    return xh256_t{xbytes_t{&v.bytes[0], &v.bytes[32]}};
}

NS_END2