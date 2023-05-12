// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvbindex.h"
#include "../xvaccount.h"
 #include "xmetrics/xmetrics.h"
 #include "xstatistic/xbasic_size.hpp"
namespace top
{
    namespace base
    {
        xvbindex_t::xvbindex_t() : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_bindex)
        {
            init();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbindex_t, 1);
        }

        xvbindex_t::xvbindex_t(xvblock_t & obj)
            :xvaccount_t(obj.get_account()), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_bindex)
        {
            init();

            m_block_height          = obj.get_height();
            m_block_viewid          = obj.get_viewid();
            m_block_viewtoken       = obj.get_viewtoken();
            m_block_hash            = obj.get_block_hash();
            m_last_block_hash       = obj.get_last_block_hash();
            m_last_fullblock_hash   = obj.get_last_full_block_hash();
            m_last_fullblock_height = obj.get_last_full_block_height();

            m_parent_block_height   = obj.get_parent_block_height();
            m_parent_block_viewid   = obj.get_parent_block_viewid();
            m_parent_block_entity_id = obj.get_parent_entity_id();
            // m_extend_cert = obj.get_cert()->get_extend_cert();  XTODO not set extend cert and extend data
            // m_extend_data = obj.get_cert()->get_extend_data();

            //copy flags of block,and combine class of block
            //[8bit:block-flags][8bit:index-bits]
            m_combineflags      = obj.get_block_flags();
            m_block_types       = obj.get_header()->get_block_raw_types();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbindex_t, 1);
        }

        xvbindex_t::xvbindex_t(xvbindex_t && obj)
            :xvaccount_t(obj), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_bindex)
        {
            init();

            m_modified              = obj.m_modified;
            m_closed                = obj.m_closed;

            m_block_height          = obj.m_block_height;
            m_block_viewid          = obj.m_block_viewid;
            m_block_viewtoken       = obj.m_block_viewtoken;
            m_block_hash            = obj.m_block_hash;
            m_last_block_hash       = obj.m_last_block_hash;
            m_last_fullblock_hash   = obj.m_last_fullblock_hash;
            m_last_fullblock_height = obj.m_last_fullblock_height;

            m_next_viewid_offset    = obj.m_next_viewid_offset;

            m_parent_block_height   = obj.m_parent_block_height;
            m_parent_block_viewid   = obj.m_parent_block_viewid;
            m_parent_block_entity_id = obj.m_parent_block_entity_id;
            m_extend_cert           = obj.m_extend_cert;
            m_extend_data           = obj.m_extend_data;

            m_combineflags          = obj.m_combineflags;
            m_block_types           = obj.m_block_types;

            m_prev_index = obj.m_prev_index;
            m_next_index = obj.m_next_index;
            m_linked_block = obj.m_linked_block;

            obj.m_prev_index = NULL;
            obj.m_next_index = NULL;
            obj.m_linked_block = NULL;
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbindex_t, 1);
        }

        xvbindex_t::xvbindex_t(const xvbindex_t & obj)
            :xvaccount_t(obj), xstatistic::xstatistic_obj_face_t(obj)
        {
            init();
            *this = obj;
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbindex_t, 1);
        }

        xvbindex_t & xvbindex_t::operator = (const xvbindex_t & obj)
        {
            if(this == &obj) {
                return *this;
            }
            xvaccount_t::operator=(obj);

            m_modified              = obj.m_modified;
            m_closed                = obj.m_closed;

            m_block_height          = obj.m_block_height;
            m_block_viewid          = obj.m_block_viewid;
            m_block_viewtoken       = obj.m_block_viewtoken;
            m_block_hash            = obj.m_block_hash;
            m_last_block_hash       = obj.m_last_block_hash;
            m_last_fullblock_hash   = obj.m_last_fullblock_hash;
            m_last_fullblock_height = obj.m_last_fullblock_height;

            m_next_viewid_offset    = obj.m_next_viewid_offset;

            m_parent_block_height   = obj.m_parent_block_height;
            m_parent_block_viewid   = obj.m_parent_block_viewid;
            m_parent_block_entity_id = obj.m_parent_block_entity_id;
            m_extend_cert           = obj.m_extend_cert;
            m_extend_data           = obj.m_extend_data;

            m_combineflags          = obj.m_combineflags;
            m_block_types           = obj.m_block_types;

            //clean first
            if(m_prev_index != NULL)
                m_prev_index->release_ref();

            if(m_next_index != NULL)
                m_next_index->release_ref();

            if(m_linked_block != NULL)
                m_linked_block->release_ref();

            //then copy ptr
            m_prev_index = obj.m_prev_index;
            if(m_prev_index != NULL)
                m_prev_index->add_ref();

            m_next_index = obj.m_next_index;
            if(m_next_index != NULL)
                m_next_index->add_ref();

            m_linked_block = obj.m_linked_block;
            if(m_linked_block != NULL)
                m_linked_block->add_ref();

            return *this;
        }

        xvbindex_t::~xvbindex_t()
        {
            statistic_del();
            xdbg("xvbindex_t::destroy,dump(%s)",dump().c_str());

            if(m_prev_index != NULL)
                m_prev_index->release_ref();

            if(m_next_index != NULL)
                m_next_index->release_ref();

            if(m_linked_block != NULL)
                m_linked_block->release_ref();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbindex_t, -1);
        }

        void xvbindex_t::init()
        {
            m_version           = 1;
            m_modified          = 0;
            m_closed            = 0;
            m_prev_index        = NULL;
            m_next_index        = NULL;
            m_linked_block      = NULL;

            m_block_height      = 0;
            m_block_viewid      = 0;
            m_block_viewtoken   = 0;
            m_last_fullblock_height = 0;
            m_next_viewid_offset= 0;

            m_parent_block_height= 0;
            m_parent_block_viewid= 0;
            m_parent_block_entity_id = 0;

            m_combineflags      = 0;
            m_block_types       = 0;
        }

        bool   xvbindex_t::close()
        {
            m_closed = 1; //mark closed flag first
            if(m_next_index != NULL)//check whether has index point this
            {
                if(m_next_index->get_prev_block() == this)
                    m_next_index->reset_prev_block(NULL);
            }
            reset_prev_block(NULL);
            reset_next_block(NULL);
            reset_this_block(NULL);
            return true;
        }

        const std::string xvbindex_t::dump() const
        {
            char local_buf[256];
            // xprintf(local_buf,sizeof(local_buf),"{xvbindex_t:account_id(%" PRIu64 "),account_addr=%s,height=%" PRIu64 ",viewid=%" PRIu64 ",next_viewid(%" PRIu64 "),  parent_height(%" PRIu64 "),block-flags=0x%x,store-flags=0x%x,refcount=%d,this=%p}",get_xvid(),get_account().c_str(), m_block_height,m_block_viewid,get_next_viewid(),m_parent_block_height,get_block_flags(),get_store_flags(), get_refcount(),this);
            xprintf(local_buf,sizeof(local_buf),"{xvbindex_t:%s,height=%" PRIu64 ",viewid=%" PRIu64 ",hash=%s,next_viewid(%" PRIu64 "),flags=0x%x,0x%x,refcount=%d,this=%p}",
                get_account().c_str(),m_block_height,m_block_viewid,base::xstring_utl::to_hex(m_block_hash).c_str(),get_next_viewid(),get_block_flags(),get_store_flags(), get_refcount(),this);

            return std::string(local_buf);
        }

        //only allow reset it when index has empty address
        bool   xvbindex_t::reset_account_addr(const xvaccount_t & addr)
        {
            if(get_address().empty() == false)
            {
                if(get_address() != addr.get_address() )
                    xerror("xvbindex_t::reset_account_addr,try to overwrite exist addr(%s) by new(%s)",get_address().c_str(),addr.get_address().c_str());
                return false;
            }
            xvaccount_t::operator=(addr);
            return true;
        }

        void    xvbindex_t::reset_next_viewid_offset(const int32_t next_viewid_offset)
        {
            m_next_viewid_offset = next_viewid_offset;
            set_modified_flag();
        }

        void   xvbindex_t::set_modified_flag()
        {
            m_modified = 1;
        }

        void   xvbindex_t::reset_modify_flag()
        {
            m_modified = 0;
        }

        //[8bit:block-flags][1bit][7bit:store-bits]
        bool   xvbindex_t::check_block_flag(enum_xvblock_flag flag) const
        {
            const int copy_flags = m_combineflags;
            return ((copy_flags & flag) != 0);
        }

        int    xvbindex_t::set_block_flag(enum_xvblock_flag flag)
        {
            if(false == check_block_flag(flag)) //duplicated setting
            {
                const uint16_t copy_flags = m_combineflags;
                m_combineflags = (copy_flags | flag);
                set_modified_flag();
            }
            if(get_this_block() != NULL) //duplicated flag as well
                get_this_block()->set_block_flag(flag);

            return m_combineflags;
        }

        int    xvbindex_t::remove_block_flag(enum_xvblock_flag flag)
        {
            uint16_t copy_flags = m_combineflags;
            copy_flags &= (~flag);
            m_combineflags = copy_flags;
            set_modified_flag();

            return m_combineflags;
        }

        int    xvbindex_t::get_block_flags() const  //return all flags related block
        {
            return (m_combineflags & enum_xvblock_flags_mask);
        }

        int    xvbindex_t::reset_block_flags(const uint32_t new_flags) //replace all flags of block
        {
            uint16_t copy_flags = m_combineflags;
            copy_flags &= (~enum_xvblock_flags_mask);//0xFF00-->0x00FF,so clean all flags of block
            copy_flags |= (new_flags & enum_xvblock_flags_mask); //just keep flags of block(highest 8bit)
            m_combineflags = copy_flags;
            set_modified_flag();

            if(get_this_block() != NULL) //duplicated flags as well
                get_this_block()->reset_block_flags(new_flags);

            return m_combineflags;
        }

        //[8bit:block-flags][1bit][7bit:store-bits]
        bool    xvbindex_t::check_store_flag(enum_index_store_flag flag) const
        {
            const int copy_flags = m_combineflags;
            return ((copy_flags & flag) != 0);
        }

        bool    xvbindex_t::check_store_flags(const uint16_t flags) const
        {
            const int copy_flags = m_combineflags;
            return ((copy_flags & flags) != 0);
        }

        int    xvbindex_t::set_store_flag(enum_index_store_flag flag)
        {
            if(check_store_flag(flag)) //duplicated setting
                return m_combineflags;

            const uint16_t copy_flags = m_combineflags;
            m_combineflags = (copy_flags | flag);
            set_modified_flag();
            return m_combineflags;
        }

        int  xvbindex_t::remove_store_flag(enum_index_store_flag flag)
        {
            uint16_t copy_flags = m_combineflags;
            copy_flags &= (~flag);
            m_combineflags = copy_flags;
            set_modified_flag();
            return m_combineflags;
        }

        int     xvbindex_t::get_store_flags() const  //return all flags related index
        {
            return (m_combineflags & enum_index_store_flags_mask);//[8bit:block-flags][1bit][7bit:store-bits]
        }

        int     xvbindex_t::reset_store_flags(const uint32_t new_flags) //clean all flags related index
        {
            uint16_t copy_flags = m_combineflags;
            copy_flags &= (~enum_index_store_flags_mask); //clean stored flags at low 7bit. 0x7F -> 0xFF80
            copy_flags |= (new_flags & enum_index_store_flags_mask); //apply new flags
            m_combineflags = copy_flags;
            set_modified_flag();
            return m_combineflags;
        }

        void  xvbindex_t::set_block_character(base::enum_xvblock_character character)
        {
            uint16_t block_types = m_block_types;
            if((block_types & character) == 0)//if not found
            {
                block_types |= character;
                m_block_types = block_types; //reset back
                set_modified_flag();
            }
        }

        void  xvbindex_t::remove_block_character(base::enum_xvblock_character character)
        {
            uint16_t block_types = m_block_types;
            if((block_types & character) != 0)//if has falg already
            {
                block_types &= (~character);
                m_block_types = block_types; //reset back
                set_modified_flag();
            }
        }

        bool  xvbindex_t::reset_prev_block(xvbindex_t * _new_prev_index)//return false if hash or height not match
        {
            if(_new_prev_index == m_prev_index) //same one
                return true;

            if(_new_prev_index != NULL)
            {
                if(get_height() == (_new_prev_index->get_height() + 1) )
                {
                    if(_new_prev_index->get_block_hash() == get_last_block_hash())//verify hash
                    {
                        _new_prev_index->add_ref();
                        xvbindex_t * old_ptr =  xatomic_t::xexchange(m_prev_index, _new_prev_index);
                        if(old_ptr != NULL)
                            old_ptr->release_ref();

                        return true;
                    }
                    xinfo("xvbindex_t::reset_prev_block,get_last_block_hash() not match prev hash,prev_node->dump=%s vs this=%s",_new_prev_index->dump().c_str(),dump().c_str());
                }
                else
                {
                    xwarn("xvbindex_t::reset_prev_block,get_height() not match (parent height + 1),prev_node->dump=%s",_new_prev_index->dump().c_str());
                }
            }
            else
            {
                xvbindex_t * old_ptr = xatomic_t::xexchange(m_prev_index, (xvbindex_t*)NULL);
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                return true;
            }
            return false;
        }

        bool   xvbindex_t::reset_next_block(xvbindex_t * _new_next_index)//return false if hash or height not match
        {
            if(_new_next_index == m_next_index)
                return true;

            if(_new_next_index != NULL)
            {
                if(_new_next_index->get_height() == (get_height() + 1) )
                {
                    if(_new_next_index->get_last_block_hash() == get_block_hash())
                    {
                        _new_next_index->add_ref();
                        xvbindex_t * old_ptr =  xatomic_t::xexchange(m_next_index, _new_next_index);
                        if(old_ptr != NULL)
                            old_ptr->release_ref();

                        return true;
                    }
                    xerror("xvbindex_t::reset_next_block,next_block'get_last_block_hash() not match current hash,next_block->dump=%s",_new_next_index->dump().c_str());
                }
                else
                {
                    xerror("xvbindex_t::reset_next_block,next_block'height != (cur-height + 1),cur-height=%" PRIu64 " vs next_block->dump=%s",get_height(),_new_next_index->dump().c_str());
                }
            }
            else
            {
                xvbindex_t * old_ptr =  xatomic_t::xexchange(m_next_index, (xvbindex_t*)NULL);
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                return true;
            }
            return false;
        }

        bool     xvbindex_t::reset_this_block(xvblock_t* _block_ptr,bool delay_release_existing_one)
        {
            if(_block_ptr == m_linked_block)
                return true;

            if(_block_ptr != NULL)
            {
                if(get_height() == _block_ptr->get_height()) //verify height
                {
                    if(get_block_hash() == _block_ptr->get_block_hash())//verify hash
                    {
                        _block_ptr->add_ref();
                        xvblock_t * old_ptr =  xatomic_t::xexchange(m_linked_block, _block_ptr);
                        if(old_ptr != NULL)
                        {
                            if(delay_release_existing_one)
                                xcontext_t::instance().delay_release_object(old_ptr);
                            else
                                old_ptr->release_ref();
                        }
                        return true;
                    }
                    xerror("xvbindex_t::reset_this_block,get_block_hash() not match hash,block->dump=%s vs this=%s",_block_ptr->dump().c_str(),dump().c_str());
                }
                else
                {
                    xwarn("xvbindex_t::reset_this_block,get_height() not match height,block->dump=%s",_block_ptr->dump().c_str());
                }
            }
            else
            {
                xvblock_t * old_ptr =  xatomic_t::xexchange(m_linked_block, (xvblock_t*)NULL);
                if(old_ptr != NULL)//delay release to provide multiple-thread safe access
                {
                    if(delay_release_existing_one)
                        xcontext_t::instance().delay_release_object(old_ptr);
                    else
                        old_ptr->release_ref();
                }
                return true;
            }
            return false;
        }

        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t     xvbindex_t::serialize_to(xstream_t & stream)   //write whole object to binary
        {
            const int32_t begin_size = stream.size();

            if(m_version < 1) //old format
            {
                stream.write_compact_var(get_account());
                stream.write_compact_var(m_block_height);
                stream.write_compact_var(m_block_viewid);
                stream.write_compact_var(m_block_viewtoken);
                stream.write_tiny_string(m_block_hash);
                stream.write_tiny_string(m_last_block_hash);
                stream.write_tiny_string(m_last_fullblock_hash);
                stream.write_compact_var(m_last_fullblock_height);
                stream.write_compact_var(m_next_viewid_offset);

                stream.write_compact_var(m_parent_block_height);
                stream.write_compact_var(m_parent_block_viewid);
                stream.write_compact_var(m_parent_block_entity_id);
                stream.write_compact_var(m_extend_cert);
                stream.write_compact_var(m_extend_data);

                stream << m_combineflags;
                stream << m_block_types;
                stream.write_compact_var(m_reserved);
            }
            else //new format
            {
                std::string empty_addr;
                stream.write_compact_var(empty_addr);

                stream << m_version;
                stream << m_combineflags;
                stream << m_block_types;

                //note:to reduce size,new version NOT save account address & m_last_fullblock_hash
                stream.write_compact_var(m_block_height);
                stream.write_compact_var(m_block_viewid);
                stream.write_compact_var(m_block_viewtoken);
                stream.write_compact_var(m_block_hash);
                stream.write_compact_var(m_last_block_hash);
                stream.write_compact_var(m_last_fullblock_height);
                stream.write_compact_var(m_next_viewid_offset);

                stream.write_compact_var(m_parent_block_height);
                stream.write_compact_var(m_parent_block_viewid);
                stream.write_compact_var(m_parent_block_entity_id);
                stream.write_compact_var(m_extend_cert);
                stream.write_compact_var(m_extend_data);

                stream.write_compact_var(m_reserved);
            }

            return (stream.size() - begin_size);
        }

        int32_t     xvbindex_t::serialize_from(xstream_t & stream)    //read from binary and regeneate content of
        {
            const int32_t begin_size = stream.size();
            try
            {
                std::string account_addr;
                stream.read_compact_var(account_addr);
                if(account_addr.empty() == false) //old format
                {
                    stream.read_compact_var(m_block_height);
                    stream.read_compact_var(m_block_viewid);
                    stream.read_compact_var(m_block_viewtoken);
                    stream.read_tiny_string(m_block_hash);
                    stream.read_tiny_string(m_last_block_hash);
                    stream.read_tiny_string(m_last_fullblock_hash);
                    stream.read_compact_var(m_last_fullblock_height);
                    stream.read_compact_var(m_next_viewid_offset);

                    stream.read_compact_var(m_parent_block_height);
                    stream.read_compact_var(m_parent_block_viewid);
                    stream.read_compact_var(m_parent_block_entity_id);
                    stream.read_compact_var(m_extend_cert);
                    stream.read_compact_var(m_extend_data);

                    stream >> m_combineflags;
                    stream >> m_block_types;
                    stream.read_compact_var(m_reserved);

                    //finally reset account information
                    xvaccount_t::operator=(account_addr);
                }
                else //new format
                {
                    //NOTE: to reduce size,we opt out account from stram,so caller MUST reassign account_addr into xvbindex later
                    stream >> m_version;
                    stream >> m_combineflags;
                    stream >> m_block_types;
                    xassert(m_version == 1);

                    stream.read_compact_var(m_block_height);
                    stream.read_compact_var(m_block_viewid);
                    stream.read_compact_var(m_block_viewtoken);
                    stream.read_compact_var(m_block_hash);
                    stream.read_compact_var(m_last_block_hash);
                    stream.read_compact_var(m_last_fullblock_height);
                    stream.read_compact_var(m_next_viewid_offset);

                    stream.read_compact_var(m_parent_block_height);
                    stream.read_compact_var(m_parent_block_viewid);
                    stream.read_compact_var(m_parent_block_entity_id);
                    stream.read_compact_var(m_extend_cert);
                    stream.read_compact_var(m_extend_data);

                    stream.read_compact_var(m_reserved);
                }

            }catch (int error_code){
                xerror("xvbindex_t::serialize_from,throw exception with error:%d",error_code);
            }
            catch(enum_xerror_code err)
            {
                xerror("xvbindex_t::serialize_from,throw exception with xerror:%d",err);
            }
            catch(...)
            {
                xerror("xvbindex_t::serialize_from,throw unknow exception");
            }
            return (begin_size - stream.size());
        }

        int32_t  xvbindex_t::serialize_to(std::string & bin_data)  //write whole object to binary
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            const int result = serialize_to(_stream);
            if(result > 0)
                bin_data.assign((const char*)_stream.data(),_stream.size());

            return result;
        }

        int32_t  xvbindex_t::serialize_from(const std::string & bin_data)  //read from binary and
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            const int result = serialize_from(_stream);
            return result;
        }

        size_t xvbindex_t::get_object_size_real() const {
            size_t total_size = sizeof(*this) + get_size(m_block_hash) + get_size(m_last_block_hash) + get_size(m_last_fullblock_hash) + get_size(m_extend_cert) +
                                 get_size(m_extend_data) + get_size(m_reserved) + get_size(get_address()) + get_size(get_storage_key());
            xdbg("-----cache size----- xvbindex_t total_size:%zu this:%d,%d:%d:%d:%d:%d:%d:%d:%d",
                 total_size,
                 sizeof(*this),
                 get_size(m_block_hash),
                 get_size(m_last_block_hash),
                 get_size(m_last_fullblock_hash),
                 get_size(m_extend_cert),
                 get_size(m_extend_data),
                 get_size(m_reserved),
                 get_size(get_address()),
                 get_size(get_storage_key()));
            return total_size;
        }

        void  xvbindex_t::set_unitblock_on_index(base::xvblock_t* _unit)
        {
            set_store_flag(base::enum_index_store_flag_unit_in_index);

            std::string block_object_bin;
            _unit->serialize_to_string(block_object_bin);
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            _stream.write_compact_var(block_object_bin);
            _stream.write_compact_var(_unit->get_input_data());
            _stream.write_compact_var(_unit->get_output_data());
            std::string unit_bin = std::string((const char*)_stream.data(), _stream.size());
            set_unitblock_bin(unit_bin);
            return;
        }
        base::xauto_ptr<base::xvblock_t> xvbindex_t::create_unitblock_on_index()
        {
            if (false == check_store_flag(base::enum_index_store_flag_unit_in_index)) {
                xassert(false);
                return nullptr;
            }
            std::string const& unitblock_bin = get_unitblock_bin();
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)unitblock_bin.data(),(uint32_t)unitblock_bin.size());
            std::string block_object_bin;
            std::string input_data;
            std::string output_data;
            _stream.read_compact_var(block_object_bin);
            _stream.read_compact_var(input_data);
            _stream.read_compact_var(output_data);
            base::xauto_ptr<base::xvblock_t> new_block_ptr(base::xvblock_t::create_block_object(block_object_bin));
            if (new_block_ptr != nullptr) {
                new_block_ptr->set_input_data(input_data, false);
                new_block_ptr->set_output_data(output_data, false);
            }
            if (check_block_flag(base::enum_xvblock_flag_committed))
                new_block_ptr->set_block_flag(base::enum_xvblock_flag_committed);
            return new_block_ptr;
        }

        xvbnode_t::xvbnode_t(xvbnode_t * parent_node,xvblock_t & raw_block)
        {
            m_parent_node = NULL;
            m_child_node  = NULL;
            m_block  = NULL;

            raw_block.add_ref();
            m_block = &raw_block;

            if(parent_node != NULL)
            {
                //basic mode may link two block with same height and viewid while consensusing,but once commit that should not happen
                if( (raw_block.get_height() == (parent_node->get_height() + 1)) || (raw_block.get_height() == parent_node->get_height()))
                {
                    if(parent_node->get_hash() == raw_block.get_last_block_hash())
                    {
                        parent_node->add_ref();
                        m_parent_node = parent_node;
                    }
                    else
                    {
                        xassert(0);//has must match first
                    }
                }
                else
                {
                    xassert(0);//has must match first
                }
            }
        }

        xvbnode_t::~xvbnode_t()
        {
            if(m_child_node != NULL)
                m_child_node->release_ref();

            if(m_parent_node != NULL)
                m_parent_node->release_ref();

            if(m_block != NULL)
                m_block->release_ref();
        }

        std::string xvbnode_t::dump() const  //just for debug purpose
        {
            return m_block->dump();
        }

        bool xvbnode_t::reset_child(xvbnode_t * _new_node_ptr)
        {
            if(_new_node_ptr != NULL)
                _new_node_ptr->add_ref();

            xvbnode_t * old_ptr =  xatomic_t::xexchange(m_child_node, _new_node_ptr) ;
            if(old_ptr != NULL)
                old_ptr->release_ref();

            return true;
        }

        bool    xvbnode_t::reset_block(xvblock_t *  new_block_ptr) //replace it with newer viewid
        {
            if (new_block_ptr != nullptr) {
                new_block_ptr->add_ref();
            }

            xvblock_t * old_block = xatomic_t::xexchange(m_block, new_block_ptr);
            if(old_block != nullptr)
                old_block->release_ref();
            return true;
        }

        //return the ptr of target node that hold this height
        //just search by hegit
        xvbnode_t* xvbnode_t::find_node(const uint64_t target_height)
        {
            if(is_close())
                return NULL;

            const uint64_t this_height = get_height();
            if(target_height < this_height)
            {
                if(m_parent_node != NULL)
                    return m_parent_node->find_node(target_height);
            }
            else if(target_height == this_height)
            {
                add_ref();
                return this;
            }
            else if(target_height == (this_height + 1))
            {
                if(m_child_node != NULL)
                    m_child_node->add_ref();

                return m_child_node;
            }
            else if(m_child_node != NULL)
            {
                return m_child_node->find_node(target_height);
            }
            return NULL;
        }

        xvbnode_t*  xvbnode_t::find_node(const uint64_t target_height,const uint64_t target_viewid) //note:must release the retured ptr after use
        {
            if(is_close())
                return NULL;

            const uint64_t this_height = get_height();
            if(target_height < this_height)
            {
                if(m_parent_node != NULL)
                    return m_parent_node->find_node(target_height);
            }
            else if(target_height == this_height)
            {
                if(target_viewid == get_viewid())
                {
                    add_ref();
                    return this;
                }
            }
            else if(target_height == (this_height + 1))
            {
                if( (m_child_node != NULL) && (m_child_node->get_viewid() == target_viewid))
                {
                    m_child_node->add_ref();
                    return m_child_node;
                }
            }
            else if(m_child_node != NULL)
            {
                return m_child_node->find_node(target_height);
            }
            return NULL;
        }

        bool       xvbnode_t::reset_parent(xvbnode_t * _new_parent_node) //return false if already attached one
        {
            if(is_close())
                return false;

            if(_new_parent_node == m_parent_node) //same one
                return true;

            if(_new_parent_node != NULL)
            {
                if(get_height() ==  (_new_parent_node->get_height() + 1) )
                {
                    if(_new_parent_node->get_hash() == get_block()->get_last_block_hash())//verify hash
                    {
                        if(_new_parent_node != NULL)
                            _new_parent_node->add_ref();

                        xvbnode_t * old_ptr =  xatomic_t::xexchange(m_parent_node, _new_parent_node) ;
                        if(old_ptr != NULL)
                            old_ptr->release_ref();

                        return true;
                    }
                    xwarn("xvbnode_t::reset_parent,get_last_block_hash() not match parent hash,parent_node->dump=%s vs this=%s",_new_parent_node->dump().c_str(),dump().c_str());
                }
                else
                {
                    xwarn("xvbnode_t::reset_parent,get_height() not match (parent height + 1),parent_node->dump=%s",_new_parent_node->dump().c_str());
                }
            }
            else
            {
                xvbnode_t * old_ptr =  xatomic_t::xexchange(m_parent_node, _new_parent_node) ;
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                return true;
            }
            return false;
        }

        xvbnode_t* xvbnode_t::attach_child(base::xvblock_t* new_block)
        {
            if( is_close() || (NULL == new_block) )
                return NULL;

            const uint64_t new_height  = new_block->get_height();
            const uint64_t this_height = get_height();
            if(new_height < this_height)//too old
            {
                if(m_parent_node != NULL)
                    return m_parent_node->attach_child(new_block);

    #ifdef DEBUG
                xdbg("xvbnode_t::attach_child,this_height > new_block'heigh,this-node=%s vs new_block=%s",get_block()->dump().c_str(),new_block->dump().c_str());
    #endif
                return NULL;
            }
            else if(new_height == this_height)
            {
                if(get_viewid() < new_block->get_viewid())
                    reset_block(new_block);

                add_ref();
                return this;
            }
            else if(new_height == (this_height + 1)) //directly child node
            {
                //verify hash first
                if(new_block->get_last_block_hash() == get_hash())
                {
                    if(m_child_node != NULL)
                    {
                        if(m_child_node->get_viewid() < new_block->get_viewid()) //replace old with newer view id
                        {
    #ifdef DEBUG
                            xdbg("xvbnode_t::attach_child,newer block to replace existing,this-node=%s vs new_block=%s",get_block()->dump().c_str(),new_block->dump().c_str());
    #endif
                            m_child_node->reset_block(new_block);
                        }
                        m_child_node->add_ref();
                        return m_child_node;
                    }
                    xvbnode_t * _added_node = new xvbnode_t(this,*new_block);
                    reset_child(_added_node);//may hold addtional reference
                    return _added_node;
                }
                xerror("xvbnode_t::attach_child,new_block->get_last_block_hash is not match parent one,new_block->dump=%s",new_block->dump().c_str());
            }
            else if(m_child_node != NULL)
            {
                xvbnode_t * result = m_child_node->attach_child(new_block);
                if(result != NULL)
                    return result;
            }
            return NULL;
        }

        xvbnode_t*   xvbnode_t::detach_child(const uint64_t height ,const uint64_t viewid) //caller should release to_detach_node after detach successful
        {
            if(is_close())
                return NULL;

            const uint64_t this_height = get_height();
            if(height <= this_height)
            {
                if(m_parent_node != NULL)
                    return m_parent_node->detach_child(height,viewid);
            }
            else if(height == (this_height + 1) )
            {
                if( (m_child_node != NULL) && (m_child_node->get_viewid() == viewid))
                {
                    xvbnode_t* old_ptr = xatomic_t::xexchange(m_child_node, (xvbnode_t*)NULL);
                    return old_ptr;
                }
            }
            else if(m_child_node != NULL)
            {
                return m_child_node->detach_child(height, viewid);
            }
            return NULL;
        }

        bool   xvbnode_t::close(bool force_async)
        {
            if(is_close() == false)
            {
                xobject_t::close(); //prevent re-enter

                xvbnode_t* _child_ptr = xatomic_t::xexchange(m_child_node, (xvbnode_t*)NULL);
                if(_child_ptr != NULL)
                {
                    //_child_ptr->reset_parent(NULL);
                    _child_ptr->release_ref();
                }
                reset_parent(NULL);
            }
            return true;
        }

        bool   xvbnode_t::close_all()
        {
            if(is_close() == false)
            {
                xobject_t::close(); //prevent re-enter

                xvbnode_t* _child_ptr = xatomic_t::xexchange(m_child_node, (xvbnode_t*)NULL);
                if(_child_ptr != NULL)
                {
                    _child_ptr->close_all();
                    _child_ptr->release_ref();
                }
                reset_parent(NULL);
            }
            return true;
        }

        xvbmnode_t::xvbmnode_t(xvbnode_t * parent_node,xvblock_t & raw_block)
        :xvbnode_t(parent_node,raw_block)
        {
        }

        xvbmnode_t::~xvbmnode_t()
        {
            auto backup_map = m_child_nodes;
            m_child_nodes.clear();

            for(auto it = backup_map.begin(); it != backup_map.end(); ++it)
            {
                xvbnode_t * _child_node = it->second;
                if(_child_node != NULL)
                {
                    _child_node->close(false);
                    _child_node->release_ref();
                }
            }
        }

        //just search by hegit
        xvbnode_t*  xvbmnode_t::find_node(const uint64_t target_height)//note:must release the retured ptr after use
        {
            if(is_close())
                return NULL;

            const uint64_t this_height = get_height();
            if(target_height < this_height)
            {
                if(get_parent() != NULL)
                    return get_parent()->find_node(target_height);
            }
            else if(target_height == this_height)
            {
                add_ref();
                return this;
            }
            else if(target_height == (this_height + 1)) //directly child node
            {
                if(m_child_nodes.empty() == false)//found first child with highest viewid
                {
                    auto it = m_child_nodes.begin();
                    it->second->add_ref();
                    return it->second;
                }
            }
            else
            {
                for(auto it = m_child_nodes.begin(); it != m_child_nodes.end(); ++it)
                {
                    xvbnode_t* result = it->second->find_node(target_height);
                    if(result != NULL)
                        return result;
                }
            }
            return NULL;
        }

        //return the ptr of target node that hold this height
        xvbnode_t*  xvbmnode_t::find_node(const uint64_t target_height,const uint64_t target_viewid)
        {
            if(is_close())
                return NULL;

            const uint64_t this_height = get_height();
            if(target_height < this_height)
            {
                if(get_parent() != NULL)
                    return get_parent()->find_node(target_height,target_viewid);
            }
            else if(target_height == this_height)
            {
                if(target_viewid == get_viewid())
                {
                    add_ref();
                    return this;
                }
            }
            else if(target_height == (this_height + 1)) //directly child node
            {
                auto it = m_child_nodes.find(target_viewid);
                if(it != m_child_nodes.end())
                {
                    it->second->add_ref();
                    return it->second;
                }
            }
            else
            {
                for(auto it = m_child_nodes.begin(); it != m_child_nodes.end(); ++it)
                {
                    xvbnode_t* result = it->second->find_node(target_height,target_viewid);
                    if(result != NULL)
                        return result;
                }
            }
            return NULL;
        }

        //length = child_node'height - parent_node'height
        xvbnode_t*  xvbmnode_t::attach_child(base::xvblock_t* new_block)
        {
            if( is_close() || (NULL == new_block) )
                return NULL;

            const uint64_t new_height  = new_block->get_height();
            const uint64_t this_height = get_height();
            if(new_height < this_height)//too old
            {
                if(get_parent() != NULL)
                    return get_parent()->attach_child(new_block);

    #ifdef DEBUG
                xdbg("xvbmnode_t::attach_child,this_height > new_block'heigh,this-node=%s vs new_block=%s",get_block()->dump().c_str(),new_block->dump().c_str());
    #endif
                return NULL;
            }
            else if(new_height == this_height)
            {
                if(get_viewid() < new_block->get_viewid()) //shoud not happen for xvbmnode_t
                    reset_block(new_block);

                add_ref();
                return this;
            }
            else if(new_height == (this_height + 1)) //directly child node
            {
                //verify hash first
                if(new_block->get_last_block_hash() == get_hash())
                {
                    auto it = m_child_nodes.find(new_block->get_viewid());
                    if(it != m_child_nodes.end()) //found existing one block
                    {
                        it->second->add_ref(); //duplicated one;
                        xdbg("xvbmnode_t::attach_child,a duplicted block->dump=%s",new_block->dump().c_str());
                        return it->second;
                    }
                    else //insert into childs
                    {
                        xvbnode_t * new_node = new xvbmnode_t(this,*new_block);
                        m_child_nodes[new_block->get_viewid()] = new_node;
                        new_node->add_ref();
                        return new_node;
                    }
                }
                xerror("xvbmnode_t::attach_child,new_block->get_last_block_hash is not match parent one,new_block->dump=%s",new_block->dump().c_str());
            }
            else //search from next level
            {
                //higher than current level ,go deeper
                for(auto it = m_child_nodes.begin(); it != m_child_nodes.end(); ++it)
                {
                    xvbnode_t* result = it->second->attach_child(new_block);
                    if(result != NULL)
                        return result;
                }
            }
            return NULL;
        }

        xvbnode_t*  xvbmnode_t::detach_child(const uint64_t height ,const uint64_t viewid) //caller should release to_detach_node after detach successful
        {
            if(is_close())
                return NULL;

            const uint64_t this_height = get_height();
            if(height <= this_height)
            {
                if(get_parent() != NULL)
                    return get_parent()->detach_child(height,viewid);
            }
            else if(height == (this_height + 1)) //found
            {
                auto it = m_child_nodes.find(viewid);
                if(it != m_child_nodes.end())
                {
                    xvbnode_t * detached_node = it->second;
                    xassert(detached_node->get_height() == height);
                    m_child_nodes.erase(it);
                    return detached_node;
                }
            }
            else
            {
                for(auto it = m_child_nodes.begin(); it != m_child_nodes.end(); ++it)
                {
                    xvbnode_t * detached_node = it->second->detach_child(height, viewid);
                    if(detached_node != NULL)
                        return detached_node;
                }
            }
            return NULL;
        }

        bool   xvbmnode_t::close(bool force_async)
        {
            if(is_close() == false)
            {
                xvbnode_t::close(); //prevent re-enter

                auto backup_map = m_child_nodes;
                m_child_nodes.clear();

                for(auto it = backup_map.begin(); it != backup_map.end(); ++it)
                {
                    xvbnode_t * _child_node = it->second;
                    if(_child_node != NULL)
                    {
                        _child_node->reset_parent(NULL);
                        _child_node->release_ref();
                    }
                }
                reset_parent(NULL);
            }
            return true;
        }

        bool   xvbmnode_t::close_all()
        {
            if(is_close() == false)
            {
                xvbnode_t::close(); //prevent re-enter

                auto backup_map = m_child_nodes;
                m_child_nodes.clear();

                for(auto it = backup_map.begin(); it != backup_map.end(); ++it)
                {
                    xvbnode_t * _child_node = it->second;
                    if(_child_node != NULL)
                    {
                        _child_node->close_all();
                        _child_node->release_ref();
                    }
                }
                reset_parent(NULL);
            }
            return true;
        }

        //vblock tree to manage nodes
        //note:multiple-threads unsafe for the below operations
        class xvbtree_t : public xobject_t
        {
        public:
            xvbtree_t();
            virtual ~xvbtree_t();
        private:
            xvbtree_t(const xvbtree_t &);
            xvbtree_t & operator = (const xvbtree_t &);
        public:
            //new_block must pass test of is_deliver()
            virtual xvbnode_t*  attach_node(base::xvblock_t* new_block);//note:must release the retured ptr after use
            virtual xvbnode_t*  find_node(const std::string & block_hash);//note:must release the retured ptr after use

            virtual bool        close(bool force_async = true) override;//close and release current node and every child node...

            //virtual xvbnode_t*  find_longest_branch() override; //return the node with branch,note:must release the retured ptr after use
        protected:
            inline xvbnode_t*   get_root() const {return m_root;}
        private:
            xvbnode_t*                        m_root;       //root of tree
            std::map<std::string,xvbnode_t*>  m_hash_pool;  //mapping block-hash ->node
        };

        //new_block must pass test of is_deliver
        xvbnode_t*  xvbtree_t::attach_node(base::xvblock_t* new_block)//note:must release the retured ptr after use
        {
            if((NULL == new_block) || is_close() )
                return NULL;

            if(new_block->is_deliver(false) == false)
            {
                xerror("xvbtree_t::attach_node,undelivered block=%s",new_block->dump().c_str());
                return NULL;
            }

            const std::string & target_block_hash = new_block->get_block_hash();
            auto existing = m_hash_pool.find(target_block_hash);
            if(existing != m_hash_pool.end())
            {
                existing->second->add_ref();
                return existing->second;
            }

            const std::string & target_parent_hash = new_block->get_last_block_hash();
            auto it = m_hash_pool.find(target_parent_hash);//most case hit here
            if(it != m_hash_pool.end()) //found parent node
            {
                xvbnode_t * result = it->second->attach_child(new_block);
                if(result != NULL)
                {
                    result->add_ref();
                    m_hash_pool[target_block_hash] = result;
                }
                return result;
            }

            //create node first
            xvbmnode_t * new_node = new xvbmnode_t(NULL,*new_block);
            for(auto i = m_hash_pool.begin(); i != m_hash_pool.end(); ++i)
            {
                if(target_block_hash == i->second->get_block()->get_last_block_hash()) //found child node
                {
                    if(i->second->get_parent() == NULL)
                    {
                        i->second->reset_parent(new_node);
                    }
                    else
                    {
                        xassert(0);//should not happen
                    }
                }
            }
            //finally put into pool
            m_hash_pool[target_block_hash] = new_node;
            new_node->add_ref();

            if(m_root == NULL)
            {
                m_root = new_node;
                m_root->add_ref();
            }
            else
            {
                if(new_node->get_height() < m_root->get_height())
                {
                    m_root->release_ref();
                    m_root = new_node;
                    m_root->add_ref();
                }
            }
            return new_node;
        }

        xvbnode_t*  xvbtree_t::find_node(const std::string & block_hash)
        {
            auto existing = m_hash_pool.find(block_hash);
            if(existing != m_hash_pool.end())
            {
                existing->second->add_ref();
                return existing->second;
            }
            return NULL;
        }

    };//end of namespace of base
};//end of namespace of top
