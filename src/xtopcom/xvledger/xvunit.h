// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xmem.h"
#include "xbase/xobject_ptr.h"

namespace top
{
    namespace base
    {

        // tableblock has accountindex, so xoffdata_t should be less 
        // unit 拆分成两部分，索引信息放在table块上，binlog部分放在offdata上，两者一起可以解开组成完整unit块


        // unit is a block including state change binlog for account
        class xvunit_t {
        public:
            xvunit_t(xvunit_t const & prev_unit, std::string const& binlog);
            virtual ~xvunit_t() {}

            int32_t serialize_to_string(std::string & str) const;
            int32_t do_write(base::xstream_t & stream) const;
            int32_t serialize_from_string(const std::string & _data);
            int32_t do_read(base::xstream_t & stream);

        public:
            std::string const&  get_address() const {return m_address;}
            uint64_t            get_height() const {return m_height;}
            std::string const&  get_last_block_hash() const {return m_last_block_hash;}
            std::string const&  get_binlog() const {return m_binlog;}            
            std::string const&  get_block_hash() const {return m_block_hash;}
        public:            

        private:
            std::string     m_address;  // need compress serialize
            uint64_t        m_height{0};
            std::string     m_last_block_hash;
            std::string     m_binlog;  // binlog or state
            uint32_t        m_flags{0};        //[1][enum_xvblock_class][enum_xvblock_level][enum_xvblock_type][enum_xvblock_reserved]
            std::string     m_state_hash;  // TODO(jimmy) need?
            uint64_t        m_parent_height{0};  // TODO(jimmy) need? 唯一的用处是同步历史unit块，服务端把unit块关联的table块完整同步过来。 还有种解决办法，共识节点如果要使用历史unit块，说明这条unit链需要保存足够多的块，可以遍历unit链往前同步。
            std::string     m_extend_data;  // for future
            std::string     m_block_hash;
        };

        class xvunit_builder_t {
        public:
            std::shared_ptr<xvunit_t>   make_unit(std::string const& address, uint64_t height);

        };

        using xvunit_ptr_t = std::shared_ptr<xvunit_t>;
        
    }//end of namespace of base

}//end of namespace top
