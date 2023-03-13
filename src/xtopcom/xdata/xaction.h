// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xstatistic/xbasic_size.hpp"
#include "xdata/xdatautil.h"
#include "xutility/xhash.h"
#include "xcommon/xaccount_address.h"

#include <string>

NS_BEG2(top, data)

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

NS_END2

NS_BEG3(top, data, details)

class xtop_action {
public:
    xtop_action() = default;
    xtop_action(xtop_action const &) = default;
    xtop_action & operator=(const xtop_action &) = default;
    xtop_action(xtop_action &&) = default;
    xtop_action & operator=(xtop_action &&) = default;
    virtual ~xtop_action() = default;

    virtual int32_t serialize_write(base::xstream_t & stream, bool is_write_without_len) const;
    virtual int32_t serialize_read(base::xstream_t & stream);

    std::string to_string() const;

    void set_action_hash(uint32_t hash);
    uint32_t get_action_hash() const;
    void set_action_type(enum_xaction_type type);
    enum_xaction_type get_action_type() const;
    void set_action_size(uint16_t size);
    uint16_t get_action_size() const;
    //void set_account_addr(const std::string & addr) {m_account_addr = addr;};
    //const std::string & get_account_addr() const {return m_account_addr;};
    void set_action_name(const std::string & action_name);
    const std::string & get_action_name() const;
    void set_action_param(const std::string & action_param);
    const std::string & get_action_param() const;
    void clear_action_param();
    void set_action_ext(const std::string & action_ext);
    const std::string & get_action_ext() const;
    void set_action_authorization(const std::string & action_authorization);
    const std::string & get_action_authorization() const;
    size_t get_ex_alloc_size() const;

    // new APIs
    void account_address(common::xaccount_address_t account_address);
    common::xaccount_address_t const & account_address() const noexcept;

private:
    uint32_t           m_action_hash{0};   // whole action content'xxhash32
    enum_xaction_type  m_action_type{enum_xaction_type::xaction_type_asset_out};  // type
    uint16_t           m_action_size{0};   // total size for whole action object

    common::xaccount_address_t m_account_addr{};  // account address must embedded network info(e.g. testnet, mainnet)
    std::string        m_action_name{};
    std::string        m_action_param{};  //
    std::string        m_action_ext{};
    std::string        m_action_authorization{};  // it might empty if container already has authorization
};

NS_END3

NS_BEG2(top, data)

using xaction_t = details::xtop_action;

NS_END2

