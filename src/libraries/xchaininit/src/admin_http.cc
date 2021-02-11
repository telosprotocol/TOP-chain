#include "xchaininit/admin_http.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/line_parser.h"
#include <asio/error.hpp>
#include "xchaininit/xchain_info_query.h"
#include "xchaininit/dashboard_html.h"

namespace  top {


namespace admin {

#ifdef ADMIN_HTTP_THREAD_POOL_COUNT
    #undef ADMIN_HTTP_THREAD_POOL_COUNT
#endif
#define ADMIN_HTTP_THREAD_POOL_COUNT      1   // thread number of thread pool

bool endswith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

//use to check if a file exists
bool isFileExist (const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}



HttpHandler::HttpHandler() {
    char buffer[256];
    getcwd(buffer, sizeof(buffer));
    webroot_ = std::string(buffer);
    TOP_INFO("set HttpHandler webroot:%s", webroot_.c_str());

    auto index_html = webroot_ + "/index.html";
    std::fstream www_file(index_html, std::ios::out);
    www_file << admin_dashboard_html;
    www_file.close();
    TOP_INFO("dump html file: %s", index_html.c_str());

    // TODO(smaug) for now, no using jwt generate token,just using random string
    auto raw_token = top::RandomString(32);
    std::string token = top::HexEncode(raw_token);
    auto token_file = webroot_ + "/token";
    std::fstream tf(token_file, std::ios::out);
    tf << token;
    tf.close();
    token_ = token;
    TOP_INFO("dump token:%s to token file:%s", token.c_str(), token_file.c_str());
}

HttpHandler::HttpHandler(const std::string& webroot) {
    // TODO(smaug) for now, only considered unix-os
    std::string tmp_webroot = webroot;
    if (webroot[webroot.size() - 1] == '/') {
        tmp_webroot = webroot.substr(0, webroot.size() -1);
    }
    if (webroot[0] == '.') {
        char buffer[256];
        getcwd(buffer, sizeof(buffer));
        webroot_ = std::string(buffer) + "/" + tmp_webroot;   // eg. /var/./www
    } else {
        webroot_ = tmp_webroot;
    }
    TOP_INFO("set HttpHandler webroot:%s", webroot_.c_str());

    auto index_html = webroot_ + "/index.html";
    std::fstream www_file(index_html, std::ios::out);
    www_file << admin_dashboard_html;
    www_file.close();
    TOP_INFO("dump html file: %s", index_html.c_str());

    // TODO(smaug) for now, no using jwt generate token,just using random string
    auto raw_token = top::RandomString(32);
    std::string token = top::HexEncode(raw_token);
    auto token_file = webroot_ + "/token";
    std::fstream tf(token_file, std::ios::out);
    tf << token;
    tf.close();
    token_ = token;
    TOP_INFO("dump token:%s to token file:%s", token.c_str(), token_file.c_str());

}

bool HttpHandler::default_not_found(ResponsePtr res, RequestPtr req) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
#endif

    std::string not_found_html = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n\
<title>404 Not Found</title>\n\
<h1>Not Found</h1>\n\
<p>The requested URL was not found on the server. If you entered the URL manually please check your spelling and try again.</p>";
    res->write(SimpleWeb::StatusCode::client_error_not_found, not_found_html);
    return true;
}

bool HttpHandler::default_error(ResponsePtr res, RequestPtr req, const SimpleWeb::error_code & ec) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
    std::cout << "error:" << ec.value() << " msg:" << ec.message() << std::endl;
#endif
    return true;
}


bool HttpHandler::index(ResponsePtr res, RequestPtr req) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
#endif

    res->write();
    return true;
}

bool HttpHandler::headers(ResponsePtr res, RequestPtr req) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
#endif

    json res_content;
    res_content["headers"] =  json::object();

    SimpleWeb::CaseInsensitiveMultimap req_headers = req->header;
    for (const auto& x : req_headers) {
        res_content["headers"][x.first] = x.second;
    }

    std::string str_res_content = res_content.dump(4);
    SimpleWeb::CaseInsensitiveMultimap res_headers;
    res_headers.insert({"Content-Type", "application/json"});
    res_headers.insert({"Connection", "keep-alive"});
    res->write(str_res_content, res_headers);
    return true;
}

bool HttpHandler::response_headers(ResponsePtr res, RequestPtr req) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
#endif

    json res_content;
    res_content["Content-Type"] = "application/json";
    std::string str_res_content = res_content.dump(4);

    SimpleWeb::CaseInsensitiveMultimap res_headers;
    res_headers.insert({"Content-Type", "application/json"});
    res_headers.insert({"Connection", "keep-alive"});
    res->write(str_res_content, res_headers);
    return true;
}


bool HttpHandler::get(ResponsePtr res, RequestPtr req) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
#endif

    json res_content;
    res_content["headers"] =  json::object();

    SimpleWeb::CaseInsensitiveMultimap req_headers = req->header;
    for (const auto& x : req_headers) {
        res_content["headers"][x.first] = x.second;
    }

    res_content["args"] =  json::object();

    auto query_fields = req->parse_query_string();
    for (const auto& x : query_fields) {
        res_content["args"][x.first] = x.second;
    }

    res_content["url"] = req->path;
    res_content["origin"] = req->remote_endpoint_address();
    res_content["origin_port"] = req->remote_endpoint_port();

    std::string str_res_content = res_content.dump(4);

    SimpleWeb::CaseInsensitiveMultimap res_headers;
    res_headers.insert({"Content-Type", "application/json"});
    res_headers.insert({"Connection", "keep-alive"});
    res->write(str_res_content, res_headers);
    return true;
}

bool HttpHandler::post(ResponsePtr res, RequestPtr req) {
    auto req_content = req->content.string();
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    std::cout << "content:" << req_content << std::endl;
#endif

    json res_content;

    res_content["headers"] =  json::object();

    SimpleWeb::CaseInsensitiveMultimap req_headers = req->header;
    for (const auto& x : req_headers) {
        res_content["headers"][x.first] = x.second;
    }

    res_content["args"] =  json::object();
    auto query_fields = req->parse_query_string();
    for (const auto& x : query_fields) {
        res_content["args"][x.first] = x.second;
    }

    res_content["url"] = req->path;
    res_content["origin"] = req->remote_endpoint_address();
    res_content["origin_port"] = req->remote_endpoint_port();
    res_content["body"] = req_content;

    std::string str_res_content = res_content.dump(4);

    SimpleWeb::CaseInsensitiveMultimap res_headers;
    res_headers.insert({"Content-Type", "application/json"});
    res_headers.insert({"Connection", "keep-alive"});
    res->write(str_res_content, res_headers);
    return true;
}

bool HttpHandler::cookies(ResponsePtr res, RequestPtr req) {
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    auto req_content = req->content.string();
    std::cout << "content:" << req_content << std::endl;
#endif

    json res_content;

    res_content["cookies"] =  json::object();
    std::string req_cookies;
    auto header_it = req->header.find("Cookies");
    if(header_it != req->header.end()) {
        req_cookies = header_it->second;
    }

    top::base::LineParser line_split(req_cookies.c_str(), ';', req_cookies.size());
    for (uint32_t i = 0; i < line_split.Count(); ++i) {
        std::string item(line_split[i]);
        top::base::LineParser item_line_split(item.c_str(), '=', item.size());
        if (item_line_split.Count() != 2) {
            continue;
        }
        std::string key = item_line_split[0];
        std::string value = item_line_split[1];
        res_content["cookies"][key] = value;
    }

    std::string str_res_content = res_content.dump(4);
    SimpleWeb::CaseInsensitiveMultimap res_headers;
    res_headers.insert({"Content-Type", "application/json"});
    res_headers.insert({"Connection", "keep-alive"});
    res->write(str_res_content, res_headers);
    return true;
}

bool HttpHandler::verify_token(ResponsePtr res, RequestPtr req) {
    // TODO(smaug) not open token check for now
    return true;
    auto token = "";
    if (token != token_) {
        return false;
    }
    return true;
}

// post method; body contain cmd
bool HttpHandler::handle_command(ResponsePtr res, RequestPtr req) {
    if (!verify_token(res, req)) {
        return false;
    }
    auto req_content = req->content.string();
#ifdef DEBUG
    std::cout << "http:" << req->http_version << " path:" << req->path << " method:" << req->method << " query_string:" << req->query_string << std::endl;
    std::cout << "content:" << req_content << std::endl;
#endif

    SimpleWeb::CaseInsensitiveMultimap req_headers = req->header;
    auto header_it = req->header.find("Content-Type");
    if (header_it == req->header.end() || (header_it->second != "application/json" && header_it->second != "Application/json")) {
        json res_content;
        res_content["status"] = "";
        res_content["error"]=  "not support params";

        std::string str_res_content = res_content.dump(4);  // dump(4)
        SimpleWeb::CaseInsensitiveMultimap res_headers;
        res_headers.insert({"Content-Type", "application/json"});
        res_headers.insert({"Connection", "keep-alive"});
        res->write(SimpleWeb::StatusCode::client_error_bad_request, str_res_content, res_headers);
        return true;
    }

    json req_json;
    try {
        req_json = json::parse(req_content);
    } catch (json::parse_error& e) {
        std::string error  = e.what();
        TOP_WARN("json parse error:%s", error.c_str());
        json res_content;
        res_content["status"] = "";
        res_content["error"]=  error;

        std::string str_res_content = res_content.dump(4);  // dump(4)
        SimpleWeb::CaseInsensitiveMultimap res_headers;
        res_headers.insert({"Content-Type", "application/json"});
        res_headers.insert({"Connection", "keep-alive"});
        res->write(SimpleWeb::StatusCode::client_error_bad_request, str_res_content, res_headers);
        return true;
    }

    auto command = req_json["command"].get<std::string>();

    json res_content;
    std::string result;
    // cmdline: node help; node isjoined; node peers
    if (ChainInfo::Instance()->ProcessCommand(command, result)) {
        res_content["status"] = "ok";
        res_content["error"] = "";
        res_content["data"] = json::object();
        res_content["data"][command] = result;
    } else {
        res_content["status"] = "fail";
        if (result.empty()) {
            res_content["error"] = "request failed";
        } else {
            res_content["error"] = result;
        }
    }

    std::string str_res_content = res_content.dump();  // dump(4)
    SimpleWeb::CaseInsensitiveMultimap res_headers;
    res_headers.insert({"Content-Type", "application/json"});
    res_headers.insert({"Connection", "keep-alive"});
    res->write(str_res_content, res_headers);
    return true;
}


bool HttpHandler::webroot(ResponsePtr res, RequestPtr req) {
    try {
        // TODO(smaug) for now ,only consider about unix-os
        auto path = webroot_ +  req->path;  //   /var/www/index.html
        if (path[path.size() - 1]  == '/') {
            path += "index.html";
        }
        if (!endswith(path, ".html")
                && !endswith(path, ".css")
                && !endswith(path, ".js")
                && !endswith(path, ".png")
                && !endswith(path, ".svg")
                && !endswith(path, ".woff")
                && !endswith(path, ".ttf")
                && !endswith(path, ".jpg")
                && !endswith(path, ".jpeg")
                && !endswith(path, ".txt")
                && !endswith(path, ".doc")
                && !endswith(path, ".pdf")
                && !endswith(path, ".rar")
                && !endswith(path, ".gz")
                && !endswith(path, ".zip")
                && !endswith(path, ".docx")
                && !endswith(path, ".csv")
                && !endswith(path, ".xlsx")) {
            path += "/index.html";
        }

        if (!isFileExist(path)) {
            TOP_WARN("not fount path: %s", path.c_str());
            return default_not_found(res, req);
        }

        SimpleWeb::CaseInsensitiveMultimap res_headers;

        // Uncomment the following line to enable Cache-Control
        res_headers.emplace("Cache-Control", "max-age=86400");

        auto ifs = std::make_shared<std::ifstream>();
        //ifs->open(path, std::fstream::in | std::fstream::binary | std::fstream::ate);
        ifs->open(path, std::fstream::in | std::fstream::binary);

        if(*ifs) {
          auto length = ifs->tellg();
          ifs->seekg(0, std::fstream::beg);

          res_headers.emplace("Content-Length", std::to_string(length));
          res->write(res_headers);

          // Trick to define a recursive function within this scope (for example purposes)
          class FileServer {
          public:
            static void read_and_send(const ResponsePtr &res, const std::shared_ptr<std::ifstream> &ifs) {
              // Read and send 128 KB at a time
              static std::vector<char> buffer(131072); // Safe when server is running on one thread
              std::streamsize read_length;
              if((read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
                res->write(&buffer[0], read_length);
                if(read_length == static_cast<std::streamsize>(buffer.size())) {
                  res->send([res, ifs](const SimpleWeb::error_code &ec) {
                    if(!ec)
                      read_and_send(res, ifs);
                    else
                      std::cerr << "Connection interrupted" << std::endl;
                  });
                }
              }
            }
          };

          FileServer::read_and_send(res, ifs);
        } else {
            throw std::invalid_argument("could not read file");
        }
    }
    catch(const std::exception &e) {
        res->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + req->path + ": " + e.what());
    }
    return true;
}



// end for HttpHandler



// begin for AdminHttpServer

AdminHttpServer::AdminHttpServer() {
    svr_ = std::make_shared<HttpServer>();
    svr_->config.address = local_ip_;
    svr_->config.port = listen_port_;
    svr_->config.thread_pool_size = ADMIN_HTTP_THREAD_POOL_COUNT;
    svr_->config.timeout_request = 5;
    svr_->config.timeout_content = 300;
    svr_->config.max_request_streambuf_size = 50 * 1024 * 1024; // 50MB

    http_handler_ = std::make_shared<HttpHandler>(webroot_);
}


AdminHttpServer::AdminHttpServer(uint16_t port) {
    listen_port_ = port;

    svr_ = std::make_shared<HttpServer>();
    svr_->config.address = local_ip_;
    svr_->config.port = listen_port_;
    svr_->config.thread_pool_size = ADMIN_HTTP_THREAD_POOL_COUNT;
    svr_->config.timeout_request = 5;
    svr_->config.timeout_content = 300;
    svr_->config.max_request_streambuf_size = 50 * 1024 * 1024; // 50MB

    http_handler_ = std::make_shared<HttpHandler>(webroot_);
}

AdminHttpServer::AdminHttpServer(const std::string& local_ip, uint16_t port) {
    local_ip_ = local_ip;
    listen_port_ = port;

    svr_ = std::make_shared<HttpServer>();
    svr_->config.address = local_ip_;
    svr_->config.port = listen_port_;
    svr_->config.thread_pool_size = ADMIN_HTTP_THREAD_POOL_COUNT;
    svr_->config.timeout_request = 5;
    svr_->config.timeout_content = 300;
    svr_->config.max_request_streambuf_size = 50 * 1024 * 1024; // 50MB

    http_handler_ = std::make_shared<HttpHandler>(webroot_);
}

AdminHttpServer::AdminHttpServer(const std::string& local_ip, uint16_t port, const std::string& webroot) {
    local_ip_ = local_ip;
    listen_port_ = port;
    webroot_ = webroot;

    svr_ = std::make_shared<HttpServer>();
    svr_->config.address = local_ip_;
    svr_->config.port = listen_port_;
    svr_->config.thread_pool_size = ADMIN_HTTP_THREAD_POOL_COUNT;
    svr_->config.timeout_request = 5;
    svr_->config.timeout_content = 300;
    svr_->config.max_request_streambuf_size = 50 * 1024 * 1024; // 50MB

    http_handler_ = std::make_shared<HttpHandler>(webroot_);
}

AdminHttpServer::~AdminHttpServer() {
    if (svr_) {
        http_handler_ = nullptr;
        svr_->stop();
        svr_ = nullptr;
        TOP_INFO("AdminHttpServer stop");
    }
}

void AdminHttpServer::Start() {
    bind_route_callback();
    bind_route_callback_for_command();

    auto self(shared_from_this());
    svr_th_ = std::thread([self]() {
        // Start server
        self->svr_->start();
    });
    svr_th_.detach();

    TOP_INFO("AdminHttpServer start with ip:%s port:%u", local_ip_.c_str(), listen_port_);
}



void AdminHttpServer::bind_route_callback() {
    if (!svr_ || !http_handler_) {
        TOP_WARN("AdminHttpServer not ready to bind route callbak");
        return;
    }

    svr_->resource["/api/headers"]["GET"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->headers(res, req);
    };
    TOP_INFO("bind_route_callback route:/headers");

    svr_->resource["/api/get"]["GET"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->get(res, req);
    };
    TOP_INFO("bind_route_callback route:/get");

    svr_->resource["/api/post"]["POST"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->post(res, req);
    };
    TOP_INFO("bind_route_callback route:/post");

    svr_->resource["/api/response-headers"]["GET"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->response_headers(res, req);
    };
    TOP_INFO("bind_route_callback route:/response-headers");

    svr_->resource["/api/cookies"]["GET"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->cookies(res, req);
    };
    TOP_INFO("bind_route_callback route:/cookies");
    // end for test api

    svr_->default_resource["GET"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->webroot(res, req);
    };

    svr_->default_resource["POST"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->default_not_found(res, req);
    };

    svr_->on_error = [&](RequestPtr req, const SimpleWeb::error_code & ec) {
    // Handle errors here
    // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
        if (ec.value() == asio::error::operation_aborted) {
            return;
        }
        ResponsePtr res;
        http_handler_->default_error(res, req, ec);
  };
}

void AdminHttpServer::bind_route_callback_for_command() {
    if (!svr_ || !http_handler_) {
        TOP_WARN("AdminHttpServer not ready to bind route callbak");
        return;
    }

    svr_->resource["/api/command"]["POST"] = [&](ResponsePtr res, RequestPtr req) {
        http_handler_->handle_command(res, req);
    };
    TOP_INFO("bind_route_callback route:/api/command POST");
}

} // namespace admin

} // namespace top
