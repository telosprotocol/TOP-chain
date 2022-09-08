// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xdata.h"
#include "xbase/xhash.h"
#include "xvledger/xvblock_offdata.h"

namespace top
{
    namespace base
    {
        //---------------------------------xunitblock_t---------------------------------//




        //---------------------------------xvblock_out_offdata_t---------------------------------//
        xvblock_out_offdata_t::xvblock_out_offdata_t(const std::vector<xobject_ptr_t<xvblock_t>> & subblocks) {
            for (auto & subblock : subblocks) {
                std::string header_bin;
                subblock->get_header()->serialize_to_string(header_bin);
                std::string binlog;
                if (subblock->get_block_class() == base::enum_xvblock_class_full) {
                    binlog = subblock->get_full_state();
                } else {
                    binlog = subblock->get_binlog();
                }

                subblock_build_info_t _info;
                _info.m_header_bin = header_bin;
                _info.m_binlog = binlog;
                _info.m_fullstate_hash = subblock->get_fullstate_hash();
                m_subblocks_info.push_back(_info);
            }
        }

        int32_t xvblock_out_offdata_t::serialize_to_string(std::string & str) const {
            base::xstream_t _stream(base::xcontext_t::instance());
            auto size = do_write(_stream);
            str.clear();
            str.assign((const char*)_stream.data(), _stream.size());
            return str.size();
        }

        int32_t xvblock_out_offdata_t::do_write(base::xstream_t & _stream) const {
            const int32_t begin_size = _stream.size();
            _stream.write_compact_var(m_version);
            uint32_t num = m_subblocks_info.size();
            _stream.write_compact_var(num);
            for (auto & _info :m_subblocks_info) {
                _stream.write_compact_var(_info.m_binlog);
                _stream.write_compact_var(_info.m_fullstate_hash);
                _stream.write_compact_var(_info.m_header_bin);
            }

            return (_stream.size() - begin_size);
        }

        int32_t xvblock_out_offdata_t::serialize_from_string(const std::string & _data) {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = do_read(_stream);
            return result;
        }

        int32_t xvblock_out_offdata_t::do_read(base::xstream_t & _stream) {
            const int32_t begin_size = _stream.size();
            uint8_t version = 0;
            _stream.read_compact_var(version);
            if (version != 0) {
                xerror("xvblock_out_offdata_t::do_read fail-unknown version %d", version);
                return -1;
            }
            uint32_t subblock_num = 0;
            _stream.read_compact_var(subblock_num);
            for (uint32_t i = 0; i < subblock_num; ++i) {
                std::string     _header_bin;
                std::string     _binlog;
                std::string     _fullstate_hash;
                _stream.read_compact_var(_binlog);
                _stream.read_compact_var(_fullstate_hash);
                _stream.read_compact_var(_header_bin);
                subblock_build_info_t _info;
                _info.m_header_bin = _header_bin;
                _info.m_binlog = _binlog;
                _info.m_fullstate_hash = _fullstate_hash;
                m_subblocks_info.push_back(_info);
            }
            return (begin_size - _stream.size());
        }

    }//end of namespace of base

}//end of namespace top
