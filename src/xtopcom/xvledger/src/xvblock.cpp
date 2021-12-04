// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <limits.h>
#include <cinttypes>
#include "xbase/xutl.h"
#include "../xvstate.h"
#include "../xvblock.h"
#include "xmetrics/xmetrics.h"

#ifdef DEBUG
    #define __DEBUG_BLOCK_CONTENT__
#endif

namespace top
{
    namespace base
    {
        //////////////////////////////////xvblock and related implementation /////////////////////////////
        int32_t xvheader_extra::serialize_to_string(std::string & str) const {
            base::xstream_t _stream(base::xcontext_t::instance());
            auto size = do_write(_stream);
            str.clear();
            str.assign((const char*)_stream.data(), _stream.size());
            return str.size();
        }

        int32_t xvheader_extra::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            stream << static_cast<uint32_t>(m_map.size());
            for (auto pair : m_map) {
                stream.write_compact_var(pair.first);
                stream.write_compact_var(pair.second);
            }
            return (stream.size() - begin_size);
        }

        int32_t xvheader_extra::serialize_from_string(const std::string & _data) {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = do_read(_stream);
            return result;
        }

        int32_t xvheader_extra::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint32_t size;
            stream >> size;
            for (uint32_t i = 0; i < size; ++i) {
                std::string key;
                std::string val;
                stream.read_compact_var(key);
                stream.read_compact_var(val);
                m_map[key] = val;
            }
            return (begin_size - stream.size());
        }

        void xvheader_extra::insert(const std::string & key, const std::string & val) {
            m_map[key] = val;
        }

        std::string xvheader_extra::get_val(const std::string & key) const {
            auto it = m_map.find(key);
            if (it != m_map.end()) {
                return it->second;
            } else {
                return "";
            }
        }

        xvheader_t::xvheader_t()  //just use when seralized from db/store
            :xobject_t(enum_xobject_type_vheader)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvheader, 1);
            m_types     = 0;
            m_versions  = 1 << 8;//[8:features][8:major][8:minor][8:patch]
            m_chainid   = 0;
            m_height    = 0;
            m_weight    = 1;
            m_last_full_block_height = 0;
        }
        
        xvheader_t::xvheader_t(const std::string & intput_hash,const std::string & output_hash)
            :xobject_t(enum_xobject_type_vheader)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvheader, 1);
            m_types     = 0;
            m_versions  = 1 << 8;//[8:features][8:major][8:minor][8:patch]
            m_chainid   = 0;
            m_height    = 0;
            m_weight    = 1;
            m_last_full_block_height = 0;
            
            m_input_hash  = intput_hash;
            m_output_hash = output_hash;
        }
        
        xvheader_t::~xvheader_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvheader, -1);
        }
        
        xvheader_t::xvheader_t(const xvheader_t & other)
            :xobject_t(enum_xobject_type_vheader)
        {
            *this = other;
        }
        
        xvheader_t & xvheader_t::operator = (const xvheader_t & other)
        {
            m_types             = other.m_types;
            m_versions          = other.m_versions;
            m_chainid           = other.m_chainid;
            m_height            = other.m_height;
            m_weight            = other.m_weight;
            m_account           = other.m_account;
            m_comments          = other.m_comments;
            m_input_hash        = other.m_input_hash;
            m_output_hash       = other.m_output_hash;
            m_last_block_hash   = other.m_last_block_hash;
            m_last_full_block_hash  = other.m_last_full_block_hash;
            m_last_full_block_height= other.m_last_full_block_height;
            m_extra_data            = other.m_extra_data;
     
            return *this;
        }
    
        xauto_ptr<xvheader_t>  xvheader_t::clone() const
        {
            return new xvheader_t(*this);
        }
        
        void*  xvheader_t::query_interface(const int32_t _enum_xobject_type_)  //caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vheader)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
        
        bool  xvheader_t::is_valid() const
        {
            if(0 == m_height) //genesis block
            {
                if(get_block_type() != enum_xvblock_type_genesis)
                    return false;
                
                //geneis block allow empty block
                if(   (m_types      == 0)
                   || (m_versions   == 0)
                   || (get_weight() == 0)
                   || (get_account().empty())
                   || (m_last_full_block_height != 0)          //must be 0
                   )
                {
                    xerror("xvheader_t::is_valid,some property not set correct for genesis block");
                    return false;
                }
            }
            else //non-genesis block
            {
                xdbgassert(get_block_type() != enum_xvblock_type_genesis);
                if((m_types    == 0)
                   || (m_versions   == 0)
                   || (get_weight() == 0)
                   || (get_account().empty())
                   || (get_last_block_hash().empty())
                   || (get_block_type() == enum_xvblock_type_genesis)
                   )
                {
                    xerror("xvheader_t::is_valid,some property not set correct");
                    return false;
                }
                if(get_last_full_block_hash().empty())
                {
                    xerror("xvheader_t::is_valid,last_full_block_hash and last_full_block_height must set as valid value");
                    return false;
                }
            }
            
            //255 are big enough to hold any hash result
            if(   (get_input_hash().size()    > 255)
               || (get_output_hash().size()   > 255)
               || (get_last_block_hash().size()     > 255)
               || (get_last_full_block_hash().size()> 255)
               )
            {
                xerror("xvheader_t::is_valid,invalid hash of size > 255");
                return false;
            }
            
            if(get_block_characters() & enum_xvblock_character_certify_header_only)//carry input/output hash
            {
                if(get_block_class() == enum_xvblock_class_nil)
                {
                    if( (false == get_input_hash().empty()) || (false == get_output_hash().empty() ) )
                    {
                        xerror("xvheader_t::is_valid,a nil block must have empty input & output hash");
                        return false;
                    }
                }
                else if(get_input_hash().empty() || get_output_hash().empty())
                {
                    xerror("xvheader_t::is_valid,a non-nil block must have valid input & output hash");
                    return false;
                }
            }
            else //input/output hash must be empty for this mode to save space
            {
                if( (false == get_input_hash().empty()) || (false == get_output_hash().empty() ) )
                {
                    xerror("xvheader_t::is_valid,block of character force empty input & output hash");
                    return false;
                }
            }
            return true;
        }
        
        bool  xvheader_t::is_equal(const xvheader_t & other) const //test without considering m_certification
        {
            if(   ( m_types         != other.m_types )
               || ( m_versions      != other.m_versions)
               || ( m_chainid       != other.m_chainid)
               || ( get_height()    != other.get_height())
               || ( get_weight()    != other.get_weight())
               || ( get_account()   != other.get_account())
               || ( get_comments()  != other.get_comments())
               || ( get_input_hash()          != other.get_input_hash())
               || ( get_output_hash()         != other.get_output_hash())
               || ( get_extra_data()          != other.get_extra_data())
               || ( get_last_block_hash()     != other.get_last_block_hash()  )
               || ( get_last_full_block_hash()!= other.get_last_full_block_hash())
               || ( m_last_full_block_height  != other.m_last_full_block_height)
               )
            {
                return false;
            }
            return true;
        }
    
        int32_t   xvheader_t::serialize_to_string(std::string & bin_data)
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            const int result = serialize_to(_stream);
            if(result > 0)
                bin_data.assign((const char*)_stream.data(),_stream.size());
            
            return result;
        }
        
        int32_t   xvheader_t::serialize_to(xstream_t & stream)
        {
            return do_write(stream);
        }
    
        int32_t   xvheader_t::serialize_from_string(const std::string & bin_data) //wrap function fo serialize_from(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            const int result = serialize_from(_stream);
            return result;
        }
            
        int32_t   xvheader_t::serialize_from(xstream_t & stream)//not allow subclass change behavior
        {
            return do_read(stream);
        }
     
        //not safe for multiple threads, do_write & do_read write and read raw data of dataobj
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t  xvheader_t::do_write(xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            //new compact mode
            {
                stream << m_types;
                stream << m_versions;
                stream.write_compact_var(m_chainid);
                stream.write_compact_var(m_height);
                stream.write_compact_var(m_weight);
                stream.write_compact_var(m_last_full_block_height);
                
                stream.write_tiny_string(m_account);
                stream.write_tiny_string(m_comments);
                stream.write_tiny_string(m_input_hash);
                stream.write_tiny_string(m_output_hash);
                stream.write_tiny_string(m_last_block_hash);
                stream.write_tiny_string(m_last_full_block_hash);
                
                stream.write_compact_var(m_extra_data);
            }

            return (stream.size() - begin_size);
        }
        
        int32_t  xvheader_t::do_read(xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            //new compact mode
            {
                stream >> m_types;
                stream >> m_versions;
                stream.read_compact_var(m_chainid);
                stream.read_compact_var(m_height);
                stream.read_compact_var(m_weight);
                stream.read_compact_var(m_last_full_block_height);
                
                stream.read_tiny_string(m_account);
                stream.read_tiny_string(m_comments);
                stream.read_tiny_string(m_input_hash);
                stream.read_tiny_string(m_output_hash);
                stream.read_tiny_string(m_last_block_hash);
                stream.read_tiny_string(m_last_full_block_hash);
                
                stream.read_compact_var(m_extra_data);
            }
         
            return (begin_size - stream.size());
        }
        
        //---------------------------------xvqcert_t---------------------------------//
        xvqcert_t::xvqcert_t()
        : xdataunit_t((enum_xdata_type)enum_xobject_type_vqccert)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvqcert, 1);
            m_viewid    = 0;
            m_view_token= 0;
            m_clock     = 0;
            m_drand_height = 0;
            m_parent_height = 0;
            m_parent_viewid = 0;
            m_expired   = (uint32_t)-1;
            m_relative_gmtime = 0;
            m_validator.low_addr    = 0;
            m_validator.high_addr   = 0;
            m_auditor.low_addr      = 0;
            m_auditor.high_addr     = 0;
            m_consensus = 0;
            m_cryptos   = 0;
            m_modified_count = 0;
            m_nonce      = (uint64_t)-1;
            m_view_token = xtime_utl::get_fast_randomu();
            set_gmtime(xtime_utl::gettimeofday());
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        
        xvqcert_t::xvqcert_t(const std::string header_hash,enum_xdata_type type)
        : xdataunit_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvqcert, 1);
            m_viewid    = 0;
            m_view_token= 0;
            m_clock     = 0;
            m_drand_height = 0;
            m_parent_height = 0;
            m_parent_viewid = 0;
            m_expired   = (uint32_t)-1;
            m_relative_gmtime = 0;
            m_validator.low_addr    = 0;
            m_validator.high_addr   = 0;
            m_auditor.low_addr      = 0;
            m_auditor.high_addr     = 0;
            m_consensus = 0;
            m_cryptos   = 0;
            m_modified_count = 0;
            
            m_header_hash = header_hash;
            m_nonce      = (uint64_t)-1;
            m_view_token = xtime_utl::get_fast_randomu();
            set_gmtime(xtime_utl::gettimeofday());
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        
        xvqcert_t::xvqcert_t(const xvqcert_t & other,enum_xdata_type type)
        : xdataunit_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvqcert, 1);
            m_viewid    = 0;
            m_view_token= 0;
            m_clock     = 0;
            m_drand_height = 0;
            m_parent_height = 0;
            m_parent_viewid = 0;
            m_expired   = (uint32_t)-1;
            m_relative_gmtime = 0;
            m_validator.low_addr    = 0;
            m_validator.high_addr   = 0;
            m_auditor.low_addr      = 0;
            m_auditor.high_addr     = 0;
            m_consensus = 0;
            m_cryptos   = 0;
            m_modified_count = 0;
            m_nonce      = (uint64_t)-1;
            m_view_token = xtime_utl::get_fast_randomu();
            set_gmtime(xtime_utl::gettimeofday());
            
            *this = other;
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        xvqcert_t::~xvqcert_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvqcert, -1);
        }
        
        xvqcert_t & xvqcert_t::operator = (const xvqcert_t & other)
        {
            m_parent_viewid     = other.m_parent_viewid;
            m_parent_height     = other.m_parent_height;
            m_view_token        = other.m_view_token;
            m_viewid            = other.m_viewid;
            m_drand_height      = other.m_drand_height;
            m_clock             = other.m_clock;
            m_expired           = other.m_expired;
            m_validator         = other.m_validator;
            m_auditor           = other.m_auditor;
            m_nonce             = other.m_nonce;
            m_consensus         = other.m_consensus;
            m_cryptos           = other.m_cryptos;
            m_relative_gmtime   = other.m_relative_gmtime;
 
            m_header_hash       = other.m_header_hash;
            m_input_root_hash   = other.m_input_root_hash;
            m_output_root_hash  = other.m_output_root_hash;
            m_justify_cert_hash = other.m_justify_cert_hash;
            
            m_verify_signature  = other.m_verify_signature;
            m_audit_signature   = other.m_audit_signature;
            m_extend_cert       = other.m_extend_cert;
            m_extend_data       = other.m_extend_data;
  
            m_modified_count    = other.m_modified_count;
            return *this;
        }
    
        xauto_ptr<xvqcert_t> xvqcert_t::clone() const
        {
            return new xvqcert_t(*this);
        }
        
        std::string   xvqcert_t::dump() const  //just for debug purpose
        {
            char local_param_buf[256];
            #ifdef DEBUG
            xprintf(local_param_buf,sizeof(local_param_buf),"{xvqcert:viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 ",validator=0x%" PRIx64 " : %" PRIx64 ",auditor=0x%" PRIx64 " : %" PRIx64 ",parent_height=%" PRIu64 ",refcount=%d,this=%" PRIu64 ",consensus_flags=%x}",get_viewid(),get_viewtoken(),get_clock(),get_validator().high_addr,get_validator().low_addr,get_auditor().high_addr,get_auditor().low_addr,m_parent_height,get_refcount(),(uint64_t)this,get_consensus_flags());
            #else
            
                xprintf(local_param_buf,sizeof(local_param_buf),"{xvqcert:viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 ",validator=0x%" PRIx64 " : %" PRIx64 ",auditor=0x%" PRIx64 " : %" PRIx64 ",consensus_flags=%x}",get_viewid(),get_viewtoken(),get_clock(),get_validator().high_addr,get_validator().low_addr,get_auditor().high_addr,get_auditor().low_addr,get_consensus_flags());
            
            #endif
            
            return std::string(local_param_buf);
        }
        
        const std::string  xvqcert_t::hash(const std::string & input) const //hash by the hash_type of qcert
        {
            if(input.empty())
                return std::string();
            
            return xcontext_t::instance().hash(input, get_crypto_hash_type());
        }
        
        void*   xvqcert_t::query_interface(const int32_t _enum_xobject_type_) //caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vqccert)
                return this;
            
            return xdataunit_t::query_interface(_enum_xobject_type_);
        }
        
        uint32_t  xvqcert_t::add_modified_count()
        {
            return xatomic_t::xadd(m_modified_count);
        }
        
        void     xvqcert_t::reset_modified_count()
        {
            xatomic_t::xreset(m_modified_count);
        }
        
        void     xvqcert_t::reset_block_flags() //clean all flags related block
        {
            reset_all_unit_flags((get_unit_flags() & enum_xdata_flags_mask)); //block flags just use high 8bit
        }
        
        void    xvqcert_t::set_parent_height(const uint64_t parent_block_height)
        {
            if(is_allow_modify())
            {
                m_parent_height = parent_block_height;
                add_modified_count();
                return;
            }
            xassert(0);
        }
    
        void    xvqcert_t::set_parent_viewid(const uint64_t parent_block_viewid)
        {
            if(is_allow_modify())
            {
                m_parent_viewid = parent_block_viewid;
                add_modified_count();
                return;
            }
            xassert(0);
        }
    
        void   xvqcert_t::set_drand(const uint64_t global_drand_height)
        {
            if(is_allow_modify())
            {
                m_drand_height = global_drand_height;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void    xvqcert_t::set_clock(const  uint64_t global_clock_round)
        {
            if(is_allow_modify())
            {
                m_clock  = global_clock_round;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void    xvqcert_t::set_expired(const  uint64_t global_clock_round)
        {
            if(is_allow_modify())
            {
                if(global_clock_round > m_clock)
                {
                    const uint64_t duration = global_clock_round - m_clock;
                    if(duration >= UINT_MAX)
                        m_expired = (uint32_t)-1;
                    else
                        m_expired = (uint32_t)duration;
                }
                else
                {
                    m_expired = 0;
                }
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        /*
        void    xvqcert_t::set_gmtime(const uint64_t gmtime_seconds_now) //gmtime_now could be get from (xtime_utl:gmttime_ms()/1000)
        {
            if(is_allow_modify())
            {
                if(0 == m_clock) //not allow modify it after set_clock()
                {
                    if(gmtime_seconds_now > (uint64_t)1573189200) //1573189200 == 2019-11-08 05:00:00  UTC
                    {
                        m_clock = (uint64_t)(gmtime_seconds_now - 1573189200) / 10;
                        xinfo("xvqcert_t::set_gmtime,good gmtime_seconds_now=%" PRIu64 "",gmtime_seconds_now);
                    }
                    else
                    {
                        xwarn("xvqcert_t::set_gmtime,bad gmtime_seconds_now=%" PRIu64 " < 946713600",gmtime_seconds_now);
                    }
                    add_modified_count();
                }
                return;
            }
            xassert(0);
        }
     */
      void    xvqcert_t::set_gmtime(const uint64_t gmtime_seconds_now) //gmtime_now could be get from (xtime_utl:gmttime_ms()/1000)
        {
            if(is_allow_modify())
            {
                if(gmtime_seconds_now > (uint64_t)1573189200) //1573189200 == 2019-11-08 05:00:00  UTC
                {
                    m_relative_gmtime = (uint32_t)(gmtime_seconds_now - 1573189200);
                }
                else
                {
                    m_relative_gmtime = 0;
                }
                add_modified_count();
                return;
            }
            xassert(0);
        }
    
        void   xvqcert_t::set_nonce(const uint64_t nonce)
        {
            if(is_allow_modify())
            {
                m_nonce = nonce;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void    xvqcert_t::set_viewid(const uint64_t viewid)
        {
            if(is_allow_modify())
            {
                m_viewid = viewid;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void   xvqcert_t::set_viewtoken(const uint32_t viewtoken)
        {
            if(is_allow_modify())
            {
                m_view_token = viewtoken;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void    xvqcert_t::set_validator(const xvip2_t & validator_xip)
        {
            if(is_allow_modify())
            {
                m_validator  = validator_xip;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        void    xvqcert_t::set_auditor(const xvip2_t & auditor_xip)
        {
            if(is_allow_modify())
            {
                m_auditor  = auditor_xip;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        //[3bit[2bit][3bit] = [enum_xconsensus_type][enum_xconsensus_threshold][enum_xconsensus_audit_flag]
        void    xvqcert_t::set_consensus_type(enum_xconsensus_type type)
        {
            if(is_allow_modify())
            {
                m_consensus = ((m_consensus & 0x1F) | (type << 5));
                add_modified_count();
                return;
            }
            xassert(0);
        }
        void    xvqcert_t::set_consensus_threshold(enum_xconsensus_threshold type)
        {
            if(is_allow_modify())
            {
                m_consensus = ((m_consensus & 0xE7) | (type << 3));
                add_modified_count();
                return;
            }
            xassert(0);
        }
        void    xvqcert_t::set_consensus_flag(enum_xconsensus_flag flag) //lowest 3bit
        {
            if(is_allow_modify())
            {
                m_consensus |= flag;
                add_modified_count();
                return;
            }
            xassert(0);
        };
        
        //[enum_xvblock_level][enum_xvblock_class][enum_xvblock_type][enum_xvblock_phase] =  [4][3][7][2] = 16bits
        void    xvqcert_t::set_crypto_key_type(enum_xvchain_key_curve type)
        {
            if(is_allow_modify())
            {
                m_cryptos = ((m_cryptos & 0x3F) | (type << 6));
                add_modified_count();
                return;
            }
            xassert(0);
        }
        void    xvqcert_t::set_crypto_sign_type(enum_xvchain_sign_scheme type)
        {
            if(is_allow_modify())
            {
                m_cryptos = ((m_cryptos & 0xC7) | (type << 3));
                add_modified_count();
                return;
            }
            xassert(0);
        }
        void    xvqcert_t::set_crypto_hash_type(enum_xhash_type type)
        {
            if(is_allow_modify())
            {
                m_cryptos = ((m_cryptos & 0xF8) | (type     ));
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void   xvqcert_t::set_input_root_hash(const std::string & merkle_hash)
        {
            if(is_allow_modify())
            {
                m_input_root_hash = merkle_hash;
                add_modified_count();
                return;
            }
            else if(merkle_hash == m_input_root_hash)
            {
                return;
            }
            xassert(0);
        }
        
        void   xvqcert_t::set_output_root_hash(const std::string & merkle_hash)
        {
            if(is_allow_modify())
            {
                m_output_root_hash = merkle_hash;
                add_modified_count();
                return;
            }
            else if(merkle_hash == m_output_root_hash)
            {
                return;
            }
            xassert(0);
        }
        
        void    xvqcert_t::set_justify_cert_hash(const std::string & hash)
        {
            if(is_allow_modify())
            {
                m_justify_cert_hash = hash;
                add_modified_count();
                return;
            }
            else if(m_justify_cert_hash == hash)
            {
                return;
            }
            xassert(0);
        }
        
        bool    xvqcert_t::set_header_hash(const std::string& header_binary_data)
        {
            if(is_allow_modify())
            {
                const std::string hash_to_check = hash(header_binary_data);
                if( (m_header_hash.empty() == false) && (m_header_hash != hash_to_check) )
                {
                    xwarn("xvqcert_t::set_header_hash,try to overwrited existing header with different hash,existing-hash(%s) vs new_hash(%s)",m_header_hash.c_str(),hash_to_check.c_str());
                }
                if(hash_to_check != m_header_hash)
                {
                    m_header_hash = hash_to_check;
                    add_modified_count();
                }
                return true;
            }
            xassert(0);
            return false;
        }
        
        //only set them after verify certification by CA(xvcertauth_t)
        void    xvqcert_t::set_verify_signature(const std::string & proof)
        {
            m_verify_signature = proof;
            add_modified_count();
            return;
        }
        
        void    xvqcert_t::set_audit_signature(const std::string & proof)
        {
            m_audit_signature = proof;
            add_modified_count();
            return;
        }
        
        void    xvqcert_t::set_extend_data(const std::string& extention)
        {
            if(m_extend_data.empty())
            {
                m_extend_data = extention;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        void   xvqcert_t::set_extend_cert(const std::string & _cert_bin)
        {
            if(m_extend_cert.empty())
            {
                m_extend_cert = _cert_bin;
                add_modified_count();
                return;
            }
            xassert(0);
        }
        
        bool    xvqcert_t::is_allow_modify() const
        {
            if( (m_verify_signature.empty() == false) || (m_audit_signature.empty() == false) || (m_extend_cert.empty() == false) )
                return false;
            
            return true;
        }
        
        bool    xvqcert_t::is_valid()  const
        {
            if(   ( 0 == m_consensus)
               || ( 0 == m_cryptos)
               || ( 0 == m_validator.low_addr)
               || ( 0 == m_view_token)
               || ( 0 == m_nonce)
               || ( 0 == m_expired)
               || ( m_header_hash.empty())
               )
            {
                xdbg_info("xvqcert_t::is_valid,some property not set correctly,dump=%s",dump().c_str());
                return false;
            }
            else if(m_viewid != 0) //any non-genesis cert
            {
                if(0 == m_clock)
                {
                    xdbg_info("xvqcert_t::is_valid,clock can not be 0 for non-genesis cert,dump=%s",dump().c_str());
                    return false;
                }
            }
            else //geneis cert
            {
                if(0 != m_clock)
                {
                    xdbg_info("xvqcert_t::is_valid,clock must be 0 for genesis cert,dump=%s",dump().c_str());
                    return false;
                }
                if(0 != m_drand_height)
                {
                    xdbg_info("xvqcert_t::is_valid,m_drand_height must be 0 for genesis cert,dump=%s",dump().c_str());
                    return false;
                }
            }
            //255 are big enough to hold any hash result
            if(   (get_header_hash().size()       > 255)
               || (get_input_root_hash().size()   > 255)
               || (get_output_root_hash().size()  > 255)
               || (get_justify_cert_hash().size() > 255)
               )
            {
                xerror("xvqcert_t::is_valid,invalid hash of size > 255");
                return false;
            }
            
            if( (get_consensus_flags() & enum_xconsensus_flag_audit_cert) != 0)
            {
                if( ( 0 == m_auditor.low_addr) || (0 == m_auditor.high_addr) ) //if ask audit but dont have set threshold for audit
                {
                    xerror("xvqcert_t::is_valid,audit and threshold cant be empty for audit required block");
                    return false;
                }
            }
            else if( ( 0 != m_auditor.low_addr) || (0 != m_auditor.high_addr) )//if NOT ask audit but DO have set threshold for audit
            {
                xerror("xvqcert_t::is_valid,audit and threshold must be empty for non-audit block");
                return false;
            }
            if( (get_consensus_flags() & enum_xconsensus_flag_commit_cert) != 0)//not support this flag yet,so not allow use now
            {
                xerror("xvqcert_t::is_valid,not support flag of commit_cert at current version");
                return false;//change behavior once we support basic-mode of xBFT
            }
            return true;
        }
        
        bool    xvqcert_t::is_deliver()  const
        {
            if(is_valid() == false)
                return false;
 
            if( (0 == m_viewid) && (0 == m_clock) ) //genesis block
                return true;
            
            if(get_consensus_flags() & enum_xconsensus_flag_extend_cert)
            {
                if(m_extend_cert.empty())
                {
                    xdbg("xvqcert_t::is_deliver,has flag_extend_cert but miss the extend_cert");
                    return false;
                }
                if( (false == m_verify_signature.empty()) || (false == m_audit_signature.empty()) )
                {
                    xerror("xvqcert_t::is_deliver,extend cert not allow carry own'verify/auditor signature");
                    return false;
                }
            }
            else
            {
                if(m_verify_signature.empty())
                    return false;
                
                if( (get_consensus_flags() & enum_xconsensus_flag_audit_cert) != 0)//if ask audit but dont have audit proof
                {
                    if(m_audit_signature.empty())
                    {
                        xerror("xvqcert_t::is_deliver,has flag_audit_cert but miss audit_signature");
                        return false;
                    }
                }
                else
                {
                    if(m_audit_signature.empty() == false)
                    {
                        xerror("xvqcert_t::is_deliver,dont ask audit_signature,but has content of audit_signature ");
                        return false;
                    }
                }
            }
            return true;
        }
        
        bool    xvqcert_t::is_equal(const xvqcert_t & other) const
        {
            if(   (m_consensus          != other.m_consensus)
               || (m_cryptos            != other.m_cryptos)
               || (m_drand_height       != other.m_drand_height)
               || (m_clock              != other.m_clock)
               || (m_expired            != other.m_expired)
               || (m_viewid             != other.m_viewid)
               || (m_view_token         != other.m_view_token)
               || (m_nonce              != other.m_nonce)
               || (m_validator.low_addr  != other.m_validator.low_addr)
               || (m_validator.high_addr != other.m_validator.high_addr)
               || (m_auditor.low_addr    != other.m_auditor.low_addr)
               || (m_auditor.high_addr   != other.m_auditor.high_addr)
               || (m_header_hash        != other.m_header_hash)
               || (m_input_root_hash    != other.m_input_root_hash)
               || (m_output_root_hash   != other.m_output_root_hash)
               || (m_justify_cert_hash  != other.m_justify_cert_hash)
               || (m_parent_height      != other.m_parent_height)
               || (m_parent_viewid      != other.m_parent_viewid)
               )
            {
                return false;
            }
            return true;
        }
        
        bool    xvqcert_t::is_same(const xvqcert_t & other) const
        {
            if(is_equal(other) == false)
                return false;
            
            if((m_verify_signature  != other.m_verify_signature) || (m_audit_signature != other.m_audit_signature) || (m_extend_cert != other.m_extend_cert) || (m_extend_data != other.m_extend_data) )
                return false;
            
            return true;
        }
        
        bool   xvqcert_t::is_validator(const uint64_t replica_xip) const
        {
            const uint64_t replica_core_network_xip = (replica_xip << 8) >> 18; //remove flags at highest 8bit,and remove local_id
            const uint64_t leader_core_network_xip = (m_validator.low_addr << 8) >> 18;
            if(leader_core_network_xip == replica_core_network_xip)
                return true;
            
            return false;
        }
        bool   xvqcert_t::is_validator(const xvip2_t& replica_xip2) const
        {
            return is_validator(replica_xip2.low_addr);
        }
        
        bool  xvqcert_t::is_auditor(const uint64_t replica_xip) const
        {
            const uint64_t replica_core_network_xip = (replica_xip << 8) >> 18; //remove round at highest 8bit,and remove local_id
            const uint64_t leader_core_network_xip = (m_auditor.low_addr << 8) >> 18;
            if(leader_core_network_xip == replica_core_network_xip)
                return true;
            
            return false;
        }
        bool  xvqcert_t::is_auditor(const xvip2_t& replica_xip2) const
        {
            return is_auditor(replica_xip2.low_addr);
        }
        
        const uint32_t   xvqcert_t::get_validator_threshold()            const
        {
            const uint32_t group_nodes_count = get_group_nodes_count_from_xip2(get_validator());
            if(group_nodes_count == 0)
                return 0;
            
            uint32_t ask_threshold = (uint32_t)(-1); //init to an unreachable number
            if(base::enum_xconsensus_threshold_2_of_3 == get_consensus_threshold())
                ask_threshold = (uint32_t)((group_nodes_count * 2) / 3) + 1;
            else if(base::enum_xconsensus_threshold_3_of_4 == get_consensus_threshold())
                ask_threshold = (uint32_t)((group_nodes_count * 3) / 4) + 1;
            else
                ask_threshold = group_nodes_count; //as default ask full pass
            
            return ask_threshold;
        }
        
        const uint32_t   xvqcert_t::get_auditor_threshold()              const
        {
            const uint32_t group_nodes_count = get_group_nodes_count_from_xip2(get_auditor());
            if(group_nodes_count == 0)
                return 0;
            
            uint32_t ask_threshold = (uint32_t)(-1); //init to an unreachable number
            if(base::enum_xconsensus_threshold_2_of_3 == get_consensus_threshold())
                ask_threshold = (uint32_t)((group_nodes_count * 2) / 3) + 1;
            else if(base::enum_xconsensus_threshold_3_of_4 == get_consensus_threshold())
                ask_threshold = (uint32_t)((group_nodes_count * 3) / 4) + 1;
            else
                ask_threshold = group_nodes_count; //as default ask full pass
            
            return ask_threshold;
        }
        
        const std::string  xvqcert_t::get_hash_to_sign() const //signatrure' hash = hash(m_target_obj_hash + m_consensus + m_cryptos..)
        {
            base::xautostream_t<512> stream(base::xcontext_t::instance());
            
            //new compact mode
            {
                stream.write_compact_var(m_nonce);
                stream.write_compact_var(m_parent_height);
                stream.write_compact_var(m_parent_viewid);
                stream.write_compact_var(m_view_token);
                stream.write_compact_var(m_viewid);
                stream.write_compact_var(m_drand_height);
                stream.write_compact_var(m_clock);
                stream.write_compact_var(m_expired);
                stream << m_validator.low_addr;
                stream << m_validator.high_addr;
                stream << m_auditor.low_addr;
                stream << m_auditor.high_addr;
                stream << m_consensus;
                stream << m_cryptos;
                
                //255 bytes is big enough to hold any hash result
                stream.write_tiny_string(m_header_hash);
                stream.write_tiny_string(m_input_root_hash);
                stream.write_tiny_string(m_output_root_hash);
                stream.write_tiny_string(m_justify_cert_hash);
            }
            
            const std::string data_to_sign((const char*)stream.data(),stream.size());
            return hash(data_to_sign);
        }
        
        const std::string  xvqcert_t::build_block_hash()    //recalculate the hash of block, note: block_hash is equal as cert'hash
        {
            std::string  cert_bin_data;
            serialize_to_string(cert_bin_data);
            return hash(cert_bin_data);
        }
        
        //note:cert may carry some flags of block temproray,so dont serialized them to persist stream
        int32_t   xvqcert_t::serialize_to(xstream_t & stream)
        {
            const uint16_t const_flags = get_unit_flags();
            //cert need keep consist at bin level, so not serialize the flags of block,so clean 0 for block flags by keep lowest 8 bit
            const int32_t  writed_size = xdataunit_t::serialize_to_with_flags(stream,const_flags & enum_xdata_flags_mask);
            return writed_size;
        }
        
        int32_t   xvqcert_t::serialize_from(xstream_t & stream)//not allow subclass change behavior
        {
            return xdataunit_t::serialize_from(stream);
        }
        
        //serialize vheader and certificaiton,return how many bytes is writed/read
        int32_t   xvqcert_t::serialize_to_string(std::string & bin_data)   //wrap function fo serialize_to(stream)
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            const uint16_t const_flags = get_unit_flags();
            //cert need keep consist at bin level, so not serialize the flags of block,so clean 0 for block flags by keep lowest 8 bit
            const int result = xdataunit_t::serialize_to_with_flags(_stream,const_flags & enum_xdata_flags_mask);
            if(result > 0)
                bin_data.assign((const char*)_stream.data(),_stream.size());
            
            return result;
        }
        
        int32_t   xvqcert_t::serialize_from_string(const std::string & bin_data) //wrap function fo serialize_from(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            const int result = xdataunit_t::serialize_from(_stream);
            return result;
        }
        
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t   xvqcert_t::do_write(xstream_t & stream)   //not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            
            //new compact mode
            {
                stream.write_compact_var(m_nonce);
                stream.write_compact_var(m_parent_height);
                stream.write_compact_var(m_parent_viewid);
                stream.write_compact_var(m_view_token);
                stream.write_compact_var(m_viewid);
                stream.write_compact_var(m_drand_height);
                stream.write_compact_var(m_clock);
                stream.write_compact_var(m_expired);
                stream << m_validator.low_addr;
                stream << m_validator.high_addr;
                stream << m_auditor.low_addr;
                stream << m_auditor.high_addr;
                stream << m_consensus;
                stream << m_cryptos;
                
                //255 bytes is big enough to hold any hash result
                stream.write_tiny_string(m_header_hash);
                stream.write_tiny_string(m_input_root_hash);
                stream.write_tiny_string(m_output_root_hash);
                stream.write_tiny_string(m_justify_cert_hash);
                
                stream.write_compact_var(m_verify_signature);
                stream.write_compact_var(m_audit_signature);
                stream.write_compact_var(m_extend_cert);
                stream.write_compact_var(m_extend_data);
            }
            
            return (stream.size() - begin_size);
        }
        
        int32_t   xvqcert_t::do_read(xstream_t & stream)    //not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            
            //new compact mode
            {
                stream.read_compact_var(m_nonce);
                stream.read_compact_var(m_parent_height);
                stream.read_compact_var(m_parent_viewid);
                stream.read_compact_var(m_view_token);
                stream.read_compact_var(m_viewid);
                stream.read_compact_var(m_drand_height);
                stream.read_compact_var(m_clock);
                stream.read_compact_var(m_expired);
                stream >> m_validator.low_addr;
                stream >> m_validator.high_addr;
                stream >> m_auditor.low_addr;
                stream >> m_auditor.high_addr;
                stream >> m_consensus;
                stream >> m_cryptos;
                
                //255 bytes is big enough to hold any hash result
                stream.read_tiny_string(m_header_hash);
                stream.read_tiny_string(m_input_root_hash);
                stream.read_tiny_string(m_output_root_hash);
                stream.read_tiny_string(m_justify_cert_hash);
                
                stream.read_compact_var(m_verify_signature);
                stream.read_compact_var(m_audit_signature);
                stream.read_compact_var(m_extend_cert);
                stream.read_compact_var(m_extend_data);
            }
         
            reset_modified_count();//reset it
            return (begin_size - stream.size());
        }
    
        //---------------------------------xvinput_t---------------------------------//
        xvinput_t::xvinput_t(enum_xobject_type type)
            :xvexemodule_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
        
        xvinput_t::xvinput_t(const std::vector<xventity_t*> & entitys,const std::string & raw_resource_data,enum_xobject_type type)
            :xvexemodule_t(entitys,raw_resource_data,type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
    
        xvinput_t::xvinput_t(std::vector<xventity_t*> && entitys,xstrmap_t & resource_obj,enum_xobject_type type)
            :xvexemodule_t(entitys,resource_obj,type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
    
        xvinput_t::xvinput_t(const std::vector<xventity_t*> & entitys,xstrmap_t & resource_obj, enum_xobject_type type)
            :xvexemodule_t(entitys,resource_obj,type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
    
        xvinput_t::~xvinput_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, -1);
        }
    
        void*   xvinput_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vinput)
                return this;
            
            return xvexemodule_t::query_interface(_enum_xobject_type_);
        }
        
        base::xvinentity_t* xvinput_t::get_primary_entity() const
        {
            if (get_entitys().empty())
            {
                return nullptr;
            }
            return (base::xvinentity_t*)get_entitys()[0];
        }
        
        size_t xvinput_t::get_action_count() const {
            size_t total_count = 0;
            auto & entitys = get_entitys();
            for (auto & entity : entitys) {
                xvinentity_t* inentity = dynamic_cast<xvinentity_t*>(entity);  // it must be inentity
                xassert(inentity != nullptr);
                total_count += inentity->get_actions().size();
            }
            return total_count;
        }
        
        std::string xvinput_t::dump() const
        {
            char local_param_buf[128];

            xprintf(local_param_buf,sizeof(local_param_buf),"{entitys=%zu,actions=%zu}",
                    get_entitys().size(),get_action_count());
            return std::string(local_param_buf);
        }

        //---------------------------------xvoutput_t---------------------------------//
        xvoutput_t::xvoutput_t(enum_xobject_type type)
            :xvexemodule_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
    
        xvoutput_t::xvoutput_t(std::vector<xventity_t*> && entitys,enum_xobject_type type)
            :xvexemodule_t(entitys, std::string(),type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
       
        xvoutput_t::xvoutput_t(const std::vector<xventity_t*> & entitys,const std::string & raw_resource_data, enum_xobject_type type)
            :xvexemodule_t(entitys, raw_resource_data,type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
    
        xvoutput_t::xvoutput_t(const std::vector<xventity_t*> & entitys,xstrmap_t & resource_obj, enum_xobject_type type)//xvqcert_t used for genreate hash for resource
            :xvexemodule_t(entitys,resource_obj,type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
    
        xvoutput_t::~xvoutput_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, -1);
        }
        
        void*   xvoutput_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_voutput)
                return this;
            
            return xvexemodule_t::query_interface(_enum_xobject_type_);
        }
        
        const std::string xvoutput_t::get_binlog()
        {
            std::string binlog_hash = get_binlog_hash();
            if (!binlog_hash.empty())
            {
                return query_resource(binlog_hash);
            }
            return std::string();
        }
        
        base::xvoutentity_t* xvoutput_t::get_primary_entity() const
        {
            if (get_entitys().empty())
            {
                return nullptr;
            }
            return (base::xvoutentity_t*)get_entitys()[0];
        }
        const std::string xvoutput_t::get_binlog_hash()
        {
            if (get_entitys().empty())
            {
                return std::string();
            }
            base::xvoutentity_t* outentity = get_primary_entity();
            if (outentity == nullptr)
            {
                return std::string();
            }
            return outentity->get_binlog_hash();
        }
        
        const std::string xvoutput_t::get_state_hash()
        {
            if (get_entitys().empty())
            {
                return std::string();
            }
            base::xvoutentity_t* outentity = get_primary_entity();
            if (outentity == nullptr)
            {
                return std::string();
            }
            return outentity->get_state_hash();
        }
        
        std::string xvoutput_t::dump() const
        {
            char local_param_buf[128];
            
            xprintf(local_param_buf,sizeof(local_param_buf),"{entitys=%zu}",
                    get_entitys().size());
            return std::string(local_param_buf);
        }
 
        //---------------------------------xvblock_t---------------------------------//
        const std::string  xvblock_t::create_block_path(const std::string & account,const uint64_t height) //path pointed to vblock at DB/disk
        {
            std::string empty_subname;
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(account));
            return xvblock_t::get_object_path(hash_account, height,empty_subname);
        }
        const std::string  xvblock_t::create_header_path(const std::string & account,const uint64_t height)
        {
            std::string empty_subname;
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(account));
            return xvblock_t::get_object_path(hash_account, height,empty_subname);
        }
        
        xvblock_t::xvblock_t()
        : xdataobj_t((enum_xdata_type)enum_xobject_type_vblock)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvblock, 1);
            m_next_next_viewid = 0;
            m_next_next_qcert  = NULL;
            m_prev_block   = NULL;
            m_next_block   = NULL;
            m_vheader_ptr  = NULL;
            m_vqcert_ptr   = NULL;
            m_vinput_ptr   = NULL;
            m_voutput_ptr  = NULL;
            m_vbstate_ptr  = NULL;
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        
        xvblock_t::xvblock_t(enum_xdata_type type)
        : xdataobj_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvblock, 1);
            m_next_next_viewid = 0;
            m_next_next_qcert  = NULL;
            m_prev_block   = NULL;
            m_next_block   = NULL;
            m_vheader_ptr  = NULL;
            m_vqcert_ptr   = NULL;
            m_vinput_ptr   = NULL;
            m_voutput_ptr  = NULL;
            m_vbstate_ptr  = NULL;
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }

        bool  xvblock_t::prepare_block(xvqcert_t & _vcert,xvheader_t & _vheader,xvinput_t * _vinput,xvoutput_t * _voutput)
        {
            if(_vheader.get_block_class() != enum_xvblock_class_nil)
            {
                if( (NULL == _vinput) || (NULL == _voutput) )
                    return false;
            }
            else
            {
                if( (NULL != _vinput) || (NULL != _voutput) )
                    return false;
            }
            
            //now header are completely ready
            std::string vheader_bin;
            std::string vinput_bin;
            std::string voutput_bin;
            if(_vheader.get_block_class() != enum_xvblock_class_nil)
            {
                //input check
                {
                    //makeup hash for input & output
                    if(_vinput->get_resources_hash().empty() == false)
                    {
                        if(_vinput->get_resources_hash() != _vcert.hash(_vinput->get_resources_data()))
                        {
                            xassert(0);
                            return false;
                        }
                    }
                    else
                    {
                        _vinput->set_resources_hash(_vcert.hash(_vinput->get_resources_data()));
                    }
                    //generate root of merkle for input & output if have
                    if(_vcert.get_input_root_hash().empty() == false)
                    {
                        if(_vcert.get_input_root_hash() != _vinput->get_root_hash())
                        {
                            xassert(0);
                            return false;
                        }
                    }
                    else
                    {
                        _vcert.set_input_root_hash(_vinput->get_root_hash());
                    }
                    
                    //now input & output are completely ready,ready to set input&output hash into header
                    _vinput->serialize_to_string(vinput_bin);
                }
           
                //output check
                {
                    if(_voutput->get_resources_hash().empty() == false)
                    {
                        if(_voutput->get_resources_hash() != _vcert.hash(_voutput->get_resources_data()))
                        {
                            xassert(0);
                            return false;
                        }
                    }
                    else
                    {
                        _voutput->set_resources_hash(_vcert.hash(_voutput->get_resources_data()));
                    }
                    
                    if(_vcert.get_output_root_hash().empty() == false)
                    {
                        if(_vcert.get_output_root_hash() != _voutput->get_root_hash())
                        {
                            xassert(0);
                            return false;
                        }
                    }
                    else
                    {
                        _vcert.set_output_root_hash(_voutput->get_root_hash());
                    }
                    _voutput->serialize_to_string(voutput_bin);
                }
            }
            
            if(_vheader.get_block_characters() & enum_xvblock_character_certify_header_only)
            {
                if(_vheader.get_input_hash().empty() == false)
                {
                    if(_vheader.get_input_hash() != _vcert.hash(vinput_bin))
                    {
                        xassert(0);
                        return false;
                    }
                }
                else
                {
                    _vheader.set_input_hash(_vcert.hash(vinput_bin));
                }
                
                if(_vheader.get_output_hash().empty() == false)
                {
                    if(_vheader.get_output_hash() != _vcert.hash(voutput_bin))
                    {
                        xassert(0);
                        return false;
                    }
                }
                else
                {
                    _vheader.set_output_hash(_vcert.hash(voutput_bin));
                }
                
                //now header are completely ready
                _vheader.serialize_to_string(vheader_bin);
                
                //link cert and header
                if(_vcert.set_header_hash(vheader_bin) == false )
                { //bind to certification
                    xassert(0);
                    return false;
                }
            }
            else //qcert.header_hash = hash(header+input+output)
            {
                //now header are completely ready
                _vheader.serialize_to_string(vheader_bin);
                
                //link cert and header,input,output
                const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
                if(_vcert.set_header_hash(vheader_input_output) == false )
                { //bind to certification
                    xassert(0);
                    return false;
                }
            }
            
            return true;
        }
    
        xvblock_t::xvblock_t(xvheader_t & _vheader,xvqcert_t & _vcert,xvinput_t * _vinput,xvoutput_t * _voutput,enum_xdata_type type)
        : xdataobj_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvblock, 1);
            m_next_next_viewid = 0;
            m_next_next_qcert  = NULL;
            m_prev_block   = NULL;
            m_next_block   = NULL;
            m_vheader_ptr  = NULL;
            m_vqcert_ptr   = NULL;
            m_vinput_ptr   = NULL;
            m_voutput_ptr  = NULL;
            m_vbstate_ptr  = NULL;
             
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
         
            if(xvblock_t::prepare_block(_vcert,_vheader,_vinput,_voutput))
            {
                _vheader.add_ref();
                m_vheader_ptr = &_vheader;
                
                _vcert.add_ref();
                m_vqcert_ptr = &_vcert;
                
                if(NULL == _vinput) //for nil block
                {
                    m_vinput_ptr = new xvinput_t();
                }
                else
                {
                    _vinput->add_ref();
                    m_vinput_ptr = _vinput;
                }

                if(NULL == _voutput) //for nil block
                {
                    m_voutput_ptr = new xvoutput_t();
                }
                else
                {
                    _voutput->add_ref();
                    m_voutput_ptr = _voutput;
                }
                    
                if(is_input_ready(false) == false)
                    xassert(0);//force quit at debug mode
                
                if(is_output_ready(false) == false)
                    xassert(0);//force quit at debug mode
                              
                add_modified_count(); //mark changed
            }else {
                xerror("xvblock_t::xvblock_t,check_objects failed with bad objects!");
            }
        }

        xvblock_t::xvblock_t(const xvblock_t & other,enum_xdata_type type)
        : xdataobj_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvblock, 1);
            m_next_next_viewid  = 0;
            
            m_next_next_qcert  = NULL;
            m_prev_block   = NULL;
            m_next_block   = NULL;
            m_vheader_ptr  = NULL;
            m_vqcert_ptr   = NULL;
            m_vinput_ptr   = NULL;
            m_voutput_ptr  = NULL;
            m_vbstate_ptr  = NULL;
            *this = other;
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        
        xvblock_t & xvblock_t::operator = (const xvblock_t & other)
        {
            if(m_vheader_ptr != NULL)
                m_vheader_ptr->release_ref();
            if(m_vqcert_ptr != NULL)
                m_vqcert_ptr->release_ref();
            if(m_vinput_ptr != NULL)
                m_vinput_ptr->release_ref();
            if(m_voutput_ptr != NULL)
                m_voutput_ptr->release_ref();
            if(m_vbstate_ptr != NULL)
                m_vbstate_ptr->release_ref();
            
            m_cert_hash         = other.m_cert_hash;
            m_vheader_ptr       = other.m_vheader_ptr;
            m_vqcert_ptr        = other.m_vqcert_ptr;
            m_prev_block        = other.m_prev_block;
            m_next_block        = other.m_next_block;
            m_vinput_ptr        = other.m_vinput_ptr;
            m_voutput_ptr       = other.m_voutput_ptr;
            m_vbstate_ptr       = other.m_vbstate_ptr;
            m_parent_account    = other.m_parent_account;
            m_next_next_viewid  = other.m_next_next_viewid;
            
            m_next_next_qcert   = other.m_next_next_qcert;
            if(m_next_next_qcert != NULL)
                m_next_next_qcert->add_ref();
            
            if(m_vheader_ptr != NULL)
                m_vheader_ptr->add_ref();
            if(m_vqcert_ptr != NULL)
                m_vqcert_ptr->add_ref();
            if(m_vinput_ptr != NULL)
                m_vinput_ptr->add_ref();
            if(m_voutput_ptr != NULL)
                m_voutput_ptr->add_ref();
            if(m_vbstate_ptr != NULL)
                m_vbstate_ptr->add_ref();
            
            if(m_prev_block != NULL)
                m_prev_block->add_ref();
            if(m_next_block != NULL)
                m_next_block->add_ref();
            
            xdataobj_t::operator=(other);
            return *this;
        }
        
        xvblock_t::~xvblock_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvblock, -1);
            if(m_vheader_ptr != NULL){
                m_vheader_ptr->close();
                m_vheader_ptr->release_ref();
            }
            if(m_vqcert_ptr != NULL){
                m_vqcert_ptr->close();
                m_vqcert_ptr->release_ref();
            }
            if(m_vinput_ptr != NULL){
                m_vinput_ptr->close();
                m_vinput_ptr->release_ref();
            }
            if(m_voutput_ptr != NULL){
                m_voutput_ptr->close();
                m_voutput_ptr->release_ref();
            }
            if(m_vbstate_ptr != NULL){
                m_vbstate_ptr->close();
                m_vbstate_ptr->release_ref();
            }

            if(m_prev_block != NULL)
                m_prev_block->release_ref();
            if(m_next_block != NULL)
                m_next_block->release_ref();
            
            if(m_next_next_qcert != NULL)
                m_next_next_qcert->release_ref();
        };
        
        std::string xvblock_t::dump() const //just for debug purpose
        {
            
#ifdef DEBUG 
            char local_param_buf[512];
        xprintf(local_param_buf,sizeof(local_param_buf),"{xvblock:account=%s,height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,class=%d,clock=%" PRIu64 ",flags=0x%x,validator=0x%" PRIx64 " : %" PRIx64 ",auditor=0x%" PRIx64 " : %" PRIx64 ",refcount=%d,this=%" PRIx64 ",block_version:%u,block_hash=%s -> last_block=%s}",get_account().c_str(),get_height(),get_viewid(),get_viewtoken(),get_block_class(),get_clock(),get_block_flags(),get_cert()->get_validator().high_addr,get_cert()->get_validator().low_addr,get_cert()->get_auditor().high_addr,get_cert()->get_auditor().low_addr,get_refcount(),(uint64_t)this,get_block_version(),xstring_utl::to_hex(m_cert_hash).c_str(),xstring_utl::to_hex(get_last_block_hash()).c_str());
            
#else
            if(check_block_flag(enum_xvblock_flag_authenticated) && (false == m_dump_info.empty()) )
            {
                return (m_dump_info + "real-flags=" + xstring_utl::tostring((int32_t)get_block_flags()));
            }
            
            char local_param_buf[256];
        xprintf(local_param_buf,sizeof(local_param_buf),"{xvblock:account=%s,height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 ",flags=0x%x,validator=0x%" PRIx64 " : %" PRIx64 ",auditor=0x%" PRIx64 " : %" PRIx64 "}",get_account().c_str(),get_height(),get_viewid(),get_viewtoken(),get_clock(),get_block_flags(),get_cert()->get_validator().high_addr,get_cert()->get_validator().low_addr,get_cert()->get_auditor().high_addr,get_cert()->get_auditor().low_addr);

#endif
            
            return std::string(local_param_buf);
        }
        
        const std::string&  xvblock_t::dump2()  //just for debug and trace purpose
        {
            if(false == m_dump_info.empty())
                return m_dump_info;
            
            m_dump_info = dump();
            return m_dump_info;
        }
        
        std::string xvblock_t::detail_dump() const //just for debug purpose
        {
#ifdef DEBUG  // only for debug
            std::string input_bin;
            std::string output_bin;
            std::string header_bin;
            get_input()->serialize_to_string(input_bin);
            get_output()->serialize_to_string(output_bin);
            get_header()->serialize_to_string(header_bin);
            uint64_t header_64 = base::xhash64_t::digest(header_bin);
            uint64_t input_64 = base::xhash64_t::digest(input_bin);
            uint64_t output_64 = base::xhash64_t::digest(output_bin);
            uint64_t input_root_64 = base::xhash64_t::digest(get_input_root_hash());
            uint64_t output_root_64 = base::xhash64_t::digest(get_output_root_hash());
            uint64_t sign_hash_64 = base::xhash64_t::digest(get_cert()->get_hash_to_sign());
            uint64_t justify_hash_64 = base::xhash64_t::digest(get_cert()->get_justify_cert_hash());
            char local_param_buf[512];
            xprintf(local_param_buf,sizeof(local_param_buf),"{header=%" PRIx64 ",input=%" PRIx64 ",output=%" PRIx64 ",inroot=%" PRIx64 ",outroot=%" PRIx64 ",sign=%" PRIx64 ",justify=%" PRIx64 "",
                    header_64,input_64,output_64,input_root_64,output_root_64,sign_hash_64,justify_hash_64);
            return std::string(local_param_buf);
#else
            return {};
#endif
        }
    
        xauto_ptr<xvblock_t> xvblock_t::clone_block() const
        {
            xobject_t* object = xcontext_t::create_xobject((enum_xobject_type)get_obj_type());
            xassert(object != NULL);
            if(object != NULL)
            {
                xvblock_t * block_obj = (xvblock_t*)object->query_interface(enum_xobject_type_vblock);
                xassert(block_obj != NULL);
                if(block_obj != NULL)
                {
                    *block_obj = *this;
                    return block_obj; //transfer ownership to xauto_ptr
                }
            }
            return nullptr;
        }
        
        bool  xvblock_t::close(bool force_async)  //close and release this node only
        {
            if(is_close() == false)
            {
                xdataobj_t::close(); //prevent re-enter
                
                reset_next_block(NULL);
                reset_prev_block(NULL);
                
                if(m_vinput_ptr != NULL)
                    m_vinput_ptr->close();
                
                if(m_voutput_ptr != NULL)
                    m_voutput_ptr->close();
                
                if(m_vbstate_ptr != NULL)
                    m_vbstate_ptr->close();
            }
            return true;
        }
        
//        #ifdef DEBUG
//        int32_t  xvblock_t::add_ref()
//        {
//            return xdataobj_t::add_ref();
//        }
//        int32_t  xvblock_t::release_ref()
//        {
//            return xdataobj_t::release_ref();
//        }
//        #endif
 
        bool  xvblock_t::reset_prev_block(xvblock_t * _new_prev_block)//return false if hash or height not match
        {
            if(_new_prev_block == m_prev_block) //same one
                return true;
            
            if(_new_prev_block != NULL)
            {
                if(get_height() == (_new_prev_block->get_height() + 1) )
                {
                    if(_new_prev_block->get_block_hash() == get_last_block_hash())//verify hash
                    {
                        _new_prev_block->add_ref();
                        xvblock_t * old_ptr =  xatomic_t::xexchange(m_prev_block, _new_prev_block);
                        if(old_ptr != NULL)
                            old_ptr->release_ref();
                    
                        return true;
                    }
                    xinfo("xvblock_t::reset_prev_block,get_last_block_hash() not match prev hash,prev_node->dump=%s vs this=%s",_new_prev_block->dump().c_str(),dump().c_str());
                }
                else
                {
                    xwarn("xvblock_t::reset_prev_block,get_height() not match (parent height + 1),prev_node->dump=%s",_new_prev_block->dump().c_str());
                }
            }
            else
            {
                xvblock_t * old_ptr =  xatomic_t::xexchange(m_prev_block, (xvblock_t*)NULL);
                if(old_ptr != NULL)
                    old_ptr->release_ref();
                
                return true;
            }
            return false;
        }
        
        bool   xvblock_t::reset_next_block(xvblock_t * _new_next_block)//return false if hash or height not match
        {
            if(_new_next_block == m_next_block)
                return true;
            
            if(_new_next_block != NULL)
            {
                if(_new_next_block->get_height() == (get_height() + 1) )
                {
                    if(_new_next_block->get_last_block_hash() == get_block_hash())
                    {
                        _new_next_block->add_ref();
                        xvblock_t * old_ptr =  xatomic_t::xexchange(m_next_block, _new_next_block);
                        if(old_ptr != NULL)
                            old_ptr->release_ref();
                        
                        return true;
                    }
                    xerror("xvblock_t::reset_next_block,next_block'get_last_block_hash() not match current hash,next_block->dump=%s",_new_next_block->dump().c_str());
                }
                else
                {
                    xerror("xvblock_t::reset_next_block,next_block'height != (cur-height + 1),cur-height=%" PRIu64 " vs next_block->dump=%s",get_height(),_new_next_block->dump().c_str());
                }
            }
            else
            {
                xvblock_t * old_ptr =  xatomic_t::xexchange(m_next_block, (xvblock_t*)NULL);
                if(old_ptr != NULL)
                    old_ptr->release_ref();
                
                return true;
            }
            return false;
        }
    
        bool  xvblock_t::reset_block_state(xvbstate_t * _new_state_ptr)//return false if hash or height not match
        {
            if(_new_state_ptr == m_vbstate_ptr) //same one
                return true;
            
            if(_new_state_ptr != NULL)
            {
                if(    (get_height()  != _new_state_ptr->get_block_height())
                    || (get_viewid()  != _new_state_ptr->get_block_viewid())
                    || (get_account() != _new_state_ptr->get_address())
                    )
                {
                    xerror("xvblock_t::reset_block_state,this block'info(%s) not match state(%s)",dump().c_str(), _new_state_ptr->dump().c_str());
                    return false;
                }
                _new_state_ptr->add_ref();
                xvbstate_t * old_ptr =  xatomic_t::xexchange(m_vbstate_ptr, _new_state_ptr);
                if(old_ptr != NULL)
                    old_ptr->release_ref();
                
                return true;
            }
            else
            {
                xvbstate_t * old_ptr =  xatomic_t::xexchange(m_vbstate_ptr, (xvbstate_t*)NULL);
                if(old_ptr != NULL)
                    old_ptr->release_ref();
                
                return true;
            }
        }
     
        void    xvblock_t::set_next_next_cert(xvqcert_t * next_next_vqcert_ptr)
        { 
            if(next_next_vqcert_ptr != NULL)
            {
                next_next_vqcert_ptr->add_ref();
                m_next_next_viewid = next_next_vqcert_ptr->get_viewid();
            }            
            xvqcert_t * old_ptr =  xatomic_t::xexchange(m_next_next_qcert, next_next_vqcert_ptr);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }
        
        bool   xvblock_t::set_input_resources(const std::string & raw_resource_data)//check whether match hash first
        {
            if(get_input() == NULL)
                return false;
            
            const std::string hash_to_check = get_cert()->hash(raw_resource_data);
            if(hash_to_check != get_input()->get_resources_hash() )
                return false;
            
            return get_input()->set_resources_data(raw_resource_data);
        }
    
        bool   xvblock_t::set_output_resources(const std::string & raw_resource_data) //check whether match hash first
        {
            if(get_output() == NULL)
                return false;
            
            const std::string hash_to_check = get_cert()->hash(raw_resource_data);
            if(hash_to_check != get_output()->get_resources_hash() )
                return false;
            
            return get_output()->set_resources_data(raw_resource_data);
        }
    
        xvinput_t *  xvblock_t::get_input() const
        {
            return m_vinput_ptr;
        }
    
        xvoutput_t*  xvblock_t::get_output() const
        {
            return m_voutput_ptr;
        }
        
        const std::string xvblock_t::get_fullstate_hash()
        {
            if (get_block_class() == enum_xvblock_class_full)
            {
                // full-block always output root hash = fullstate hash
                return get_output_root_hash();
            }
            else if (get_block_class() == enum_xvblock_class_light)
            {
                // light-block may has full-state has in output entity, eg.light-table
                std::string _hash = get_output()->get_state_hash();
                if (_hash.empty())
                {
                    _hash = get_output_root_hash();
                }
                xassert(!_hash.empty());
                return _hash;
            }
            else
            {
                // nil-block has null fullstate hash
                return std::string();
            }
        }
        
        const std::string xvblock_t::get_full_state()
        {
            if (!m_offblock_snapshot.empty())
            {
                return m_offblock_snapshot;
            }
            std::string state_hash = get_fullstate_hash();
            if (!state_hash.empty())
            {
                const std::string full_state = get_output()->query_resource(state_hash);
                return full_state;
            }
            return std::string();
        }
        
        bool xvblock_t::is_full_state_block() {
            if (get_block_class() == base::enum_xvblock_class_nil) {
                return true;
            }
            if (!get_full_state().empty()) {
                return true;
            }
            return false;
        }
        
        bool xvblock_t::set_offblock_snapshot(const std::string & snapshot)
        {
            if (!m_offblock_snapshot.empty())
            {
                xassert(m_offblock_snapshot == snapshot);
                return true;
            }
            if (snapshot.empty())
            {
                xassert(false);
                return false;
            }
            m_offblock_snapshot = snapshot;
            return true;
        }
    
        //only open for xvblock_t object to set them after verify singature by CA(xvcertauth_t)
        void  xvblock_t::set_verify_signature(const std::string & proof)
        {
            if(check_block_flag(enum_xvblock_flag_authenticated))
            {
                xerror("xvblock_t::set_verify_signature,try overwrite a exiting and autheticated proof,block=%s",dump().c_str());
                return;
            }
            get_cert()->set_verify_signature(proof);
            add_modified_count();
        }
        void  xvblock_t::set_audit_signature(const std::string & proof)
        {
            if(check_block_flag(enum_xvblock_flag_authenticated))
            {
                xerror("xvblock_t::set_audit_signature,try overwrite a exiting and autheticated proof,block=%s",dump().c_str());
                return;
            }
            get_cert()->set_audit_signature(proof);
            add_modified_count();
        }
        void  xvblock_t::set_extend_data(const std::string& extention)
        {
            if(check_block_flag(enum_xvblock_flag_authenticated))
            {
                xerror("xvblock_t::set_extend_data,try overwrite a exiting autheticated proof,block=%s",dump().c_str());
                return;
            }
            get_cert()->set_extend_data(extention);
            add_modified_count();
        }
        void  xvblock_t::set_extend_cert(const std::string & _cert_bin)
        {
            if(check_block_flag(enum_xvblock_flag_authenticated))
            {
                xerror("xvblock_t::set_extend_cert,try overwrite a exiting autheticated proof,block=%s",dump().c_str());
                return;
            }
            get_cert()->set_extend_cert(_cert_bin);
            add_modified_count();
        }
        
        bool  xvblock_t::merge_cert(xvqcert_t & other)  //do check first before merge
        {
            if(check_block_flag(enum_xvblock_flag_authenticated))
            {
                xerror("xvblock_t::merge_cert,try overwrite a exiting and autheticated proof,block=%s",dump().c_str());
                return false;
            }
            
            if(other.is_deliver() && other.is_equal(*get_cert()))
            {
                get_cert()->set_extend_cert(other.get_extend_cert());
                get_cert()->set_extend_data(other.get_extend_data());
                
                get_cert()->set_verify_signature(other.get_verify_signature());
                get_cert()->set_audit_signature(other.get_audit_signature());
                
                add_modified_count();
                return true;
            }
            return false;
        }
        
        bool  xvblock_t::is_genesis_block() const
        {
            if ((NULL == get_header()) || (NULL == get_cert()) )
                return false;
            
            if(   (get_height() == 0)
               && (get_clock()  == 0)
               && (get_viewid() == 0)
               && (enum_xvblock_type_genesis == get_header()->get_block_type())
               )
            {
                return true;
            }
            return false;
        }
        bool  xvblock_t::set_block_flag(enum_xvblock_flag flag)  //refer enum_xvblock_flag
        {
            if(check_block_flag(flag)) //duplicated setting
                return true;
            
            if(enum_xvblock_flag_authenticated == flag) //force to regenerated cert/block hash
            {
                if(get_cert()->is_deliver())
                {
                    get_cert()->set_unit_flag(enum_xvblock_flag_authenticated);//add flag for cert
                    m_cert_hash = get_cert()->build_block_hash();
                    get_cert()->reset_modified_count();//reset it because everyting has count on
                    
                    xatomic_t::xorx(m_obj_flags,(uint16_t)enum_xvblock_flag_authenticated);//now safe to set at block
                    add_modified_count();
                    
                    dump2();//copy dump information
                    return true;
                }
                else
                {
                    xerror("xvblock_t::set_block_flag,but cert is not devlier yet,block=%s",dump().c_str());
                    return false;
                }
            }
            xatomic_t::xorx(m_obj_flags,(uint16_t)flag);//atomic change flag
            add_modified_count();
            return true;
        }
        void  xvblock_t::remove_block_flag(enum_xvblock_flag flag)
        {
            const uint16_t _rflag = ~flag;
            xatomic_t::xand(m_obj_flags,(uint16_t)_rflag);
            add_modified_count();
        }
        bool  xvblock_t::check_block_flag(enum_xvblock_flag flag) const //test whether has been setted
        {
            return check_unit_flag(flag);
        }
        int xvblock_t::reset_block_flags(const uint32_t new_flags) //clean all flags related block
        {
            int old_flags = get_block_flags();
            old_flags =  (old_flags & 0x00FF); //clean first
            old_flags |= (new_flags & 0xFF00); //assign again
            reset_all_unit_flags(old_flags);//clean high 8bits for flags of block
            if(get_cert() != nullptr) //clean flags of cert as well
                get_cert()->reset_block_flags();

            add_modified_count();
            return old_flags;
        }
    
        bool  xvblock_t::is_input_ready(bool full_check_resources) const                  //nil-block return true because it dont need input
        {
            if( (get_header() == NULL) || (get_cert() == NULL) || (get_input() == NULL) ) {
                xassert(0); //it should not happen,just put assert in case
                return false;
            }
            
            if(get_header()->get_block_class() == enum_xvblock_class_nil)
                return true;
            
            if(get_input()->get_resources_hash().empty())//if no resources
                return true;
            
            if(full_check_resources)
            {
                const std::string _resources_hash = get_cert()->hash(get_input()->get_resources_data());
                if(_resources_hash != get_input()->get_resources_hash()){
                    if(_resources_hash.empty())
                        xwarn("xvblock_t::xvblock_t,empty input_resources vs existing get_resources_hash(%s)",get_input()->get_resources_hash().c_str());
                    else
                        xerror("xvblock_t::xvblock_t,bad input_resources with wrong hash,existing-hash(%s) vs new_hash(%s)",get_input()->get_resources_hash().c_str(),_resources_hash.c_str());
                    return false;
                }
            }
            
            return true;
        }
        
        bool  xvblock_t::is_output_ready(bool full_check_resources) const                  //nil-block return true because it dont need input
        {
            if( (get_header() == NULL) || (get_cert() == NULL) || (get_output() == NULL)) {
                xassert(0); //it should not happen,just put assert in case
                return false;
            }
            
            if(get_header()->get_block_class() == enum_xvblock_class_nil)
                return true;
            
            if(get_output()->get_resources_hash().empty())//if no resources
                return true;
            
            if(full_check_resources)
            {
                const std::string _resources_hash = get_cert()->hash(get_output()->get_resources_data());
                if(_resources_hash != get_output()->get_resources_hash()){
                    if(_resources_hash.empty())
                        xwarn("xvblock_t::xvblock_t,empty _voutput_resources vs existing get_resources_hash(%s)",get_output()->get_resources_hash().c_str());
                    else
                        xerror("xvblock_t::xvblock_t,bad _voutput_resources with wrong hash,existing-hash(%s) vs new_hash(%s)",get_output()->get_resources_hash().c_str(),_resources_hash.c_str());
                    return false;
                }
            }
            
            return true;
        }
        
        bool  xvblock_t::is_valid(bool deep_test) const  //just check height/view/hash/account and last_hash/last_qc_hash
        {
            if( (get_header() == NULL) || (get_cert() == NULL) || (get_input() == NULL) || (get_output() == NULL)){
                xassert(0);
                return false;
            }
            
            if( (get_header()->is_valid() == false) || (get_cert()->is_valid()  == false))
                return false;
            
            if(get_cert()->get_viewid() < get_header()->get_height()) //at least viewid >= height
            {
                xerror("xvblock_t::is_valid,viewid must >= height for block");
                return false;
            }
            
            //genesis block must be height of 0, clock of 0,viewid of 0 and set the flag of enum_xvblock_type_genesis
            if( (get_height() == 0) || (get_clock() == 0) || (get_viewid() == 0) || (enum_xvblock_type_genesis == get_header()->get_block_type()) )
            {
                if(is_genesis_block() == false)
                {
                    xerror("xvblock_t::is_valid,ggenesis block must be height of 0, clock of 0,viewid of 0 and set the flag of enum_xvblock_type_genesis");
                    return false;
                }
            }

            return true;
        }
        
        bool    xvblock_t::is_deliver(bool deep_test) const //test everytiing
        {
            if(is_valid(deep_test) && check_block_flag(enum_xvblock_flag_authenticated) )
            {
                if(m_cert_hash.empty()) //cert_hash must has been generated already
                {
                    xerror("xvblock_t::is_deliver,hash of cert is empty,but block mark as enum_xvblock_flag_authenticated");
                    return false;
                }
 
                if(deep_test){
                    const std::string cert_hash = get_cert()->build_block_hash();
                    if(cert_hash != m_cert_hash) {
                        xerror("xvblock_t::is_valid,hash of cert not match,cert_hash=%s vs m_cert_hash(%s)",cert_hash.c_str(), m_cert_hash.c_str());
                        return false;
                    }
                }
                
                //genesis block dont need verify certification
                if(get_cert()->is_deliver())
                {
                    return true;
                }
            }
            return false;
        }
        
        bool   xvblock_t::is_equal(const xvblock_t & other)  const//compare everyting except certification
        {
            if( (is_valid() == false) || (other.is_valid() == false) ) //must be valid for both
                return false;
            
            if( (get_header()->is_equal(*other.get_header()) == false) || (get_cert()->is_equal(*other.get_cert()) == false) )
                return false;
            
            return true;
        }
        
        void*   xvblock_t::query_interface(const int32_t _enum_xobject_type_)//caller respond to cast (void*) to related  interface ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vblock)
                return this;
            
            return xdataobj_t::query_interface(_enum_xobject_type_);
        }
        
        const std::string   xvblock_t::get_block_path() const //path pointed to vblock at DB/disk
        {
            if(get_account().empty())
                return std::string();
            
            std::string empty_subname;
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(get_account()));
            return xvblock_t::get_object_path(hash_account, get_height(),empty_subname);
        }
        
        const std::string   xvblock_t::get_header_path() const //path pointed to header and cert
        {
            if(get_account().empty())
                return std::string();
            
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(get_account()));
            return xvblock_t::get_object_path(hash_account, get_height(),get_block_hash());
        }
        
        const std::string   xvblock_t::get_input_path()   const //path pointed to input at DB/disk
        {
            if(get_account().empty())
                return std::string();
            
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(get_account()));
            return xvblock_t::get_object_path(hash_account, get_height(),get_input_hash());
        }
        
        const std::string   xvblock_t::get_output_path()   const //path pointed to output at DB/disk
        {
            if(get_account().empty())
                return std::string();
            
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(get_account()));
            return xvblock_t::get_object_path(hash_account, get_height(),get_output_hash());
        }
    
        int32_t   xvblock_t::serialize_to(xstream_t & stream)
        {
            return xdataobj_t::serialize_to(stream);
        }
    
        int32_t   xvblock_t::serialize_from(xstream_t & stream)//not allow subclass change behavior
        {
            return xdataobj_t::serialize_from(stream);
        }
        
        //only just store m_vblock_header_path , m_vblock_body_path and related managed-purpose information
        int32_t  xvblock_t::do_write(xstream_t & stream)
        {
            const int32_t begin_size = stream.size();

            std::string vqcert_bin;
            get_cert()->serialize_to_string(vqcert_bin);
            
            std::string vheader_bin;
            get_header()->serialize_to_string(vheader_bin);

            stream.write_tiny_string(m_cert_hash);
            stream.write_compact_var(m_next_next_viewid);
            stream.write_compact_var(vqcert_bin);
        
            stream.write_compact_var(vheader_bin);
            if(get_block_class() != enum_xvblock_class_nil)
            {
                std::string vinput_bin;
                get_input()->serialize_to_string(vinput_bin);
                std::string voutput_bin;
                get_output()->serialize_to_string(voutput_bin);
                
                stream.write_compact_var(vinput_bin);
                stream.write_compact_var(voutput_bin);
                
                #ifdef __DEBUG_BLOCK_CONTENT__
                if(get_header()->get_block_characters() & enum_xvblock_character_certify_header_only)
                {
                    const std::string vheader_bin_hash = get_cert()->hash(vheader_bin);
                    const std::string vinput_bin_hash  = get_cert()->hash(vinput_bin);
                    const std::string voutput_bin_hash = get_cert()->hash(voutput_bin);
                    xassert(vheader_bin_hash == get_cert()->get_header_hash());
                    xassert(vinput_bin_hash  == get_input_hash());
                    xassert(voutput_bin_hash == get_output_hash());
                }
                else //qcert.header_hash = hash(header+input+output)
                {
                    const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
                    const std::string vheader_input_output_hash = get_cert()->hash(vheader_input_output);
                    xassert(vheader_input_output_hash == get_cert()->get_header_hash());
                }
                #endif //endif __DEBUG_BLOCK_CONTENT__
            }
            return (stream.size() - begin_size);
        }
        
        int32_t  xvblock_t::do_read(xstream_t & stream)
        {
            //------------------------------clean first------------------------------//
            if(m_vqcert_ptr != NULL)
            {
                xvqcert_t * old_ptr = xatomic_t::xexchange(m_vqcert_ptr,(xvqcert_t*)NULL);
                if(old_ptr != NULL){
                    old_ptr->release_ref();
                    old_ptr = NULL;
                }
            }
            if(m_vheader_ptr != NULL)
            {
                xvheader_t * old_ptr = xatomic_t::xexchange(m_vheader_ptr,(xvheader_t*)NULL);
                if(old_ptr != NULL){
                    old_ptr->release_ref();
                    old_ptr = NULL;
                }
            }
            //reset first if have
            if(m_vinput_ptr != NULL){
                m_vinput_ptr->release_ref();
                m_vinput_ptr = NULL;
            }
            if(m_voutput_ptr != NULL){
                m_voutput_ptr->release_ref();
                m_voutput_ptr = NULL;
            }
            
            //start read hash
            const int32_t begin_size = stream.size();
            stream.read_tiny_string(m_cert_hash);
            stream.read_compact_var(m_next_next_viewid);
            //------------------------------read xvqcert------------------------------//
            std::string vqcert_bin;
            stream.read_compact_var(vqcert_bin);
            xassert(vqcert_bin.empty() == false);
            if(vqcert_bin.empty() == false)
            {
                xvqcert_t*  qcert_ptr = xvblock_t::create_qcert_object(vqcert_bin);
                xassert(qcert_ptr != NULL); //should has value
                if(qcert_ptr != NULL)
                {
                    #if DEBUG
                        const std::string vcert_hash = qcert_ptr->hash(vqcert_bin);
                        if( (m_cert_hash.empty() == false) && (vcert_hash != m_cert_hash) ) //test match
                        {
                            xerror("xvblock_t::do_read, vqcert not match with stored hash, vcert_hash:%s but ask %s",vcert_hash.c_str(),m_cert_hash.c_str());
                        }   
                        else
                    #endif
                        {
                            m_vqcert_ptr = qcert_ptr;
                        }
                }
            }
            if(NULL == m_vqcert_ptr) {
                 return enum_xerror_code_bad_block;
            }
            
            //------------------------------read xvheader_t------------------------------//
            std::string vheader_bin;
            stream.read_compact_var(vheader_bin);
            xassert(vheader_bin.empty() == false);
            if(vheader_bin.empty() == false)
            {
                xvheader_t*  vheader_ptr = xvblock_t::create_header_object(vheader_bin);
                xassert(vheader_ptr != NULL); //should has value
                if(vheader_ptr != NULL)
                {
                    m_vheader_ptr = vheader_ptr;
                }
            }
            if(NULL == m_vheader_ptr) {
                return enum_xerror_code_bad_block;
            }
            
            //------------------------------read input&output------------------------------//
            std::string vinput_bin;
            std::string voutput_bin;
            if(get_block_class() != enum_xvblock_class_nil)
            {
                stream.read_compact_var(vinput_bin);
                xassert(vinput_bin.empty() == false);
                if(vinput_bin.empty())
                    return enum_xerror_code_bad_block;
                
                stream.read_compact_var(voutput_bin);
                xassert(voutput_bin.empty() == false);
                if(voutput_bin.empty())
                    return enum_xerror_code_bad_block;
            }
            
            //------------------------------data verify------------------------------//
            if(get_header()->get_block_characters() & enum_xvblock_character_certify_header_only)
            {
                const std::string vheader_bin_hash = get_cert()->hash(vheader_bin);
                const std::string vinput_bin_hash  = get_cert()->hash(vinput_bin);
                const std::string voutput_bin_hash = get_cert()->hash(voutput_bin);
                if(vheader_bin_hash != get_cert()->get_header_hash()) //check with cert
                {
                    xerror("xvblock_t::do_read, xvheader_t not match with xvqcert, vheader_hash:%s but ask %s",vheader_bin_hash.c_str(),get_header_hash().c_str());
                    return enum_xerror_code_bad_block;
                }
                if(vinput_bin_hash != get_input_hash()) //nil block may nil input hash
                {
                    xerror("xvblock_t::do_read, corrupt xvinput_t, vinput_bin_hash:%s but ask %s",vinput_bin_hash.c_str(),get_input_hash().c_str());
                    return enum_xerror_code_bad_block;
                }
                if(voutput_bin_hash != get_output_hash())//nil block may nil input hash
                {
                    xerror("xvblock_t::do_read, corrupt xvoutput_t, voutput_bin_hash:%s but ask %s",voutput_bin_hash.c_str(),get_output_hash().c_str());
                    return enum_xerror_code_bad_block;
                }
            }
            else //qcert.header_hash = hash(header+input+output)
            {
                const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
                const std::string vheader_input_output_hash = get_cert()->hash(vheader_input_output);
                if(get_cert()->get_header_hash() != vheader_input_output_hash)
                {
                    xerror("xvblock_t::do_read, xvheader_t not match with xvqcert,[vheader+vinput+ voutput=%s] but ask %s",vheader_input_output_hash.c_str(), get_header_hash().c_str());
                    return enum_xerror_code_bad_block;
                }
            }
            
            //------------------------------create input/output object------------------------------//
            if(vinput_bin.empty() == false)
            {
                xvinput_t*  vinput_ptr = xvblock_t::create_input_object(vinput_bin);
                xassert(vinput_ptr != NULL); //should has value
                if(vinput_ptr != NULL)
                {
                    m_vinput_ptr = vinput_ptr;
                }
            }
            else //for nil block
            {
                m_vinput_ptr = new xvinput_t(); //creat an empty object
            }
            
            if(voutput_bin.empty() == false)
            {
                xvoutput_t*  voutput_ptr = xvblock_t::create_output_object(voutput_bin);
                xassert(voutput_ptr != NULL); //should has value
                if(voutput_ptr != NULL)
                {
                    m_voutput_ptr = voutput_ptr;
                }
            }
            else //for nil block
            {
                m_voutput_ptr = new xvoutput_t(); //creat an empty object
            }
            
            //to prevent be attacked by bad data,throw error
            if( (NULL == m_vheader_ptr) || (NULL == m_vqcert_ptr) ||  (NULL == m_vinput_ptr) || (NULL == m_voutput_ptr) )
            {
                return enum_xerror_code_bad_block;
            }
            return (begin_size - stream.size());
        }
    
        bool xvblock_t::set_parent_block(const std::string parent_addr, uint32_t parent_entity_id)
        {
            m_parent_account = parent_addr;
            m_parent_entity_id = parent_entity_id;
            return true;
        }
        
        void xvblock_t::register_object(xcontext_t & _context)
        {
            static int32_t static_registered_block_flag = 0;
            ++static_registered_block_flag;
            if(static_registered_block_flag > 1)
                return;
            
            auto lambda_new_qcert= [](const int type)->xobject_t*{
                return new xvqcert_t();
            };
            auto lambda_new_header= [](const int type)->xobject_t*{
                return new xvheader_t();
            };
            auto lambda_new_input = [](const int type)->xobject_t*{
                return new xvinput_t();
            };
            auto lambda_new_output = [](const int type)->xobject_t*{
                return new xvoutput_t();
            };
            auto lambda_new_block = [](const int type)->xobject_t*{
                return new xvblock_t();
            };

            auto lambda_new_input_entity = [](const int type)->xobject_t*{
                return new xvinentity_t();
            };
            auto lambda_new_output_entity = [](const int type)->xobject_t*{
                return new xvoutentity_t();
            };
            auto lambda_new_bin_entity = [](const int type)->xobject_t*{
                return new xvbinentity_t();
            };
            
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvqcert_t::enum_obj_type,lambda_new_qcert);
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvheader_t::enum_obj_type,lambda_new_header);
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvinput_t::enum_obj_type,lambda_new_input);
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvoutput_t::enum_obj_type,lambda_new_output);
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvblock_t::enum_obj_type,lambda_new_block);
            
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvinentity_t::enum_obj_type,lambda_new_input_entity);
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvoutentity_t::enum_obj_type,lambda_new_output_entity);
            xcontext_t::register_xobject2(_context,(enum_xobject_type)xvbinentity_t::enum_obj_type,lambda_new_bin_entity);
            
            xvbstate_t::register_object(_context);
            xkinfo("xvblock_t::register_object,finish");
        }
        
        //generated the unique path of vblock to store data to DB
        //create path like :  /chainid/account/height/, and vblock may append it's own one like "header","body" at the end
        std::string  xvblock_t::get_object_path(const std::string & account,uint64_t height,const std::string & name)
        {
            std::string full_path;
            full_path.reserve(256);
            full_path += account;
            full_path += "/";
            full_path += xstring_utl::tostring(height);
            full_path += "/";
            full_path += name;
            return full_path;
        }
    
        xvheader_t*      xvblock_t::create_header_object(const std::string & vheader_serialized_data)
        {
            if(vheader_serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)vheader_serialized_data.data(),(uint32_t)vheader_serialized_data.size());
            
            xvheader_t* header_ptr  = new xvheader_t();
            if(header_ptr->serialize_from(_stream) <= 0)
            {
                xerror("xvblock_t::create_header_object,bad vheader_serialized_data that not follow spec");
                header_ptr->release_ref();
                return NULL;
            }
            return header_ptr;
        }
        
        xvqcert_t*       xvblock_t::create_qcert_object(const std::string  & vqcert_serialized_data)
        {
            if(vqcert_serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)vqcert_serialized_data.data(),(uint32_t)vqcert_serialized_data.size());
            xdataunit_t*  _data_obj_ptr = xdataunit_t::read_from(_stream);
            if(NULL == _data_obj_ptr)
            {
                xerror("xvblock_t::create_qcert_object,bad vqcert_serialized_data that not follow spec");
                return NULL;
            }
            xvqcert_t* qcert_ptr = (xvqcert_t*)_data_obj_ptr->query_interface(enum_xobject_type_vqccert);
            if(NULL == qcert_ptr)
            {
                xerror("xvblock_t::create_qcert_object,bad vqcert_serialized_data is not for xvqcert_t,but for type:%d",_data_obj_ptr->get_obj_type());
                
                _data_obj_ptr->release_ref();
                return NULL;
            }
            return qcert_ptr;
        }
        
        xvinput_t*       xvblock_t::create_input_object(const std::string  & serialized_data)
        {
            if(serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)serialized_data.data(),(uint32_t)serialized_data.size());
        
            xvinput_t* input_ptr = new xvinput_t();
            if(input_ptr->serialize_from(_stream) <= 0)
            {
                xerror("xvblock_t::create_input_object,bad serialized_data that not follow spec");
                input_ptr->release_ref();
                return NULL;
            }
            return input_ptr;
        }
        
        xvoutput_t*       xvblock_t::create_output_object(const std::string  & serialized_data)
        {
            if(serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)serialized_data.data(),(uint32_t)serialized_data.size());
            
            xvoutput_t* output_ptr = new xvoutput_t();
            if(output_ptr->serialize_from(_stream) <= 0)
            {
                xerror("xvblock_t::create_output_object,bad serialized_data that not follow spec");
                output_ptr->release_ref();
                return NULL;
            }
            return output_ptr;
        }
    
        xvbindex_t*       xvblock_t::create_index_object(const std::string & serialized_data)
        {
            if(serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)serialized_data.data(),(uint32_t)serialized_data.size());
            xdataunit_t*  _data_obj_ptr = xdataunit_t::read_from(_stream);
            if(NULL == _data_obj_ptr)
            {
                xerror("xvblock_t::create_index_object,bad serialized_data that not follow spec");
                return NULL;
            }
            xvbindex_t* index_ptr = (xvbindex_t*)_data_obj_ptr->query_interface(enum_xobject_type_vbindex);
            if(NULL == index_ptr)
            {
                xerror("xvblock_t::create_index_object,bad serialized_data is not for xvbindex_t,but for type:%d",_data_obj_ptr->get_obj_type());
                
                _data_obj_ptr->release_ref();
                return NULL;
            }
            return index_ptr;
        }
        
        xvbstate_t*     xvblock_t::create_state_object(const std::string & serialized_data)
        {
            if(serialized_data.empty())
                return NULL;
            
            xdataunit_t * _data_obj_ptr = xdataunit_t::read_from(serialized_data);
            if(NULL == _data_obj_ptr)
            {
                xerror("xvblock_t::create_state_object,bad serialized_data that not follow spec");
                return NULL;
            }
            xvbstate_t* state_ptr = (xvbstate_t*)_data_obj_ptr->query_interface(enum_xobject_type_vbstate);
            if(NULL == state_ptr)
            {
                xerror("xvblock_t::create_state_object,bad serialized_data is not for xvbstate_t,but for type:%d",_data_obj_ptr->get_obj_type());
                
                _data_obj_ptr->release_ref();
                return NULL;
            }
            return state_ptr;
        }
    
        //create a  xvheader_t from bin data(could be from DB or from network)
        base::xvblock_t*  xvblock_t::create_block_object(const std::string & vblock_serialized_data)
        {
            if(vblock_serialized_data.empty()) //check first
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)vblock_serialized_data.data(),(uint32_t)vblock_serialized_data.size());
            xdataunit_t*  _data_obj_ptr = xdataunit_t::read_from(_stream);
            if(NULL == _data_obj_ptr)
            {
                xerror("xvblock_t::create_block_object,bad vblock_serialized_data that not follow spec");
                return NULL;
            }
            xvblock_t* block_ptr = (xvblock_t*)_data_obj_ptr->query_interface(enum_xobject_type_vblock);
            if(NULL == block_ptr)
            {
                xerror("xvblock_t::create_block_object,bad vblock_serialized_data is not for xvblock_t,but for type:%d",_data_obj_ptr->get_obj_type());
                
                _data_obj_ptr->release_ref();
                return NULL;
            }
            if( (block_ptr->get_cert() == NULL) || (block_ptr->get_header() == NULL) )
            {
                xerror("xvblock_t::create_block_object,bad vblock_serialized_data");
                _data_obj_ptr->release_ref();
                return NULL;
            }
            if((block_ptr->get_input() == NULL) || (block_ptr->get_output() == NULL) )
            {
                xerror("xvblock_t::create_block_object,bad vblock_serialized_data");
                _data_obj_ptr->release_ref();
                return NULL;
            }
            
            block_ptr->dump2(); //genereate dump information before return, to improve performance
            return block_ptr;
        }
        
    

    };//end of namespace of base
};//end of namespace of top
