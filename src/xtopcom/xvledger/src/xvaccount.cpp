// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvaccount.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        xvaccount_t::xvaccount_t()
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_xid = 0;
        }
    
        xvaccount_t::xvaccount_t(const std::string & account_address)
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_addr  = account_address;
            m_account_xid   = get_xid_from_account(account_address);
            m_account_xid_str = xstring_utl::uint642hex(m_account_xid);
        }
        
        xvaccount_t::xvaccount_t(const xvaccount_t & obj)
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_xid_str = obj.m_account_xid_str;
        }
    
        xvaccount_t & xvaccount_t::operator = (const xvaccount_t & obj)
        {
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_xid_str = obj.m_account_xid_str;
            return *this;
        }
    
        xvaccount_t & xvaccount_t::operator = (const std::string & new_account_addr)
        {
            m_account_addr  = new_account_addr;
            m_account_xid   = get_xid_from_account(new_account_addr);
            m_account_xid_str = xstring_utl::uint642hex(m_account_xid);
            return *this;
        }
    
        xvaccount_t::~xvaccount_t()
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, -1);
        }

        std::string xvaccount_t::compact_address_to(const std::string & account_addr)
        {
            std::string compact_addr;
            enum_vaccount_addr_type addr_type = get_addrtype_from_account(account_addr);
            if (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account)
            {
                char compact_type = enum_vaccount_compact_type_eth_main_chain;
                xassert(ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN == account_addr.substr(0, enum_vaccount_address_prefix_size));
                compact_addr = account_addr.substr(enum_vaccount_address_prefix_size);
                compact_addr = base::xstring_utl::from_hex(compact_addr);
                return compact_type + compact_addr;
            }
            else
            {
                return account_addr;
            }
        }
        std::string xvaccount_t::compact_address_from(const std::string & data)
        {
            // the first byte is compact type
            enum_vaccount_compact_type compact_type = (enum_vaccount_compact_type)data[0];

            if (compact_type == enum_vaccount_compact_type_no_compact)
            {
                return data;
            }
            else if (compact_type == enum_vaccount_compact_type_eth_main_chain)
            {
                std::string compact_addr = data.substr(1);
                std::string account_addr;
                compact_addr = base::xstring_utl::to_hex(compact_addr);
                account_addr = ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN + compact_addr;
                return account_addr;
            }
            else
            {
                xassert(false);
                return {};
            }
        }

    };//end of namespace of base
};//end of namespace of top
