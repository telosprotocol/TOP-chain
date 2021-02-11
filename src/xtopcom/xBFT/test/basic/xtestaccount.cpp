// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xBFT/xconsengine.h"
#include "xtestaccount.hpp"
#include "xunitblock.hpp"
#include "xtestnode.hpp"

namespace top
{
    namespace test
    {
        xtestaccount_t::xtestaccount_t(xconsensus::xcsobject_t & parent_object,const std::string & account_address)
            :xconsensus::xcsaccount_t(parent_object,account_address)
        {
            xdbg("xtestaccount_t::create");
        }
        
        xtestaccount_t::xtestaccount_t(base::xcontext_t & _context,const int32_t target_thread_id,const std::string & account_address)
            :xconsensus::xcsaccount_t(_context,target_thread_id,account_address)
        {
            xdbg("xtestaccount_t::create");
        }
        
        xtestaccount_t::~xtestaccount_t()
        {
            xdbg("xtestaccount_t::destroy,account=%s",get_account().c_str());
        }; 
    };
};


