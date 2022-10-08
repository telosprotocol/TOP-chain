// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xns_macro.h"

#include <map>
#include <memory>
#include <string>

NS_BEG2(top, xhttp)

// fwd:
class HttpClientWrapper;

/// @brief blocking http client base
class xtop_http_client_base {
public:
    xtop_http_client_base() = delete;
    virtual ~xtop_http_client_base();

    xtop_http_client_base(std::string const & url);
    xtop_http_client_base(std::pair<std::string, uint16_t> const & remote_host_port);
    xtop_http_client_base(std::string const & remote_ip, uint16_t remote_port);

public:
    std::string percent_encode(std::string const & data);

protected:
    std::string request_get();

    std::string request_get(std::string const & path);

    std::string request_post_string(std::string const & path, std::string const & request);

    std::string request_post_json(std::string const & path, std::string const & json_request);

private:
    std::shared_ptr<HttpClientWrapper> m_client;

    std::string default_request_path{""};
};

using xhttp_client_base_t = xtop_http_client_base;

NS_END2