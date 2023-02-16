// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xtoken_to_ticket_swap_contract_data_decoder.h"

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xbasic/xutility.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>

NS_BEG4(top, tools, decoders, details)

void xtop_token_to_ticket_swap_contract_data_decoder::decode(std::string const & input) {
    assert(!input.empty());

    auto bytes = from_hex(input);
    base::xstream_t stream{base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size())};
    uint64_t vote_amount;
    uint16_t lock_duration;
    stream >> vote_amount;
    stream >> lock_duration;

    nlohmann::json json;
    json["vote_num"] = vote_amount;
    json["duration"] = lock_duration;

    std::cout << json.dump() << std::endl;
}

NS_END4
