// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xhttp/xhttps_client_base.h"

#include <string>
#include <vector>

NS_BEG2(top, elect)

class HttpsSeedFetcher : public xhttp::xhttps_client_base_t {
private:
    using base_t = xhttp::xhttps_client_base_t;

public:
    HttpsSeedFetcher(std::string const & url);

public:
    bool GetSeeds(std::vector<std::string> & url_seeds);
};

NS_END2