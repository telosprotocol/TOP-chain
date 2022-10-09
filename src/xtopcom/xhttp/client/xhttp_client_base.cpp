// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xhttp/xhttp_client_base.h"

#ifndef ASIO_STANDALONE
#    define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#    define USE_STANDALONE_ASIO
#endif

#include "simplewebserver/client_http.hpp"

#ifdef ASIO_STANDALONE
#    undef ASIO_STANDALONE
#endif
#ifdef USE_STANDALONE_ASIO
#    undef USE_STANDALONE_ASIO
#endif

using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

NS_BEG2(top, xhttp)

// HttpClientWrapper:
class HttpClientWrapper {
public:
    HttpClient _;
    HttpClientWrapper(std::string const & host) : _{host} {
        _.config.timeout = 20;
    }
};

xtop_http_client_base::xtop_http_client_base(std::string const & url) {
    std::string hosturl = url;
    auto pos = url.find("//");  // http://xxx.xxx/some_path  find and remove `http://`
    if (pos != std::string::npos) {
        hosturl = url.substr(pos + 2);  // xxx.xxx/some_path
    }
    auto path_pos = hosturl.find("/");
    if (path_pos != std::string::npos) {
        hosturl = hosturl.substr(0, path_pos);            // xxx.xxx;
        default_request_path = hosturl.substr(path_pos);  // some_path
    } else {
        default_request_path = "/";
    }

    // xinfo("url: %s, hosturl: %s, path: %s", url.c_str(),hosturl.c_str(),default_request_path.c_str());
    m_client = std::make_shared<HttpClientWrapper>(hosturl);
}

xtop_http_client_base::xtop_http_client_base(std::pair<std::string, uint16_t> const & remote_host_port) : xtop_http_client_base(remote_host_port.first, remote_host_port.second) {
}

xtop_http_client_base::xtop_http_client_base(std::string const & remote_ip, uint16_t remote_port) {
    std::string remote_ip_port = remote_ip + ":" + std::to_string(remote_port);
    m_client = std::make_shared<HttpClientWrapper>(remote_ip_port);
}

xtop_http_client_base::~xtop_http_client_base() {
    m_client->_.stop();
}

std::string xtop_http_client_base::percent_encode(std::string const & data) {
    return SimpleWeb::Percent::encode(data);
}

std::string xtop_http_client_base::request_get() {
    auto res = m_client->_.request("GET", default_request_path);
    return res->content.string();
}

std::string xtop_http_client_base::request_get(std::string const & path) {
    auto res = m_client->_.request("GET", path);
    return res->content.string();
}

std::string xtop_http_client_base::request_post_string(std::string const & path, std::string const & request) {
    auto res = m_client->_.request("POST", path, request);
    return res->content.string();
}

std::string xtop_http_client_base::request_post_json(std::string const & path, std::string const & json_request) {
    SimpleWeb::CaseInsensitiveMultimap header;
    header.insert({"Content-Type", "application/json"});
    auto res = m_client->_.request("POST", path, json_request, header);
    return res->content.string();
}

xtop_http_client_async_base::xtop_http_client_async_base(std::string const & ip_port) {
    m_client = std::make_shared<HttpClientWrapper>(ip_port);
}

xtop_http_client_async_base::~xtop_http_client_async_base() {
    m_client->_.stop();
}

void xtop_http_client_async_base::request_post_string(std::string const & path, std::string const & request, std::function<void(std::string const &)> callback) {
    m_client->_.request("POST", path, request, [callback](std::shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code & ec) {
        if (!ec) {
            callback(response->content.string());
        } else {
            callback("");
        }
    });
}

void xtop_http_client_async_base::run_io_service() {
    m_client->_.io_service->run();
}

NS_END2