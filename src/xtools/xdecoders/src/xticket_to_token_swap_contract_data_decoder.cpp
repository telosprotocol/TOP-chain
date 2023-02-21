// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xticket_to_token_swap_contract_data_decoder.h"

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xbasic/xutility.h"

#include <json/json.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>

NS_BEG4(top, tools, decoders, details)

void xtop_ticket_to_token_swap_contract_data_decoder::decode(std::string const & input) {
    assert(!input.empty());

    auto bytes = from_hex(input);
    base::xstream_t stream{base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size())};
    uint64_t vote_amount;
    stream >> vote_amount;

    nlohmann::json json;
    json["vote_num"] = vote_amount;

    std::cout << json.dump() << std::endl;
}

NS_END4
