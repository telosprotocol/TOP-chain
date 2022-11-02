// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xhttp/xhttp_server_base.h"

#include <string>

NS_BEG2(top, chain_command)

class ChainCommandServer : public xhttp::xhttp_server_base_t {
private:
    using base_t = xhttp::xhttp_server_base_t;

public:
    ChainCommandServer(std::string const & server_ip, uint16_t server_port);

    void Start();
};

NS_END2