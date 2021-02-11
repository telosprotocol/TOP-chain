// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_map>
#include <memory>
#include "xrpc/xerror/xrpc_error.h"
#include "xpre_request_handler_server.h"
NS_BEG2(top, xrpc)
template <typename T>
class pre_request_face
{
public:
    using method_handlers = std::unordered_map<pair<string, string>, pre_request_handler>;

    virtual ~pre_request_face() {}
    method_handlers const& methods() const { return m_methods; }
    virtual void init(pre_request_service<>* service)
    {
        m_service = service;
    }

protected:
    void register_method(const std::pair<std::string, std::string>& method_name, const pre_request_handler& method_handler)
    {
        m_methods.emplace(method_name, method_handler);
    }
    pre_request_service<>* get_service()
    {
        return m_service;
    }
private:
    method_handlers          m_methods;
    pre_request_service<>*   m_service;
};

NS_END2
