// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <vector>
#include <string>

#include "simplewebserver/client_https.hpp"

namespace  top {

using HttpsClient    = SimpleWeb::Client<SimpleWeb::HTTPS>;
using HttpsClientPtr = std::shared_ptr<HttpsClient>;

namespace elect {

class SeedHttpsClient : public std::enable_shared_from_this<SeedHttpsClient> {
public:
    SeedHttpsClient(SeedHttpsClient const &)              = delete;
    SeedHttpsClient & operator=(SeedHttpsClient const &)  = delete;
    SeedHttpsClient(SeedHttpsClient &&)                   = delete;
    SeedHttpsClient & operator=(SeedHttpsClient &&)       = delete;

public:
    SeedHttpsClient(const std::string& url);
    virtual  ~SeedHttpsClient();

public:
    bool GetSeeds(std::vector<std::string>& url_seeds);
    void Request(const std::string& path, std::string& result);

private:
    std::string                         path_              { "" };
    HttpsClientPtr                      cli_               { nullptr };
};

using SeedHttpsClientPtr = std::shared_ptr<SeedHttpsClient>;

} // namespace elect  

} // end namespace top
