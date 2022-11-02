// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xhttp/xhttp_client_base.h"

#include <map>

NS_BEG2(top, chain_command)

class ChainCommandFetcher : public xhttp::xhttp_client_base_t {
private:
    using base_t = xhttp::xhttp_client_base_t;

public:
    ChainCommandFetcher(std::pair<std::string, uint16_t> const & http_ip_port);
    void Request(std::string const & cmdline, std::string & result);
};

NS_END2