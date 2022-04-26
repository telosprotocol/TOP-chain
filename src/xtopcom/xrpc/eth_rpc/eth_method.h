// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <unordered_map>
#include <iostream>
#include <functional>
#include <string>
#include <set>
#include "json/json.h"
#include "xbase/xbase.h"

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
        m_eth_method_map.emplace(std::make_pair("eth_estimateGas", std::bind(&EthMethod::eth_estimateGas, this, std::placeholders::_1, std::placeholders::_2)));

        m_supported_method.insert("eth_call");
        m_supported_method.insert("eth_estimateGas");
        m_supported_method.insert("eth_getCode");
        m_supported_method.insert("eth_getStorageAt");
        m_supported_method.insert("eth_sendRawTransaction");
        m_supported_method.insert("Eth_getLogs");
        m_supported_method.insert("web3_sha3");
        m_supported_method.insert("eth_getBalance");
        m_supported_method.insert("eth_blockNumber");
        m_supported_method.insert("eth_getBlockByHash");
        m_supported_method.insert("eth_getBlockByNumber");
        m_supported_method.insert("eth_getTransactionCount");
        m_supported_method.insert("eth_getTransactionByHash");
        m_supported_method.insert("eth_getTransactionReceipt");
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
        response = "0x3ff";
    }

    inline void net_version(const Json::Value & request, Json::Value & response) {
        response = "0x457";
    }

    inline void web3_clientVersion(const Json::Value & request, Json::Value & response) {
        response = "Geth/v1.10.17-25c9b49f-20220330/linux-amd64/go1.17.6";
        xinfo("web3_clientVersion: %s", response.asString().c_str());
    }

    inline void eth_gasPrice(const Json::Value & request, Json::Value & response) {
        response = "0x3b9aca00";
    }

    inline void eth_estimateGas(const Json::Value & request, Json::Value & response) {
        response = "0x5208";
    }

};

}  // namespace rpc
