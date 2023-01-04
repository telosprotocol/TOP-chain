// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbasic/xbasic_size.hpp"
#include "xutility/xhash.h"
#include "xdata/xdatautil.h"
#include "json/json.h"

namespace top { namespace data {

enum enum_xaction_type : uint16_t {
    xaction_type_asset_out                  = 0,    // asset transfer out
    xaction_type_source_null                = 1,    // source action do nothing
    xaction_type_create_user_account        = 2,    // create user account
    xaction_type_create_contract_account    = 3,    // create contract account
    xaction_type_tep                        = 4,    // TEP
    xaction_type_run_contract               = 5,    // run contract
    xaction_type_asset_in                   = 6,    // asset transfer in
    xaction_type_alias_name                 = 8,    // set alias name
    xaction_type_set_account_keys           = 12,    // set account keys
    xaction_type_lock_token                 = 13,    // lock token
    xaction_type_unlock_token               = 14,    // unlock token
    xaction_type_tep0                       = 15,   // system token
    xaction_type_tep1                       = 16,   // native token
    xaction_type_tep2                       = 17,   // contract token
    xaction_type_tep3                       = 18,   // NFT
    xaction_type_tep4                       = 19,   // DID
    xaction_type_update_pledge_contract     = 20,   // update pledge contract token num
    xaction_type_pledge_token_vote          = 21,   // pledge token in exchange of votes
    xaction_type_redeem_token_vote          = 22,   // redeem token and release votes
    xaction_type_pledge_token               = 23,   // pledge token in exchange of resource
    xaction_type_redeem_token               = 24,   // redeem token and release resource

    xaction_type_max
};


class xaction_t {
 public:
    xaction_t& operator = (const xaction_t&) = default;

    virtual int32_t    serialize_write(base::xstream_t & stream, bool is_write_without_len) const {
        const int32_t begin_pos = stream.size();
        stream << m_action_hash;
        stream << m_action_type;
        if (is_write_without_len) {
            uint16_t action_size = 0;
            stream << action_size;
        } else {
            stream << m_action_size;
        }
        stream << m_account_addr;
        stream << m_action_name;
        stream << m_action_param;
        stream << m_action_ext;
        stream << m_action_authorization;
        const int32_t end_pos = stream.size();
        return (end_pos - begin_pos);
    }
    virtual int32_t    serialize_read(base::xstream_t & stream) {
        const int32_t begin_pos = stream.size();
        stream >> m_action_hash;
        stream >> m_action_type;
        stream >> m_action_size;
        stream >> m_account_addr;
        stream >> m_action_name;
        stream >> m_action_param;
        stream >> m_action_ext;
        stream >> m_action_authorization;
        const int32_t end_pos = stream.size();
        return (begin_pos - end_pos);
    }

    std::string get_action_str() const {
        std::string str;
        str += std::to_string(m_action_hash);
        str += std::to_string(m_action_type);
        str += std::to_string(m_action_size);
        str += m_account_addr;
        str += m_action_name;
        str += to_hex_str(m_action_param);
        str += m_action_authorization;
        str += m_action_ext;
        return str;
    }

 public:
    void set_action_hash(uint32_t hash) {m_action_hash = hash;};
    uint32_t get_action_hash() const {return m_action_hash;};
    void set_action_type(enum_xaction_type type) {m_action_type = type;};
    enum_xaction_type get_action_type() const { return m_action_type; };
    void set_action_size(uint16_t size) {m_action_size = size;};
    uint16_t get_action_size() const {return m_action_size;};
    void set_account_addr(const std::string & addr) {m_account_addr = addr;};
    const std::string & get_account_addr() const {return m_account_addr;};
    void set_action_name(const std::string & action_name) {m_action_name = action_name;};
    const std::string & get_action_name() const {return m_action_name;};
    void set_action_param(const std::string & action_param) {m_action_param = action_param;};
    const std::string & get_action_param() const {return m_action_param;};
    void clear_action_param() {m_action_param.clear();};
    void set_action_ext(const std::string & action_ext) {m_action_ext = action_ext;};
    const std::string & get_action_ext() const {return m_action_ext;};
    void set_action_authorization(const std::string & action_authorization) {m_action_authorization = action_authorization;};
    const std::string & get_action_authorization() const {return m_action_authorization;};
    int32_t get_ex_alloc_size() const {
        return get_size(m_account_addr) + get_size(m_action_name) + get_size(m_action_param) + get_size(m_action_ext) + get_size(m_action_authorization);
    }

private:
    uint32_t           m_action_hash{0};   // whole action content'xxhash32
    enum_xaction_type  m_action_type{enum_xaction_type::xaction_type_asset_out};  // type
    uint16_t           m_action_size{0};   // total size for whole action object

    std::string        m_account_addr{};  // account address must embedded network info(e.g. testnet, mainnet)
    std::string        m_action_name{};
    std::string        m_action_param{};  //
    std::string        m_action_ext{};
    std::string        m_action_authorization{};  // it might empty if container already has authorization
};

}  // namespace data
}  // namespace top
