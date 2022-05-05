// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "eth_method.h"
namespace eth {

void EthMethod::web3_sha3(const Json::Value & request, Json::Value & response) {
    std::string input = request[0].asString();
    if (input.size() < 2) {
        response = "0x";
        return;
    }
    input = top::HexDecode(input.substr(2));
    const top::uint256_t hash_value = top::utl::xkeccak256_t::digest(input.data(), input.size());
    std::string hash_str = std::string((char *)hash_value.data(), (std::string::size_type)hash_value.size());
    response = std::string("0x") + top::HexEncode(hash_str);
    xdbg("web3_sha3: %s", response.asString().c_str());
}

}