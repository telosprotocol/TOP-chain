// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xdata.h"
#include "xbase/xmem.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvunit.h"

namespace top
{
    namespace base
    {
        xvunit_t::xvunit_t(xvunit_t const & prev_unit, std::string const& binlog) {
            m_address = prev_unit.get_address();
            m_height = prev_unit.get_height() + 1;
            m_last_block_hash = prev_unit.get_block_hash();
            m_binlog = binlog;
            m_flags = 0; // TODO(jimmy)
            m_extend_data = std::string();
        }
        int32_t xvunit_t::serialize_to_string(std::string & str) const {
            base::xstream_t _stream(base::xcontext_t::instance());
            do_write(_stream);
            str.clear();
            str.assign((const char*)_stream.data(), _stream.size());
            return str.size();
        }

        int32_t xvunit_t::serialize_from_string(const std::string & _data) {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = do_read(_stream);
            return result;
        }

        int32_t xvunit_t::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            stream.write_compact_var(m_address);
            stream.write_compact_var(m_height);
            stream.write_compact_var(m_last_block_hash);
            stream.write_compact_var(m_binlog);
            stream.write_compact_var(m_flags);
            stream.write_compact_var(m_extend_data);       

            stream.write_compact_var(m_block_hash);
            return (stream.size() - begin_size);
        }

        int32_t xvunit_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.read_compact_var(m_address);
            stream.read_compact_var(m_height);
            stream.read_compact_var(m_last_block_hash);
            stream.read_compact_var(m_binlog);
            stream.read_compact_var(m_flags);
            stream.read_compact_var(m_extend_data);

            stream.read_compact_var(m_block_hash);              
            return (begin_size - stream.size());
        }
                
    }//end of namespace of base

}//end of namespace top
