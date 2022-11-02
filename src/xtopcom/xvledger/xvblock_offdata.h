// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xvledger/xvblock.h"

namespace top
{
    namespace base
    {

        struct subblock_build_info_t {
            std::string     m_header_bin;  // TODO(jimmy)
            std::string     m_binlog;
            std::string     m_fullstate_hash;
        };

        class xvblock_offdata_base_t {
        public:
            int32_t serialize_to_string(std::string & str) const;
            int32_t do_write(base::xstream_t & stream) const;
            int32_t serialize_from_string(const std::string & _data);
            int32_t do_read(base::xstream_t & stream);
            void    insert(const std::string & key, const std::string & val);
            std::string get_val(const std::string & key) const;
            bool    is_empty() const {return m_map.empty();}
            std::map<std::string, std::string> const&   get_all_val() const {return m_map;}

        private:
            uint8_t     m_version{0};
            std::map<std::string, std::string> m_map;
        };

        class xvblock_out_offdata_t : public xvblock_offdata_base_t {
        private:
            static  constexpr char const * KEY_SUBBLOCKS     = "1";
        public:
            xvblock_out_offdata_t() = default;
            xvblock_out_offdata_t(const std::vector<xobject_ptr_t<xvblock_t>> & subblocks);

        public:
            std::vector<subblock_build_info_t>      get_subblocks_info() const;
            void    set_subblocks_info(const std::vector<subblock_build_info_t> & info);

        private:
            uint8_t     m_version{0};
            std::map<std::string, std::string>  m_values;
        };

    }//end of namespace of base

}//end of namespace top
