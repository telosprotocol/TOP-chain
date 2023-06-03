// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cassert>
#include "xbase/xutl.h"
#include "xvledger/xvblock_extra.h"

namespace top
{
    namespace base
    {
        int32_t xtableheader_extra_t::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            stream << m_paras;
            return (stream.size() - begin_size);
        }

        int32_t xtableheader_extra_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream >> m_paras;
            return (begin_size - stream.size());
        }

        int32_t xtableheader_extra_t::deserialize_from_string(const std::string & extra_data) {
            base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)extra_data.data(), (uint32_t)extra_data.size());
            return serialize_from_short(_stream);
        }

        int32_t xtableheader_extra_t::serialize_to_string(std::string & extra_data) {
            if (!m_paras.empty()) {
                base::xautostream_t<1024> _stream(base::xcontext_t::instance());
                int32_t ret = serialize_to(_stream);
                extra_data.clear();
                extra_data.assign((const char *)_stream.data(), _stream.size());
                return ret;
            }
            return 0;
        }

        std::string xtableheader_extra_t::get_value(xblockheader_extra_data_type type) const {
            auto iter = m_paras.find(type);
            if (iter == m_paras.end()) {
                return {};
            } else {
                return iter->second;
            }
        }
        void xtableheader_extra_t::set_value(xblockheader_extra_data_type type, std::string const& value) {
            if (!value.empty()) {
                m_paras[type] = value;
            }
        }

        uint64_t xtableheader_extra_t::get_tgas_total_lock_amount_property_height() const {
            std::string value = get_value(enum_extra_data_type_tgas_total_lock_amount_property_height);
            if (value.empty()) {
                return 0;
            } else {
                return base::xstring_utl::touint64(value);
            }
        }

        void xtableheader_extra_t::set_tgas_total_lock_amount_property_height(uint64_t height) {
            std::string height_str = base::xstring_utl::tostring(height);
            set_value(enum_extra_data_type_tgas_total_lock_amount_property_height, height_str);
        }

        uint64_t xtableheader_extra_t::get_second_level_gmtime() const {
            std::string value = get_value(enum_extra_data_type_tgas_second_level_gmtime);
            if (value.empty()) {
                return 0;
            } else {
                return base::xstring_utl::touint64(value);
            }
        }

        void     xtableheader_extra_t::set_second_level_gmtime(uint64_t gmtime) {
            std::string value = base::xstring_utl::tostring(gmtime);
            set_value(enum_extra_data_type_tgas_second_level_gmtime, value);
        }

        std::string xtableheader_extra_t::get_ethheader() const {
            return get_value(enum_extra_data_type_eth_header);
        }

        void xtableheader_extra_t::set_ethheader(const std::string & value) {
            set_value(enum_extra_data_type_eth_header, value);
        }

        std::string xtableheader_extra_t::get_output_offdata_hash() const {
            return get_value(enum_extra_data_type_output_offdata_hash);
        }
        void xtableheader_extra_t::set_output_offdata_hash(const std::string & value) {
            set_value(enum_extra_data_type_output_offdata_hash, value);
        }

        std::string xtableheader_extra_t::get_pledge_balance_change_tgas() const {
            return get_value(enum_extra_data_type_pledge_balance_change_tgas);
        }
        void xtableheader_extra_t::set_pledge_balance_change_tgas(const std::string & value) {
            set_value(enum_extra_data_type_pledge_balance_change_tgas, value);
        }

        void xtableheader_extra_t::set_total_burn_gas(uint64_t burn_gas) {
            std::string burn_gas_str = base::xstring_utl::tostring(burn_gas);
            set_value(enum_extra_data_type_burn_gas, burn_gas_str);
        }

        uint64_t xtableheader_extra_t::get_total_burn_gas() const {
            std::string value = get_value(enum_extra_data_type_burn_gas);
            if (value.empty()) {
                return 0;
            } else {
                return base::xstring_utl::touint64(value);
            }
        }            
        std::string   xtableheader_extra_t::dump() const //just for debug purpose
        {
            char local_param_buf[256];

            xprintf(local_param_buf,sizeof(local_param_buf),"{%zu,%ld,%ld,%zu,%ld,%ld,%s,%ld}",
                m_paras.size(),
                get_tgas_total_lock_amount_property_height(),
                get_second_level_gmtime(),
                get_ethheader().size(),
                base::xhash64_t::digest(get_ethheader()),
                base::xhash64_t::digest(get_output_offdata_hash()),
                get_pledge_balance_change_tgas().c_str(),
                get_total_burn_gas());
           
            return std::string(local_param_buf);
        }


        //-------------------xunit_header_extra_t-------------------------//
        int32_t xunit_header_extra_t::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            stream.write_compact_var(static_cast<uint32_t>(m_paras.size()));
            for (auto & pair : m_paras) {
                stream.write_compact_var(pair.first);
                stream.write_compact_var(pair.second);
            }
            return (stream.size() - begin_size);
        }
        int32_t xunit_header_extra_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint32_t size;
            stream.read_compact_var(size);
            for (uint32_t i = 0; i < size; ++i) {
                uint16_t key;
                std::string val;
                stream.read_compact_var(key);
                stream.read_compact_var(val);
                m_paras[key] = val;
            }
            return (begin_size - stream.size());
        }     
        std::string xunit_header_extra_t::get_value(uint16_t type) const {
            auto iter = m_paras.find(type);
            if (iter != m_paras.end()) {
                return iter->second;
            }
            return {};
        }
        void xunit_header_extra_t::set_value(uint16_t type, std::string const& value) {
            if (!value.empty()) {
                m_paras[type] = value;
            } else {
                xassert(false);
            }
        }
        int32_t xunit_header_extra_t::deserialize_from_string(const std::string & extra_data) {
            if (!extra_data.empty()) {
                base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)extra_data.data(), (uint32_t)extra_data.size());
                return do_read(_stream);
            }
            return 0;
        }
        int32_t xunit_header_extra_t::serialize_to_string(std::string & extra_data) {
            if (!m_paras.empty()) {
                base::xautostream_t<1024> _stream(base::xcontext_t::instance());
                int32_t ret = do_write(_stream);
                extra_data.clear();
                extra_data.assign((const char *)_stream.data(), _stream.size());
                return ret;
            }
            return 0;
        }
    }//end of namespace of base

}//end of namespace top
