#pragma once

#include <memory>

#include "simplewebserver/client_http.hpp"

namespace  top {

using HttpClient    = SimpleWeb::Client<SimpleWeb::HTTP>;
using HttpClientPtr = std::shared_ptr<HttpClient>;
using ClResponsePtr = std::shared_ptr<HttpClient::Response>;

namespace admin {

class AdminHttpClient : public std::enable_shared_from_this<AdminHttpClient> {
public:
    AdminHttpClient(AdminHttpClient const &)              = delete;
    AdminHttpClient & operator=(AdminHttpClient const &)  = delete;
    AdminHttpClient(AdminHttpClient &&)                   = delete;
    AdminHttpClient & operator=(AdminHttpClient &&)       = delete;

public:
    AdminHttpClient(const std::string& remote_ip, uint16_t remote_port);
    AdminHttpClient(const std::string& remote_ip, uint16_t remote_port, uint32_t timeout);
    AdminHttpClient(const std::string& remote_ip, uint16_t remote_port, uint32_t timeout, const std::string& proxy);
    virtual  ~AdminHttpClient();

public:
    void Request(const std::string& cmdline, std::string& result);

private:
    HttpClientPtr                      cli_          { nullptr };
};

using AdminHttpClientPtr = std::shared_ptr<AdminHttpClient>;

} // namespace admin

} // end namespace top
