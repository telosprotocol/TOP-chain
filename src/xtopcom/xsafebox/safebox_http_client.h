// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xhttp/xhttp_client_base.h"

#include <map>

NS_BEG2(top, safebox)

class SafeBoxHttpClient : public xhttp::xhttp_client_base_t {
private:
    using base_t = xhttp::xhttp_client_base_t;

public:
    SafeBoxHttpClient(std::string const & local_host);

    /// @brief request account from safebox
    /// @param account optional, return lastest account if empty.
    /// @return account string
    std::string request_account(std::string const & account = "");

    /// @brief request prikey from safebox
    /// @param account optional, return lastest account's prikey if empty.
    /// @return prikey string
    std::string request_prikey(std::string const & account = "");

    /// @brief request both account and prikey from safebox. used in get default account
    /// @return std::pair<account, prikey>
    std::pair<std::string, std::string> request_account_prikey();

    bool set_prikey(std::string const & account, std::string const & pri_key, std::ostringstream & out_str, uint32_t expired_time);

private:
    std::string request_get(std::string const & account = "");
};

NS_END2