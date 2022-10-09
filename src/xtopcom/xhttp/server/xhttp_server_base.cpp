// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xhttp/xhttp_server_base.h"

#ifndef ASIO_STANDALONE
#    define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#    define USE_STANDALONE_ASIO
#endif

#include "simplewebserver/server_http.hpp"

#ifdef ASIO_STANDALONE
#    undef ASIO_STANDALONE
#endif
#ifdef USE_STANDALONE_ASIO
#    undef USE_STANDALONE_ASIO
#endif

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

NS_BEG2(top, xhttp)

// HttpServerWrapper:
class HttpServerWrapper {
public:
    HttpServer _;
    HttpServerWrapper(std::string const & ip, uint16_t port) {
        _.config.address = ip;
        _.config.port = port;
        _.config.thread_pool_size = 1;
        _.config.timeout_request = 5;
        _.config.timeout_content = 300;
        _.config.max_request_streambuf_size = 50 * 1024 * 1024;  // 50MB
    }
};

xtop_http_server_base::~xtop_http_server_base() {
    m_server->_.stop();
}

xtop_http_server_base::xtop_http_server_base(std::string const & server_ip, uint16_t server_port) {
    m_server = std::make_shared<HttpServerWrapper>(server_ip, server_port);
}

bool xtop_http_server_base::register_json_request_callback(std::string const & path,
                                                           xhttp_server_method_type_t const & method,
                                                           std::function<bool(std::string const & json_req_content, std::string & json_resp_content)> callback) {
    assert(method == xhttp_server_method_type_t::GET || method == xhttp_server_method_type_t::POST);
    std::string method_str{""};
    if (method == xhttp_server_method_type_t::GET) {
        method_str = "GET";
    } else if (method == xhttp_server_method_type_t::POST) {
        method_str = "POST";
    } else {
        return false;
    }
    m_server->_.resource[path][method_str] = [callback](std::shared_ptr<HttpServer::Response> res, std::shared_ptr<HttpServer::Request> req) {
        // check header content-type is json.
        SimpleWeb::CaseInsensitiveMultimap req_headers = req->header;
        auto header_it = req->header.find("Content-Type");
        if (header_it == req->header.end() || (header_it->second != "application/json" && header_it->second != "Application/json")) {
            json res_content;
            res_content["status"] = "";
            res_content["error"] = "not support params";

            std::string str_res_content = res_content.dump(4);  // dump(4)
            SimpleWeb::CaseInsensitiveMultimap res_headers;
            res_headers.insert({"Content-Type", "application/json"});
            res_headers.insert({"Connection", "keep-alive"});
            res->write(SimpleWeb::StatusCode::client_error_bad_request, str_res_content, res_headers);
            return;
        }

        auto const & req_content = req->content.string();
#ifdef DEBUG
        std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
        std::cout << "content:" << req_content << std::endl;
#endif

        std::string json_res_content;
        if (callback(req_content, json_res_content)) {
            // success.
            SimpleWeb::CaseInsensitiveMultimap res_headers;
            res_headers.insert({"Content-Type", "application/json"});
            res_headers.insert({"Connection", "keep-alive"});
            res->write(json_res_content, res_headers);
        } else {
            // false.
            json res_content;
            res_content["status"] = "";
            res_content["error"] = "not support params";

            std::string str_res_content = res_content.dump(4);  // dump(4)
            SimpleWeb::CaseInsensitiveMultimap res_headers;
            res_headers.insert({"Content-Type", "application/json"});
            res_headers.insert({"Connection", "keep-alive"});
            res->write(SimpleWeb::StatusCode::client_error_bad_request, str_res_content, res_headers);
            return;
        }

        return;
    };

    return true;
}

void xtop_http_server_base::start_server(bool block) {
    if (block) {
        m_server->_.start();
    } else {
        auto self(shared_from_this());
        auto server_thread = std::thread([self]() {
            // Start server
            self->m_server->_.start();
        });
        server_thread.detach();
    }
}

NS_END2