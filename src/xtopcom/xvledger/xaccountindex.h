// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbase/xobject_ptr.h"
#include "xbase/xns_macro.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, base)

// [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
// the account flag for checking need sync or load firstly 4bit
enum enum_xaccount_index_flag {
    enum_xaccount_index_flag_account_destroy  = 0x01,  // the account has been destroyed
    enum_xaccount_index_flag_has_unconfirm_tx = 0x02,  // the account has unconfirm send tx
    enum_xaccount_index_flag_carry_nonce      = 0x04,  // the account has tx nonce
    enum_xaccount_index_flag_res2             = 0x08,  // reserved
};
// the latest non-empty block consensus flag 2bit
enum enum_xblock_consensus_type {
    enum_xblock_consensus_flag_authenticated  = 0,
    enum_xblock_consensus_flag_locked         = 1,
    enum_xblock_consensus_flag_committed      = 2,
    enum_xblock_consensus_flag_res            = 3,
};

// account index info is the index info of account unit blockchain
class xaccount_index_t {
 public:
    xaccount_index_t();
    xaccount_index_t(base::xvblock_t* unit,
                     bool has_unconfirm_tx,
                     enum_xblock_consensus_type _cs_type,
                     bool is_account_destroy,
                     uint64_t latest_tx_nonce);
    ~xaccount_index_t();
    xaccount_index_t(const xaccount_index_t& left);
    bool operator == (const xaccount_index_t &other) const {
        if (m_latest_unit_height == other.m_latest_unit_height
            && m_latest_unit_viewid == other.m_latest_unit_viewid
            && m_account_flag == other.m_account_flag
            && m_latest_tx_nonce == other.m_latest_tx_nonce) {
            return true;
        }
        return false;
    }

    int32_t do_write(base::xstream_t & stream) const;
    int32_t do_read(base::xstream_t & stream);
    int32_t         serialize_to(std::string & bin_data) const;
    int32_t         serialize_from(const std::string & bin_data);
    std::string     dump() const;

 public:
    const uint64_t          get_latest_tx_nonce() const {return m_latest_tx_nonce;}
    uint64_t                get_latest_unit_height() const {return m_latest_unit_height;}
    uint64_t                get_latest_unit_viewid() const {return m_latest_unit_viewid;}
    bool                    is_match_unit(base::xvblock_t* unit) const;
    bool                    is_has_unconfirm_tx() const {return check_account_index_flag(enum_xaccount_index_flag_has_unconfirm_tx);}
    bool                    is_account_destroy() const {return check_account_index_flag(enum_xaccount_index_flag_account_destroy);}

    // [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
    base::enum_xvblock_class    get_latest_unit_class() const {return (base::enum_xvblock_class)((m_account_flag >> 13) & 0x07);}
    base::enum_xvblock_type     get_latest_unit_type() const {return (base::enum_xvblock_type)((m_account_flag >> 6) & 0x7F);}
    enum_xaccount_index_flag    get_account_index_flag() const {return (enum_xaccount_index_flag)((m_account_flag >> 2) & 0x0F);}
    enum_xblock_consensus_type  get_latest_unit_consensus_type() const {return (enum_xblock_consensus_type)((m_account_flag) & 0x03);}
    bool                        check_account_index_flag(enum_xaccount_index_flag _flag) const;

 private:  // only can be set by constructor function
    // [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
    void                    set_latest_unit_class(base::enum_xvblock_class _class);
    void                    set_latest_unit_type(base::enum_xvblock_type _type);
    void                    set_account_index_flag(enum_xaccount_index_flag _flag);
    void                    set_latest_unit_consensus_type(enum_xblock_consensus_type _type);

 private:
    uint64_t        m_latest_unit_height{0};
    uint64_t        m_latest_unit_viewid{0};
    uint64_t        m_latest_tx_nonce{0};
    uint16_t        m_account_flag{0};  // [enum_xvblock_class 3bit][enum_xvblock_type 7bit][enum_xaccount_index_flag 4bit][enum_xblock_consensus_type 2bit] = 16bits
};

NS_END2
