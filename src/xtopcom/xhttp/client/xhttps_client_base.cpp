// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xhttp/xhttps_client_base.h"

#ifndef ASIO_STANDALONE
#    define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#    define USE_STANDALONE_ASIO
#endif

#include "simplewebserver/client_https.hpp"

#ifdef ASIO_STANDALONE
#    undef ASIO_STANDALONE
#endif
#ifdef USE_STANDALONE_ASIO
#    undef USE_STANDALONE_ASIO
#endif

using HttpsClient = SimpleWeb::Client<SimpleWeb::HTTPS>;

NS_BEG2(top, xhttp)

// HttpsClientWrapper:
class HttpsClientWrapper {
public:
    HttpsClient _;
    HttpsClientWrapper(std::string const & host) : _{host} {
        _.config.timeout = 20;
    }
};

xtop_https_client_base::xtop_https_client_base(std::string const & url) {
    std::string hosturl = url;
    auto pos = url.find("//");  // https://xxx.xxx/some_path  find and remove `https://`
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

    // xinfo("url: %s, hosturl: %s, path: %s", url.c_str(), hosturl.c_str(), default_request_path.c_str());
    m_client = std::make_shared<HttpsClientWrapper>(hosturl);
}

xtop_https_client_base::~xtop_https_client_base() {
    m_client->_.stop();
}

std::string xtop_https_client_base::request_get() {
    auto res = m_client->_.request("GET", default_request_path);
    return res->content.string();
}

std::string xtop_https_client_base::request_get(std::string const & path) {
    auto res = m_client->_.request("GET", path);
    return res->content.string();
}

std::string xtop_https_client_base::request_post_string(std::string const & path, std::string const & request) {
    auto res = m_client->_.request("POST", path, request);
    return res->content.string();
}

std::string xtop_https_client_base::request_post_json(std::string const & path, std::string const & json_request) {
    SimpleWeb::CaseInsensitiveMultimap header;
    header.insert({"Content-Type", "application/json"});
    auto res = m_client->_.request("POST", path, json_request, header);
    return res->content.string();
}

NS_END2