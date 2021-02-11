// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>

#include "xbase/xint.h"
#include "xbase/xlog.h"
#include "xbasic/xversion.h"
#include "xbasic/xserializable_based_on.h"
#include "xdata/xdata_common.h"
#include "xdata/xnative_property.h"
#include "xdata/xdatautil.h"

namespace top { namespace data {

struct xpledge_balance
{
    uint64_t tgas{0};
    uint64_t disk{0};
    uint64_t vote{0};
};

struct xpledge_balance_change{
    int64_t tgas{0};
    int64_t disk{0};
    int64_t vote{0};
};

// main state of account
class xaccount_mstate2 : public xserializable_based_on<void> {
 public:
    int32_t do_write(base::xstream_t & stream) const override;
    int32_t do_read(base::xstream_t & stream) override;
    std::string dump() const;

 public:
    uint64_t            get_latest_send_trans_number() const {return m_latest_send_trans_number;}
    void                set_latest_send_trans_number(uint64_t number) {m_latest_send_trans_number = number;}
    const uint256_t &   get_latest_send_trans_hash() const {return m_latest_send_trans_hash;}
    void                set_latest_send_trans_hash(const uint256_t & hash) {m_latest_send_trans_hash = hash;}
    uint64_t            get_latest_recv_trans_number() const {return m_latest_recv_trans_number;}
    void                set_latest_recv_trans_number(uint64_t number) {m_latest_recv_trans_number = number;}
    const uint256_t &   get_latest_recv_trans_hash() const {return m_latest_recv_trans_hash;}
    void                set_latest_recv_trans_hash(const uint256_t & hash) {m_latest_recv_trans_hash = hash;}
    uint64_t            get_balance() const {return m_account_balance;}
    void                set_balance(uint64_t balance) {m_account_balance = balance;}
    void                set_balance_change(int64_t change) {m_account_balance += change;}
    uint64_t            get_burn_balance() const {return m_account_burn_balance;}
    void                set_burn_balance(uint64_t token) {m_account_burn_balance = token;}
    void                set_burn_balance_change(int64_t change) {m_account_burn_balance += change;}
    uint64_t            get_pledge_tgas_balance() const {return m_account_pledge_balance.tgas;}
    void                set_pledge_tgas_balance(uint64_t balance) {m_account_pledge_balance.tgas = balance;}
    void                set_pledge_tgas_balance_change(int64_t change) {m_account_pledge_balance.tgas += change;}
    uint64_t            get_pledge_disk_balance() const {return m_account_pledge_balance.disk;}
    void                set_pledge_disk_balance(uint64_t balance) {m_account_pledge_balance.disk = balance;}
    void                set_pledge_disk_balance_change(int64_t change) {m_account_pledge_balance.disk += change;}
    uint64_t            get_pledge_vote_balance() const {return m_account_pledge_balance.vote;}
    void                set_pledge_vote_balance(uint64_t balance) {m_account_pledge_balance.vote = balance;}
    void                set_pledge_vote_balance_change(int64_t change) {m_account_pledge_balance.vote += change;}
    //uint64_t            get_lock_balance() const {return m_account_lock_balance;}
    //void                set_lock_balance_change(int64_t change) {m_account_lock_balance += change;}
    uint64_t            get_lock_balance() const {return m_account_lock_balance;}
    void                set_lock_balance_change(int64_t change) {m_account_lock_balance += change;}
    uint64_t            get_lock_tgas() const {return m_account_lock_tgas;}
    void                set_lock_tgas(uint64_t tgas) {m_account_lock_tgas = tgas;}
    void                set_lock_tgas_change(int64_t change) {m_account_lock_tgas += change;}
    uint64_t            get_unvote_number() const {return m_account_unvote_num;}
    void                set_unvote_number(uint64_t number) {m_account_unvote_num = number;}
    void                set_unvote_number_change(int64_t change) {m_account_unvote_num += change;}
    int64_t             get_credit() const {return m_account_credit;}
    uint64_t            get_account_nonce() const {return m_account_nonce;}
    uint64_t            get_account_create_time() const {return m_account_create_time;}
    void                set_account_create_time(uint64_t time) {m_account_create_time = time;}
    const std::map<std::string, std::string> & get_propertys_hash() const {return m_property_hash;}
    std::string         get_property_hash(const std::string & prop_name) const;
    bool                get_property_hash(const std::string & prop_name, std::string & hash) const;
    void                set_propertys_hash_change(const std::map<std::string, std::string> & prop_hashs);
    const xnative_property_t & get_native_property() const {return m_native_property;}
    void                set_native_property_change(const xnative_property_t & native_prop);
    uint16_t            get_unconfirm_sendtx_num() const {return m_unconfirm_sendtx_num;}
    void                set_unconfirm_sendtx_num(uint16_t number) {m_unconfirm_sendtx_num = number;}

 private:
    // user fire each transactions linked as a chain
    uint64_t    m_latest_send_trans_number{0};  // heigh or number of transaction submited by account,0 is invalid but 1 fo genius trans
    uint256_t   m_latest_send_trans_hash{};  // all transaction fired by account, they are construct a chain

    // consensus mechanisam connect each received transaction as a chain
    uint64_t    m_latest_recv_trans_number{0};  // heigh or number of transaction submited by account,0 is invalid
    uint256_t   m_latest_recv_trans_hash{};   // all receipt contruct a mpt tree, m_account_address.root point to storage object

    // note: the below properties are not allow to be changed by outside,it only be changed by a valid unit indirectly
    uint64_t    m_account_balance{0};      // token balance,
    uint64_t    m_account_burn_balance{0};
    xpledge_balance m_account_pledge_balance;
    uint64_t    m_account_lock_balance{0};
    //uint64_t    m_account_lock_balance{0};
    uint64_t    m_account_lock_tgas{0};
    uint64_t    m_account_unvote_num{0};   // unvoted number
    int64_t     m_account_credit{0};       // credit score,it from the contribution for chain/platform/network etc
    uint64_t    m_account_nonce{0};        // account 'latest nonce,increase atomic
    uint64_t    m_account_create_time{0};     // when the account create
    int32_t     m_account_status{0};       // status for account like lock,suspend etc
    std::map<std::string, std::string>  m_property_hash;  // [Property Name as key, hash of unfixed-length Property' value]
    xnative_property_t                  m_native_property;
    uint16_t                            m_unconfirm_sendtx_num{0};
    std::map<uint16_t, std::string>     m_ext;
};

}  // namespace data
}  // namespace top
