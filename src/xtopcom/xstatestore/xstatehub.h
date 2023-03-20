// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xns_macro.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, statestore)

class xvstatehub_t
{
public:
    xvstatehub_t(base::xvtable_t * target_table);
    virtual ~xvstatehub_t();
private:
    xvstatehub_t(xvstatehub_t &&);
    xvstatehub_t(const xvstatehub_t &);
    xvstatehub_t & operator = (const xvstatehub_t &);
public:
    void    set_unitstate(std::string const& block_hash, xobject_ptr_t<base::xvbstate_t> const& unitstate);
    xobject_ptr_t<base::xvbstate_t>  get_unitstate(std::string const& account, uint64_t height, std::string const& block_hash) const;
private:
    base::xvtable_t* m_target_table{nullptr};
    uint64_t m_idle_timeout_ms{0};
    mutable std::mutex m_hub_mutex; 
};

using xvstatehub_ptr_t = std::shared_ptr<xvstatehub_t>;

NS_END2