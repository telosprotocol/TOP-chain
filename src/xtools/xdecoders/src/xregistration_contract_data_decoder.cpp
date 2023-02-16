// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xregistration_contract_data_decoder.h"

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xbasic/xutility.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xcommon/xnode_type.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <cstdint>
#include <iostream>

NS_BEG4(top, tools, decoders, details)

void xtop_registration_contract_data_decoder::decode(std::string const & input) {
    assert(!input.empty());

    auto bytes = from_hex(input);
    base::xstream_t stream{base::xcontext_t::instance(), bytes.data(), static_cast<uint32_t>(bytes.size())};
    std::uint32_t prefix;
    stream >> prefix;

    std::vector<data::system_contract::xaction_node_info_t> node_slash_info;
    VECTOR_OBJECT_DESERIALZE2(stream, node_slash_info);

    nlohmann::json json;
    for (auto const & value : node_slash_info) {
        auto & account = json[value.node_id.to_string()];
        account["action"] = value.action_type ? "slash credit" : "award credit";
        account["node_type"] = common::to_string(value.node_type);
    }

    std::cout << json.dump() << std::endl;
}

NS_END4
