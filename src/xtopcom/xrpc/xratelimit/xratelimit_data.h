#pragma once
#ifndef RATELIMIT_DATA_H_
#define RATELIMIT_DATA_H_

#include "xratelimit_thread_queue.h"
#include "xbase/xns_macro.h"
#include "xrpc/xhttp/xhttp_server.h"



NS_BEG2(top, xChainRPC)


class RatelimitDataHttp : public RatelimitData {
public:
    virtual ~RatelimitDataHttp() {}
    std::string content_;
    std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response_;
};

class RatelimitDataWs : public RatelimitData {
public:
    virtual~RatelimitDataWs() {}
    std::string content_;
    std::shared_ptr<SimpleWeb::SocketServer<SimpleWeb::WS>::Connection> connection_;
};

NS_END2

#endif  // !RATELIMIT_DATA_H_
