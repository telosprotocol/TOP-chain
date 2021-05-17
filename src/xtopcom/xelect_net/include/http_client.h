// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <vector>
#include <string>

#include "simplewebserver/client_http.hpp"

namespace  top {

using HttpClient    = SimpleWeb::Client<SimpleWeb::HTTP>;
using HttpClientPtr = std::shared_ptr<HttpClient>;

namespace elect {

class SeedHttpClient : public std::enable_shared_from_this<SeedHttpClient> {
public:
    SeedHttpClient(SeedHttpClient const &)              = delete;
    SeedHttpClient & operator=(SeedHttpClient const &)  = delete;
    SeedHttpClient(SeedHttpClient &&)                   = delete;
    SeedHttpClient & operator=(SeedHttpClient &&)       = delete;

public:
    SeedHttpClient(const std::string& url);
    virtual  ~SeedHttpClient();

public:
    bool GetSeeds(std::vector<std::string>& url_seeds);
    bool GetEdgeSeeds(std::vector<std::string>& url_seeds);
    void Request(const std::string& path, std::string& result);

private:
    std::string                         path_              { "" };
    HttpClientPtr                       cli_               { nullptr };
};

using SeedHttpClientPtr = std::shared_ptr<SeedHttpClient>;

} // namespace elect

} // end namespace top
