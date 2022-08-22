// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <set>
#include "json/json.h"
#include "xbase/xbase.h"
#include "xutility/xhash.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xint.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"

namespace eth {
using eth_method_handler = std::function<void(const xJson::Value & request, xJson::Value & response)>;
namespace Json = xJson;
class EthMethod {
    std::unordered_map<std::string, eth_method_handler> m_eth_method_map;
    std::set<std::string> m_supported_method;

public:
    EthMethod() {
        m_eth_method_map.emplace(std::make_pair("eth_chainId", std::bind(&EthMethod::eth_chainId, this, std::placeholders::_1, std::placeholders::_2)));
        m_eth_method_map.emplace(std::make_pair("web3_clientVersion", std::bind(&EthMethod::web3_clientVersion, this, std::placeholders::_1, std::placeholders::_2)));
        m_eth_method_map.emplace(std::make_pair("net_version", std::bind(&EthMethod::net_version, this, std::placeholders::_1, std::placeholders::_2)));
        m_eth_method_map.emplace(std::make_pair("eth_gasPrice", std::bind(&EthMethod::eth_gasPrice, this, std::placeholders::_1, std::placeholders::_2)));
        m_eth_method_map.emplace(std::make_pair("web3_sha3", std::bind(&EthMethod::web3_sha3, this, std::placeholders::_1, std::placeholders::_2)));

        m_supported_method.insert("eth_call");
        m_supported_method.insert("eth_estimateGas");
        m_supported_method.insert("eth_getCode");
        m_supported_method.insert("eth_getStorageAt");
        m_supported_method.insert("eth_sendRawTransaction");
        m_supported_method.insert("eth_getLogs");
        m_supported_method.insert("eth_getBalance");
        m_supported_method.insert("eth_blockNumber");
        m_supported_method.insert("eth_getBlockByHash");
        m_supported_method.insert("eth_getBlockByNumber");
        m_supported_method.insert("eth_getTransactionCount");
        m_supported_method.insert("eth_getTransactionByHash");
        m_supported_method.insert("eth_getTransactionReceipt");

        m_supported_method.insert("topRelay_getPolyBlockHashListByHash");
        m_supported_method.insert("topRelay_getLeafBlockHashListByHash");
        m_supported_method.insert("topRelay_getBlockByNumber");
        m_supported_method.insert("topRelay_getBlockByHash");
        m_supported_method.insert("topRelay_blockNumber");
        m_supported_method.insert("topRelay_getTransactionByHash");
        m_supported_method.insert("topRelay_getTransactionReceipt");

        m_supported_method.insert("top_getBalance");
    }
    bool supported_method(const std::string& method) {
        if (m_supported_method.find(method) == m_supported_method.end())
            return false;
        return true;
    }
    int CallMethod(const xJson::Value & req, xJson::Value & res) {
        std::string method = req["method"].asString();
        xJson::Value eth_params = req["params"];
        auto iter = m_eth_method_map.find(method);
        if (iter != m_eth_method_map.end()) {
            xinfo("call method: %s", method.c_str());
            iter->second(eth_params, res);
            return 0;
        } else {
            //xinfo("rpc request eth:CallMethod %s nonexist!", method.c_str());
            return 1;
        }
    }
    inline void eth_chainId(const Json::Value & request, Json::Value & response) {
        uint32_t chain_id = XGET_CONFIG(chain_id);
        std::stringstream outstr;
        outstr << "0x" << std::hex << chain_id;
        Json::Value value = outstr.str();  //"0x3ff";
        response["result"] = value;
    }

    inline void net_version(const Json::Value & request, Json::Value & response) {
        uint32_t chain_id = XGET_CONFIG(chain_id);
        Json::Value value = std::to_string(chain_id); // "1023";
        response["result"] = value;
    }

    inline void web3_clientVersion(const Json::Value & request, Json::Value & response) {
        xdbg("web3_clientVersion1");
        Json::Value value = "Geth/v1.10.17-25c9b49f-20220330/linux-amd64/go1.17.6";
        response["result"] = value;
        //xdbg("web3_clientVersion: %s", response.asString().c_str());
    }

    void eth_gasPrice(const Json::Value & request, Json::Value & response);
    void web3_sha3(const Json::Value & request, Json::Value & response);
};

}  // namespace rpc
