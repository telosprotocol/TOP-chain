// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xhttp/xhttp_client_base.h"

#include <string>
#include <vector>

NS_BEG2(top, elect)

class HttpSeedFetcher : public xhttp::xhttp_client_base_t {
private:
    using base_t = xhttp::xhttp_client_base_t;

public:
    HttpSeedFetcher(std::string const & url);

public:
    bool GetSeeds(std::vector<std::string> & url_seeds);
    bool GetEdgeSeeds(std::vector<std::string> & url_seeds);
};

NS_END2