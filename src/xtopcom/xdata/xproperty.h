// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xcxx_config.h"
#include "xbase/xint.h"
#include "xbase/xdata.h"

namespace top { namespace data {

XINLINE_CONSTEXPR char const * XPROPERTY_ASSET_TOP                       = "TOP";

// $1-9 reserved for future
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_AVAILABLE              = "$0";  //available balance
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_BURN                   = "$a";  //burn balance
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_LOCK                   = "$b";  //lock balance
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_PLEDGE_TGAS            = "$c";  //pledge balance for tgas
XINLINE_CONSTEXPR char const * XPROPERTY_BALANCE_PLEDGE_VOTE            = "$d";  //pledge balance for vote

// tgas related propertys
XINLINE_CONSTEXPR char const * XPROPERTY_LOCK_TGAS                      = "$00";  //TODO(jimmy) this property should discuss later
XINLINE_CONSTEXPR char const * XPROPERTY_USED_TGAS_KEY                  = "$01";
XINLINE_CONSTEXPR char const * XPROPERTY_LAST_TX_HOUR_KEY               = "$02";
// vote related propertys
XINLINE_CONSTEXPR char const * XPROPERTY_PLEDGE_VOTE_KEY                = "$03";  // the detailed info of pledge vote
XINLINE_CONSTEXPR char const * XPROPERTY_EXPIRE_VOTE_TOKEN_KEY          = "$04";  // the token of all expired pledge token for vote
XINLINE_CONSTEXPR char const * XPROPERTY_UNVOTE_NUM                     = "$05";  // unvote number
// tx info propertys
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO                        = "$06";
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_LATEST_SENDTX_NUM      = "1";  // XPROPERTY_TX_INFO map field
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_LATEST_SENDTX_HASH     = "2";  // XPROPERTY_TX_INFO map field
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_RECVTX_NUM             = "3";  // XPROPERTY_TX_INFO map field
XINLINE_CONSTEXPR char const * XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM       = "4";  // XPROPERTY_TX_INFO map field
// account create time
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_CREATE_TIME            = "$07";
// lock token related
XINLINE_CONSTEXPR char const * XPROPERTY_LOCK_TOKEN_KEY                 = "$08";




XINLINE_CONSTEXPR char const* XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT = "@38";
XINLINE_CONSTEXPR char const* XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME = "@39";

XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_STANDBYS_KEY        = "@41";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_ELECTION_RESULT_KEY = "@42"; // use data::election::get_property_name_by_addr() to calculate it
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY = "@42-e";//"@42_EXECUTED";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_GROUP_ASSOC_KEY     = "@43";


// the corresponding token num of already expired locked vote

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
