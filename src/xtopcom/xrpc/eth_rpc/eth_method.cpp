// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "eth_method.h"
#include "xbasic/xhex.h"
#include "eth_error_code.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xgasfee/xgas_estimate.h"
namespace eth {

void EthMethod::web3_sha3(const Json::Value & js_req, Json::Value & js_rsp) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 1))
        return;
    if (!js_req[0].isString()) {
        eth::EthErrorCode::deal_error(js_rsp, enum_eth_rpc_invalid_params, "invalid argument 0: json: cannot unmarshal non-string into Go value of type hexutil.Bytes");
        return;
    }

    if (!eth::EthErrorCode::check_hex(js_req[0].asString(), js_rsp, 0, eth::enum_rpc_type_data))
        return;

    std::string input = js_req[0].asString();
    input = top::HexDecode(input.substr(2));
    const top::uint256_t hash_value = top::utl::xkeccak256_t::digest(input.data(), input.size());
    std::string hash_str = std::string((char *)hash_value.data(), (std::string::size_type)hash_value.size());
    js_rsp["result"] = std::string("0x") + top::HexEncode(hash_str);
    xdbg("web3_sha3: %s", js_rsp["result"].asString().c_str());
}

void EthMethod::eth_gasPrice(const Json::Value & js_req, Json::Value & js_rsp) {
    if (!eth::EthErrorCode::check_req(js_req, js_rsp, 0))
        return;
    //Json::Value value = "0x3b9aca00";
    // Json::Value value = "0x989680";
    auto base_price = top::gasfee::xgas_estimate::base_price();

    std::string baseprice_str = top::to_hex((top::evm_common::h256)base_price);
    uint32_t i = 0;
    for (; i < baseprice_str.size() - 1; i++) {
        if (baseprice_str[i] != '0') {
            break;
        }
    }
/*    std::stringstream outstr;
    uint32_t gas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
    outstr << "0x" << std::hex << gas;
    //Json::Value value = outstr.str();
    Json::Value value = "0x1";*/
    js_rsp["result"] = "0x" + baseprice_str.substr(i);
}
}