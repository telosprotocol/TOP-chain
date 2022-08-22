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
        class xvblock_out_offdata_t {
        public:
            xvblock_out_offdata_t() = default;
            xvblock_out_offdata_t(const std::vector<xobject_ptr_t<xvblock_t>> & subblocks);
        public:
            int32_t serialize_to_string(std::string & str) const;
            int32_t do_write(base::xstream_t & stream) const;
            int32_t serialize_from_string(const std::string & _data);
            int32_t do_read(base::xstream_t & stream);
        public:
            void    set_subblocks(std::vector<xobject_ptr_t<xvblock_t>> subblocks);

        public:
            bool    is_empty() const;                        
            const std::vector<xobject_ptr_t<xvblock_t>> &   get_subblocks() const {return m_subblocks;}

        private:
            std::vector<xobject_ptr_t<base::xvblock_t>>     m_subblocks;
        };

    }//end of namespace of base

}//end of namespace top
