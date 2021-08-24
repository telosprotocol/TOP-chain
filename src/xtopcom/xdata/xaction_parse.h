// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xaction.h"
#include "xdata/xdatautil.h"
#include "xdata/xtransaction.h"

#include <map>
#include <string>

namespace top {
namespace data {

class xaction_face_t {
public:
    virtual int32_t parse(const xaction_t & action) = 0;
};

class xaction_asset_out : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action, const data::xproperty_asset & asset);
    bool operator==(const xaction_asset_out & other);
    bool operator!=(const xaction_asset_out & other);
    bool is_top_token() const;

public:
    data::xproperty_asset m_asset_out{0};
};

class xaction_asset_proc : public xaction_face_t {
public:
    virtual enum_xaction_type action_type() = 0;
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action, const data::xproperty_asset & asset, enum_xaction_type action_type);
    bool is_top_token() const;

public:
    data::xproperty_asset m_asset{0};
};

class xaction_asset_in : public xaction_asset_proc {
public:
    enum_xaction_type action_type() {
        return xaction_type_asset_in;
    };
};

class xaction_pledge_token : public xaction_asset_proc {
public:
    enum_xaction_type action_type() {
        return xaction_type_pledge_token;
    };
};

class xaction_redeem_token : public xaction_asset_proc {
public:
    enum_xaction_type action_type() {
        return xaction_type_redeem_token;
    };
};

class xaction_source_null : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action);
};

class xaction_create_user_account : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action, const std::string & address);

    std::string m_address;
};

class xaction_deploy_contract : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action, uint64_t tgas_limit, const std::string & code);

    uint64_t m_tgas_limit;
    std::string m_code;
};

class xaction_deploy_clickonce_contract : public xaction_face_t {
public:
    int32_t parse(xaction_t const & action) override;

    uint64_t tgas_limit{UINT64_MAX};
    std::string code;
    enum_xaction_type type{enum_xaction_type::xaction_type_tep};
};

class xaction_run_contract : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action, const std::string & function_name, const std::string & para);

    std::string m_function_name;
    std::string m_para;
};

class xaction_alias_name : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    static int32_t serialze_to(xaction_t & action, const std::string & name);

    std::string m_name;
};

class xaction_set_account_keys : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;

    std::string m_account_key;
    std::string m_key_value;
};

class xaction_lock_account_token : public xaction_face_t {
public:
    enum unlock_type {
        UT_time = 0,
        UT_alone_key,
        UT_muti_keys,
    };

public:
    int32_t parse(const xaction_t & action) override;

    int32_t parse_param(const std::string & params);

    uint32_t m_version;
    uint64_t m_amount;
    /*unlock_type: 0 time; 1: alone key; 2: mutiply keys*/
    uint32_t m_unlock_type;                    // timer/alone key/mutiply keys
    std::vector<std::string> m_unlock_values;  // unlock_type timer vale/alone key value/mutiply keys value
    std::string m_params;                      // serialize lock context
};

class xaction_unlock_account_token : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;

    int32_t parse_param(const std::string & params);

    uint32_t m_version;
    std::string m_lock_tran_hash;
    std::vector<std::string> m_signatures;  // timer/alone key/mutiply keys
};

class xaction_pledge_token_vote : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    int32_t serialze_to(xaction_t & action, const uint64_t num, const uint16_t duration);

    uint64_t m_vote_num;
    uint16_t m_lock_duration;  // unit day
};

class xaction_redeem_token_vote : public xaction_face_t {
public:
    int32_t parse(const xaction_t & action) override;
    int32_t serialze_to(xaction_t & action, const uint64_t num);

    uint64_t m_vote_num;
};

}  // namespace data
}  // namespace top
