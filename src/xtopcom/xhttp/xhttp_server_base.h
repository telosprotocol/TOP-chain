// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "xbase/xns_macro.h"

#include <functional>
#include <memory>
#include <string>

NS_BEG2(top, xhttp)

class HttpServerWrapper;

enum class xtop_http_server_method_type {
    no_support = 0,
    GET = 1,
    POST = 2,
};
using xhttp_server_method_type_t = xtop_http_server_method_type;

/// @brief http server base
class xtop_http_server_base : public std::enable_shared_from_this<xtop_http_server_base> {
public:
    xtop_http_server_base() = delete;
    virtual ~xtop_http_server_base();

    xtop_http_server_base(std::string const & server_ip, uint16_t server_port);

protected:
    bool register_json_request_callback(std::string const & path,
                                        xhttp_server_method_type_t const & method,
                                        std::function<bool(std::string const & json_req_content, std::string & json_resp_content)> callback);

    // call this after register all callback
    // defaultly using no-blocking, detach a new server thread.
    void start_server(bool block = false);

private:
    std::shared_ptr<HttpServerWrapper> m_server;
};
using xhttp_server_base_t = xtop_http_server_base;

NS_END2