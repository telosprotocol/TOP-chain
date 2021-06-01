// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xint.h"
#include "xbase/xdata.h"

namespace top { namespace data {

class xnative_property_name_t {
 public:
    static bool is_native_property(const std::string & name) {
        return name[0] == '@';
    }
};

XINLINE_CONSTEXPR char const * XPROPERTY_ASSET_TOP                       = "TOP";

XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_AVAILABLE              = "@@1";  //available balance
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_BURN                   = "@@2";  //burn balance  TODO(jimmy) is need ?
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_LOCK                   = "@@3";  //lock balance
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_PLEDGE_TGAS            = "@@4";  //pledge balance for tgas
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_PLEDGE_VOTE            = "@@5";  //pledge balance for vote

XINLINE_CONSTEXPR char const * XPROPERTY_LOCK_TGAS                      = "@00";  //TODO(jimmy) this property should discuss later
XINLINE_CONSTEXPR char const * XPROPERTY_UNVOTE_NUM                     = "@01";  // where is voted num ?
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO                        = "@02";
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_LATEST_SENDTX_NUM      = "1";
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_LATEST_SENDTX_HASH     = "2";
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_RECVTX_NUM             = "3";
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM       = "4";
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_CREATE_TIME            = "@03";

XINLINE_CONSTEXPR char const * XPROPERTY_CONTRACT_CODE                   = "@1";
XINLINE_CONSTEXPR char const * XPROPERTY_LOCK_TOKEN_KEY                  = "@4";


XINLINE_CONSTEXPR char const * XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY        = "@14";
XINLINE_CONSTEXPR char const * XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY     = "@15";

XINLINE_CONSTEXPR const char* XPROPERTY_USED_TGAS_KEY                    = "@30";
XINLINE_CONSTEXPR const char* XPROPERTY_LAST_TX_HOUR_KEY                 = "@32";

XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_TGAS_LIMIT_KEY          = "@37";
XINLINE_CONSTEXPR char const* XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT = "@38";
XINLINE_CONSTEXPR char const* XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME = "@39";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_BLOCK_CONTENT_KEY       = "@40";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_STANDBYS_KEY        = "@41";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_ELECTION_RESULT_KEY = "@42"; // use data::election::get_property_name_by_addr() to calculate it
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY = "@42_EXECUTED";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_GROUP_ASSOC_KEY     = "@43";

XINLINE_CONSTEXPR const char* XPROPERTY_PLEDGE_VOTE_KEY                  = "@45";
// the corresponding token num of already expired locked vote
XINLINE_CONSTEXPR const char* XPROPERTY_EXPIRE_VOTE_TOKEN_KEY            = "@46";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_TIME_KEY                = "#102";

XINLINE_CONSTEXPR uint16_t MAX_NORMAL_CONTRACT_ACCOUNT = 32;

class xproperty_asset {
 public:
    explicit xproperty_asset(uint64_t amount) : m_amount(amount) { }
    xproperty_asset(const std::string & token_name, uint64_t amount) : m_token_name(token_name), m_amount(amount) { }
    bool is_top_token() const {return m_token_name.empty() || XPROPERTY_ASSET_TOP == m_token_name;};
    std::string m_token_name{XPROPERTY_ASSET_TOP};
    uint64_t    m_amount{0};
};

}  // namespace data
}  // namespace top
