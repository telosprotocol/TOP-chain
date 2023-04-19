// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xns_macro.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvstate.h"

NS_BEG2(top, statestore)

class xvstateplugin_t : public base::xvactplugin_t
{
public:
    xvstateplugin_t(base::xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms);
    virtual ~xvstateplugin_t();
private:
    xvstateplugin_t();
    xvstateplugin_t(xvstateplugin_t &&);
    xvstateplugin_t(const xvstateplugin_t &);
    xvstateplugin_t & operator = (const xvstateplugin_t &);
    
public:
    //only allow call once
    // virtual bool                    init_meta(const base::xvactmeta_t & meta) override {}

public:
    void    set_unitstate(std::string const& block_hash, xobject_ptr_t<base::xvbstate_t> const& unitstate);
    xobject_ptr_t<base::xvbstate_t>  get_unitstate(uint64_t height, std::string const& block_hash) const;
private:
    uint8_t     m_max_count{2};
    std::map<uint64_t, std::pair<std::string, xobject_ptr_t<base::xvbstate_t>>> m_bstates;
};

NS_END2