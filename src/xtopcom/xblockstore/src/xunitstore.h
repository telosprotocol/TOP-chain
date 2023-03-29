// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"
#include "xvblockdb.h"

namespace top
{
    namespace store
    {
        class xunitstore_t {
         public:
            xunitstore_t(xvblockdb_t* blockdb);
            bool    store_units(base::xvblock_t* table_block, std::vector<base::xvblock_ptr_t> const& units);
            bool    store_units(base::xvblock_t* table_block);
            bool    store_unit(const base::xvaccount_t & account,base::xvblock_t* unit);            
            bool    exist_unit(const base::xvaccount_t & account) const;
            base::xauto_ptr<base::xvblock_t>  load_unit(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash);
            base::xauto_ptr<base::xvblock_t>  load_unit(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid);
            base::xauto_ptr<base::xvblock_t>  load_unit(const base::xvaccount_t & account,const uint64_t height);// return committed unit
         private:
            xvblockdb_t*         m_blockdb_ptr;
        };

        using xunitstore_ptr_t = std::shared_ptr<xunitstore_t>;
    }
}
