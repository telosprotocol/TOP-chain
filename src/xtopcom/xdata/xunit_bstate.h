// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xblock.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvstate.h"
#include "xdata/xproperty.h"
#include "xdata/xbstate_ctx.h"

NS_BEG2(top, data)

class xunit_bstate_t : public xbstate_ctx_t {
 public:
    xunit_bstate_t(base::xvbstate_t* bstate, bool readonly = true);
    
    ~xunit_bstate_t();
 private:
    xunit_bstate_t(const xunit_bstate_t &);
    xunit_bstate_t & operator = (const xunit_bstate_t &);

 public:  // for unit account
    inline uint64_t     balance()const {return token_get(XPROPERTY_BALANCE_AVAILABLE);}
    inline uint64_t     burn_balance()const {return token_get(XPROPERTY_BALANCE_BURN);}
    inline uint64_t     tgas_balance() const {return token_get(XPROPERTY_BALANCE_PLEDGE_TGAS);}
    inline uint64_t     disk_balance() const {return 0;}  // TODO(jimmy) not support
    inline uint64_t     vote_balance() const {return token_get(XPROPERTY_BALANCE_PLEDGE_VOTE);}
    inline uint64_t     lock_balance() const {return token_get(XPROPERTY_BALANCE_LOCK);}
    inline uint64_t     lock_tgas() const {return uint64_property_get(XPROPERTY_LOCK_TGAS);}
    inline uint64_t     unvote_num() const {return uint64_property_get(XPROPERTY_UNVOTE_NUM);}
    uint64_t            get_account_create_time() const;
    uint32_t            get_unconfirm_sendtx_num() const;
    uint64_t            get_latest_send_trans_number() const;
    uint64_t            account_recv_trans_number() const;
    uint256_t           account_send_trans_hash() const;
    uint64_t            account_send_trans_number() const;
    uint64_t            tep_balance(const std::string & token_name) const;
    uint64_t            get_free_tgas() const ;
    uint64_t            get_total_tgas(uint32_t token_price) const ;
    uint64_t            get_last_tx_hour() const ;
    uint64_t            get_used_tgas() const ;
    uint64_t            calc_decayed_tgas(uint64_t timer_height) const ;
    static uint32_t     get_token_price(uint64_t onchain_total_gas_deposit) ;
    uint64_t            get_available_tgas(uint64_t timer_height, uint32_t token_price) const ;

 public: // Set APIs
    int32_t     set_account_create_time(uint64_t clock);
    int32_t     set_tx_info_latest_sendtx_num(uint64_t num);
    int32_t     set_tx_info_latest_sendtx_hash(const std::string & hash);
    int32_t     set_tx_info_recvtx_num(uint64_t num);
};

using xaccount_ptr_t = std::shared_ptr<xunit_bstate_t>;  // TODO(jimmy) rename to xunitstate_ptr_t
using xunitstate_ptr_t = std::shared_ptr<xunit_bstate_t>;

NS_END2
