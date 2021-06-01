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
XINLINE_CONSTEXPR char const * XPROPERTY_GLOBAL_NAME                     = "@2";
XINLINE_CONSTEXPR char const * XPROPERTY_ALIAS_NAME                      = "@3";
XINLINE_CONSTEXPR char const * XPROPERTY_LOCK_TOKEN_KEY                  = "@4";
XINLINE_CONSTEXPR char const * XPROPERTY_LOCK_TOKEN_SUM_KEY              = "@5";
XINLINE_CONSTEXPR char const * XPROPERTY_AUTHORIZE_KEY                   = "@6";
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_KEYS                    = "@7";
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_VOTE_KEY                = "@8";
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_TRANSFER_KEY            = "@9";
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_DATA_KEY                = "@10";
XINLINE_CONSTEXPR char const * XPROPERTY_ACCOUNT_CONSENSUS_KEY           = "@11";
XINLINE_CONSTEXPR char const * XPORPERTY_PARENT_ACCOUNT_KEY              = "@12";
XINLINE_CONSTEXPR char const * XPORPERTY_SUB_ACCOUNT_KEY                 = "@13";
XINLINE_CONSTEXPR char const * XPORPERTY_CONTRACT_SUB_ACCOUNT_KEY        = "@14";
XINLINE_CONSTEXPR char const * XPORPERTY_CONTRACT_PARENT_ACCOUNT_KEY     = "@15";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_NATIVE_TOKEN_KEY       = "@16";
XINLINE_CONSTEXPR const char * XPORPERTY_CONTRACT_NATIVE_TOKEN_OWERN_KEY = "@17";

XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_VOTE_KEY                = "@18";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_VOTE_OUT_KEY            = "@19";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_VOTE_STAT_KEY           = "@20";

XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_BEC_RESULT_KEY          = "@21";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_BEC_ZEC_RESULT_KEY      = "@22";

XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_BEC_VERSION_KEY         = "@23";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_BEC_ZEC_VERSION_KEY     = "@23_1";

XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_RECMD_RESULT_KEY        = "@24";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_EDGE_RESULT_KEY         = "@25";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_ARCHIVE_RESULT_KEY      = "@26";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_ZONE_POOL_KEY           = "@27";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_BEC_POOL_KEY            = "@28";

// XINLINE_CONSTEXPR const char* XPROPERTY_PLEDGE_TOKEN_TGAS_KEY            = "@29";
XINLINE_CONSTEXPR const char* XPROPERTY_USED_TGAS_KEY                    = "@30";
// XINLINE_CONSTEXPR const char* XPROPERTY_FROZEN_TGAS_KEY                  = "@31";
XINLINE_CONSTEXPR const char* XPROPERTY_LAST_TX_HOUR_KEY                 = "@32";

// XINLINE_CONSTEXPR const char* XPROPERTY_PLEDGE_TOKEN_DISK_KEY            = "@33";
XINLINE_CONSTEXPR const char* XPROPERTY_USED_DISK_KEY                    = "@34";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_TGAS_LIMIT_KEY          = "@37";
XINLINE_CONSTEXPR char const* XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT = "@38";
XINLINE_CONSTEXPR char const* XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME = "@39";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_BLOCK_CONTENT_KEY       = "@40";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_STANDBYS_KEY        = "@41";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_ELECTION_RESULT_KEY = "@42"; // use data::election::get_property_name_by_addr() to calculate it
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY = "@42_EXECUTED";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_GROUP_ASSOC_KEY     = "@43";
XINLINE_CONSTEXPR char const* XPROPERTY_CONTRACT_RECOMMEND_KEY       = "@44";

XINLINE_CONSTEXPR const char* XPROPERTY_PLEDGE_VOTE_KEY                  = "@45";
// the corresponding token num of already expired locked vote
XINLINE_CONSTEXPR const char* XPROPERTY_EXPIRE_VOTE_TOKEN_KEY            = "@46";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_TIME_KEY                = "#102";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_ONCHAIN_TIME_ROUND_KEY  = "@100";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_ONCHAIN_TIMESTAMP_ROUND_KEY  = "@101";
XINLINE_CONSTEXPR const char* XPORPERTY_CONTRACT_ONCHAIN_TIMESTAMP_KEY   = "@102";



XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_ELECT_ROUND_KEY         = "round";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_REC_STANDBY_KEY         = "rec_s";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_ZEC_STANDBY_KEY         = "zec_s";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_AUDITOR_STANDBY_KEY     = "audi_s";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_VALIDATOR_STANDBY_KEY   = "valida_s";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_EDGE_STANDBY_KEY        = "edge_s";
XINLINE_CONSTEXPR const char* XPROPERTY_CONTRACT_ARCHIVE_STANDBY_KEY     = "archive_s";


XINLINE_CONSTEXPR int MAX_AUTHOR_KEYS_NUM              = 8;
XINLINE_CONSTEXPR uint16_t MAX_SUB_ACCOUNT             = 256;
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
