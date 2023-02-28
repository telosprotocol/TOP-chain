// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvote_data_decoder.h"

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xbasic/xutility.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>

NS_BEG4(top, tools, decoders, details)

void xtop_vote_data_decoder::decode(std::string const & input) {
    assert(!input.empty());

    auto bytes = from_hex(input);
    base::xstream_t stream{base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size())};
    std::map<std::string, uint64_t> vote_details;
    stream >> vote_details;

    nlohmann::json json;
    for (auto const & vote_datum : vote_details) {
        json[top::get<std::string const>(vote_datum)] = top::get<uint64_t>(vote_datum);
    }

    std::cout << json.dump() << std::endl;
}

NS_END4
