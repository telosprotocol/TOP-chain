#include "xrpc/eth_rpc/eth_error_code.h"
#include "xbase/xbase.h"

namespace eth {
void EthErrorCode::deal_error(xJson::Value & js_rsp, eth::enum_eth_rpc_code error_id, const std::string& msg) {
    js_rsp["error"]["code"] = error_id;
    js_rsp["error"]["message"] = msg;
    return;
}
bool EthErrorCode::check_req(const xJson::Value & js_req, xJson::Value & js_rsp, const uint32_t number) {
    if (js_req.size() < number) {
        std::string msg = std::string("missing value for required argument ") + std::to_string(number - 1);
        deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return false;
    }
    if (js_req.size() > number) {
        std::string msg = std::string("too many arguments, want at most ") + std::to_string(number);
        deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return false;
    }
    return true;
}
bool EthErrorCode::check_hex(const std::string& value, xJson::Value & js_rsp, uint32_t index, const enum_rpc_check_type type) {
    if (value.empty()) {
        std::string msg = std::string("invalid argument ") + std::to_string(index) + ": empty hex string";
        deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return false;
    }
    if (type == enum_rpc_type_block) {
        if (value == "latest" || value == "earliest" || value == "pending")
            return true;
    } else if (type == enum_rpc_type_address) {
        if (value.size() % 2 == 1) {
            std::string msg = std::string("invalid argument ") + std::to_string(index) + ": json: cannot unmarshal hex string of odd length into Go value of type common.Address";
            deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
            return false;
        }
    } else if (type == enum_rpc_type_hash) {
        if (value.size() % 2 == 1) {
            std::string msg = std::string("invalid argument ") + std::to_string(index) + ": json: cannot unmarshal hex string of odd length into Go value of type common.Hash";
            deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
            return false;
        }
    } else if (type == enum_rpc_type_data) {
        if (value.size() % 2 == 1) {
            std::string msg = std::string("invalid argument ") + std::to_string(index) + ": json: cannot unmarshal hex string of odd length into Go value of type hexutil.Bytes";
            deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
            return false;
        }
    } else if (type == enum_rpc_type_topic) {
        if (value.size() % 2 == 1) {
            std::string msg = std::string("invalid argument ") + std::to_string(index) + ": json: cannot unmarshal hex string of odd length";
            deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
            return false;
        }
        if (value.size() != 66) {
            std::string msg = std::string("invalid argument ") + std::to_string(index) + ": hex has invalid length " + std::to_string(value.size()/2 - 1) +
                " after decoding; expected 32 for topic";
            xinfo("check_topic fail: %s", value.c_str());
            deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
            return false;
        }
    }
    if (value.substr(0, 2) != "0x" && value.substr(0, 2) != "0X") {
        std::string msg = "invalid argument " + std::to_string(index) + ": hex string without 0x prefix";
        deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return false;
    }
    if (type == enum_rpc_type_block) {
        if (value.size() >= 4 && value[2] == '0') {
            std::string msg = "invalid argument " + std::to_string(index) + ": hex number with leading zero digits";
            deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
            return false;
        }
    }
    return true;
}
bool EthErrorCode::check_eth_address(const std::string& account, xJson::Value & js_rsp) {
    if (account.size() != 42) {
        std::string msg = std::string("invalid argument: hex string has length ") + std::to_string(account.size()-2) + ", want 40 for common.Address";
        xinfo("check_eth_address fail: %s", account.c_str());
        deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return false;
    }
    return true;
}
bool EthErrorCode::check_hash(const std::string& hash, xJson::Value & js_rsp) {
    if (hash.size() != 66) {
        std::string msg = std::string("invalid argument 0: hex string has length ") + std::to_string(hash.size()-2) + ", want 64 for common.Hash";
        deal_error(js_rsp, eth::enum_eth_rpc_invalid_params, msg);
        return false;
    }
    return true;
}
}