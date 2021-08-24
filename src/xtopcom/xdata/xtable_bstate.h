// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xobject_ptr.h"
#include "xvledger/xvstate.h"
#include "xvledger/xdataobj_base.hpp"
#include "xvledger/xaccountindex.h"
#include "xvledger/xreceiptid.h"

NS_BEG2(top, data)

XINLINE_CONSTEXPR char const * XPROPERTY_TABLE_ACCOUNT_INDEX        = "@T0";
XINLINE_CONSTEXPR char const * XPROPERTY_TABLE_RECEIPTID            = "@T2";


// xtable_bstate_t is a wrap of xvbstate_t for table state
class xtable_bstate_t {
 public:
    xtable_bstate_t(base::xvbstate_t* bstate);
    ~xtable_bstate_t();

 public:
    std::string             make_snapshot();
    static bool             set_block_offsnapshot(base::xvblock_t* block, const std::string & snapshot);

 public:
    const xobject_ptr_t<base::xvbstate_t> &     get_bstate() const {return m_bstate;}
    const std::string &     get_account() const {return m_bstate->get_account();}
    uint64_t                get_block_height() const {return m_bstate->get_block_height();}

 public:
    bool                    get_account_index(const std::string & account, base::xaccount_index_t & account_index) const;
    std::set<std::string>   get_all_accounts() const;
    std::set<std::string>   get_unconfirmed_accounts() const;
    int32_t                 get_account_size() const;
    bool                    find_receiptid_pair(base::xtable_shortid_t sid, base::xreceiptid_pair_t & pair) const;
    uint32_t                get_unconfirm_tx_num() const {return m_cache_receiptid->get_unconfirm_tx_num();}
    const base::xreceiptid_state_ptr_t & get_receiptid_state() const {return m_cache_receiptid;}

 public:
    bool                    set_account_index(const std::string & account, const base::xaccount_index_t & account_index, base::xvcanvas_t* canvas);
    bool                    set_receiptid_pair(base::xtable_shortid_t sid, const base::xreceiptid_pair_t & pair, base::xvcanvas_t* canvas);

 protected:
    void                    cache_receiptid();

 private:
    xobject_ptr_t<base::xvbstate_t> m_bstate;
    base::xreceiptid_state_ptr_t    m_cache_receiptid{nullptr};
};

using xtablestate_ptr_t = std::shared_ptr<xtable_bstate_t>;

NS_END2
