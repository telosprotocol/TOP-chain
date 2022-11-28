// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsafebox/safebox_http_client.h"

#include <iostream>
#include <sstream>

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

NS_BEG2(top, safebox)

SafeBoxHttpClient::SafeBoxHttpClient(std::string const & local_host) : base_t{local_host} {
}

std::string SafeBoxHttpClient::request_account(std::string const & account) {
    auto response_str = request_get(account);
    json response;
    try {
        response = json::parse(response_str);
    } catch (json::parse_error & e) {
        std::cout << "json parse failed" << std::endl;
        return "";
    }
    if (response.find("account") == response.end()) {
        return "";
    }
    return response["account"].get<std::string>();
}

std::string SafeBoxHttpClient::request_prikey(std::string const & account) {
    auto response_str = request_get(account);
    json response;
    try {
        response = json::parse(response_str);
    } catch (json::parse_error & e) {
        std::cout << "json parse failed" << std::endl;
        return "";
    }
    if (response.find("private_key") == response.end()) {
        return "";
    }
    return response["private_key"].get<std::string>();
}

std::pair<std::string, std::string> SafeBoxHttpClient::request_account_prikey() {
    auto response_str = request_get();
    json response;
    try {
        response = json::parse(response_str);
    } catch (json::parse_error & e) {
        std::cout << "json parse failed" << std::endl;
        return std::make_pair("", "");
    }
    if (response.find("account") == response.end() || response.find("private_key") == response.end()) {
        return std::make_pair("", "");
    }
    return std::make_pair(response["account"].get<std::string>(), response["private_key"].get<std::string>());
}

std::string SafeBoxHttpClient::request_get(std::string const & account) {
    std::string path = "/api/safebox";
    json body = json::object();
    body["method"] = "get";
    if (!account.empty()) {
        body["account"] = account;
    }
    std::string result;
    try {
        result = request_post_json(path, body.dump());
    } catch (std::exception const & e) {
        std::cout << "catch exception:" << e.what() << std::endl;
        return "";
    }
    return result;
}

bool SafeBoxHttpClient::set_prikey(std::string const & account, std::string const & pri_key, std::ostringstream & out_str, uint32_t expired_time) {
    std::string path = "/api/safebox";
    json body = json::object();
    body["method"] = "set";
    body["account"] = account;
    body["private_key"] = pri_key;
    body["expired_time"] = expired_time;

    std::string result;
    try {
        auto response = request_post_json(path, body.dump());
        auto response_json = json::parse(response);
        if ((response_json["status"].get<std::string>()) == "fail") {
            out_str << (response_json["error"].get<std::string>()) << std::endl;
            return false;
        }
    } catch (json::parse_error & e) {
        out_str << "json parse error" << e.what() << std::endl;
        std::cout << "json parse failed" << std::endl;
        return false;
    } catch (std::exception const & e) {
        out_str << e.what() << std::endl;
        std::cout << "catch exception:" << e.what() << std::endl;
        return false;
    }
    return true;
}

NS_END2