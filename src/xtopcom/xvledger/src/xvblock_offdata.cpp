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


        int32_t xvblock_offdata_base_t::serialize_to_string(std::string & str) const {
            base::xstream_t _stream(base::xcontext_t::instance());
            auto size = do_write(_stream);
            str.clear();
            str.assign((const char*)_stream.data(), _stream.size());
            return str.size();
        }

        int32_t xvblock_offdata_base_t::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            uint32_t map_count = static_cast<uint32_t>(m_map.size());
            stream.write_compact_var(m_version);
            stream.write_compact_var(map_count);
            for (auto pair : m_map) {
                stream.write_compact_var(pair.first);
                stream.write_compact_var(pair.second);
            }
            return (stream.size() - begin_size);
        }

        int32_t xvblock_offdata_base_t::serialize_from_string(const std::string & _data) {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = do_read(_stream);
            return result;
        }

        int32_t xvblock_offdata_base_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.read_compact_var(m_version);
            uint32_t map_count;
            stream.read_compact_var(map_count);
            for (uint32_t i = 0; i < map_count; ++i) {
                std::string key;
                std::string val;
                stream.read_compact_var(key);
                stream.read_compact_var(val);
                m_map[key] = val;
            }
            return (begin_size - stream.size());
        }

        void xvblock_offdata_base_t::insert(const std::string & key, const std::string & val) {
            m_map[key] = val;
        }

        std::string xvblock_offdata_base_t::get_val(const std::string & key) const {
            auto it = m_map.find(key);
            if (it != m_map.end()) {
                return it->second;
            } else {
                return "";
            }
        }

        //---------------------------------xvblock_out_offdata_t---------------------------------//
        xvblock_out_offdata_t::xvblock_out_offdata_t(const std::vector<xobject_ptr_t<xvblock_t>> & subblocks) {
            std::vector<subblock_build_info_t> subblocks_info;
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
                subblocks_info.push_back(_info);
            }
            set_subblocks_info(subblocks_info);
        }
        void xvblock_out_offdata_t::set_subblocks_info(const std::vector<subblock_build_info_t> & infos) {
            base::xstream_t _stream(base::xcontext_t::instance());
            uint32_t count = infos.size();
            _stream.write_compact_var(count);
            for (auto & v : infos) {
                _stream.write_compact_var(v.m_header_bin);
                _stream.write_compact_var(v.m_fullstate_hash);
                _stream.write_compact_var(v.m_binlog);
            }
            std::string _stream_str;
            _stream_str.assign((const char*)_stream.data(), _stream.size());
            insert(KEY_SUBBLOCKS, _stream_str);
        }

        std::vector<subblock_build_info_t> xvblock_out_offdata_t::get_subblocks_info() const {
            std::string _str = get_val(KEY_SUBBLOCKS);
            base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)_str.data(), _str.size());
            uint32_t count;
            _stream.read_compact_var(count);
            std::vector<subblock_build_info_t> infos;
            for (uint32_t i = 0; i < count; i++) {
                subblock_build_info_t info;
                _stream.read_compact_var(info.m_header_bin);
                _stream.read_compact_var(info.m_fullstate_hash);
                _stream.read_compact_var(info.m_binlog);
                infos.push_back(info);
            }
            return infos;
        }

    }//end of namespace of base

}//end of namespace top
