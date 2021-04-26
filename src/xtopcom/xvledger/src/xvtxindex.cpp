// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvblock.h"
#include "../xvtxindex.h"

namespace top
{
    namespace base
    {
        xvtxindex_t::xvtxindex_t()
            : xdataunit_t(xdataunit_t::enum_xdata_type_undefine)
        {
            m_raw_tx_obj    = NULL;
            m_block_flags = 0;
            m_block_height = 0;
            m_tx_phase_type = 0;
        }
    
        xvtxindex_t::xvtxindex_t(xvblock_t & owner, xdataunit_t* raw_tx,const std::string & txhash, enum_transaction_subtype type)
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine)
        {
            m_raw_tx_obj    = NULL;
            m_block_addr    = owner.get_account();
            m_block_height  = owner.get_height();
            m_block_hash    = owner.get_block_hash();
            m_tx_hash       = txhash;
            m_tx_phase_type = type;
            m_block_flags   = (owner.get_block_flags() >> 8); //lowest 8bit is meaning less,so just skip it
            
            m_raw_tx_obj = raw_tx;
            if(raw_tx != NULL)
                raw_tx->add_ref();
        }
    
        xvtxindex_t::~xvtxindex_t()
        {
            if(m_raw_tx_obj != NULL)
                m_raw_tx_obj->release_ref();
        }
    
        int32_t xvtxindex_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream.write_compact_var(m_block_addr);
            stream.write_compact_var(m_block_height);
            stream.write_compact_var(m_block_hash);
            stream.write_compact_var(m_tx_hash);
            
            stream << m_tx_phase_type;
            stream << m_block_flags;
 
            return (stream.size() - begin_size);
        }

        int32_t xvtxindex_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream.read_compact_var(m_block_addr);
            stream.read_compact_var(m_block_height);
            stream.read_compact_var(m_block_hash);
            stream.read_compact_var(m_tx_hash);
            
            stream >> m_tx_phase_type;
            stream >> m_block_flags;
            
            return (begin_size - stream.size());
        }
      
    };//end of namespace of base
};//end of namespace of top
