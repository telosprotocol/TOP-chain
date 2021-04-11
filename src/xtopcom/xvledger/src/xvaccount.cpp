// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvaccount.h"

namespace top
{
    namespace base
    {
        xvaccount_t::xvaccount_t(const std::string & account_address)
        {
            m_account_addr  = account_address;
            m_account_xid   = get_xid_from_account(m_account_addr);
        }
        
        xvaccount_t::xvaccount_t(const xvaccount_t & obj)
        {
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
        }
    
        xvaccount_t::~xvaccount_t()
        {
        }
    
    };//end of namespace of base
};//end of namespace of top
