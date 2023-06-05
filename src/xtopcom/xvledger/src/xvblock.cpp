// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <limits.h>
#include <cinttypes>
#include "xbase/xutl.h"
#include "xstatistic/xbasic_size.hpp"
#include "../xvstate.h"
#include "../xvblock.h"
#include "xmetrics/xmetrics.h"
#include "xvledger/xerror.h"

#ifdef DEBUG
    #define __DEBUG_BLOCK_CONTENT__
#endif

namespace top
{
    namespace base
    {
        uint64_t clock_to_gmtime(uint64_t clock) {
            return clock * 10 + TOP_BEGIN_GMTIME;
        }
        //////////////////////////////////xvblock and related implementation /////////////////////////////
        xvheader_t::xvheader_t()  //just use when seralized from db/store
            :xobject_t(enum_xobject_type_vheader), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_block_header)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvheader, 1);
            m_types     = 0;
            m_versions  = 1 << 8;//[8:features][8:major][8:minor][8:patch]
            m_chainid   = 0;
            m_height    = 0;
            m_weight    = 1;
            m_last_full_block_height = 0;
        }
        
        xvheader_t::~xvheader_t()
        {
            statistic_del();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvheader, -1);
        }
        
        xvheader_t::xvheader_t(const xvheader_t & other)
            :xobject_t(enum_xobject_type_vheader), xstatistic::xstatistic_obj_face_t(other)
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
                    if (get_block_level() != enum_xvblock_level_unit) {  // XTODO unit level can has no last full hash and height
                        xerror("xvheader_t::is_valid,last_full_block_hash and last_full_block_height must set as valid value");
                        return false;
                    }
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

        std::string   xvheader_t::dump() const  //just for debug purpose
        {
            char local_param_buf[256];

            xprintf(local_param_buf,sizeof(local_param_buf),"{xvheader:t=%d,v=%u,c=%u,h=%" PRIu64 ",w=%" PRIu64 ",l=%" PRIu64 ",a=%s,c=%s,,i=%" PRIu64 ",o=%" PRIu64 ",l=%" PRIu64 ",f=%" PRIu64 ",%" PRIu64 "}",
            m_types,m_versions,m_chainid,m_height,m_weight,m_last_full_block_height,m_account.c_str(),m_comments.c_str(),
            base::xhash64_t::digest(m_input_hash),base::xhash64_t::digest(m_output_hash),base::xhash64_t::digest(m_last_block_hash),base::xhash64_t::digest(m_last_full_block_hash),base::xhash64_t::digest(m_extra_data));
           
            return std::string(local_param_buf);
        }

        size_t xvheader_t::get_object_size_real() const {
            size_t total_size = sizeof(*this);
            total_size += get_size(m_account) + get_size(m_comments) + get_size(m_input_hash) + get_size(m_output_hash) + get_size(m_last_block_hash) +
                          get_size(m_last_full_block_hash) + get_size(m_extra_data);
            xdbg(
                "------cache size------ xvheader_t total_size:%zu "
                "this:%d,m_account:%d,m_comments:%d,m_input_hash:%d,m_output_hash:%d,m_last_block_hash:%d,m_last_full_block_hash:%d,m_extra_data:%d",
                total_size,
                sizeof(*this),
                get_size(m_account),
                get_size(m_comments),
                get_size(m_input_hash),
                get_size(m_output_hash),
                get_size(m_last_block_hash),
                get_size(m_last_full_block_hash),
                get_size(m_extra_data));
            return total_size;
        }
        
        //---------------------------------xvqcert_t---------------------------------//
        xvqcert_t::xvqcert_t()
        : xdataunit_t((enum_xdata_type)enum_xobject_type_vqccert), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vqcert)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvqcert, 1);
            m_viewid    = 0;
            m_view_token= 0;
            m_clock     = 0;
            m_drand_height = 0;
            m_parent_height = 0;
            m_parent_viewid = 0;
            m_expired   = (uint32_t)-1;
            m_validator.low_addr    = 0;
            m_validator.high_addr   = 0;
            m_auditor.low_addr      = 0;
            m_auditor.high_addr     = 0;
            m_consensus = 0;
            m_cryptos   = 0;
            m_modified_count = 0;
            m_nonce      = (uint64_t)-1;
            m_view_token = 0; // xtime_utl::get_fast_randomu();
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        
        xvqcert_t::xvqcert_t(const xvqcert_t & other,enum_xdata_type type)
        : xdataunit_t(type), xstatistic::xstatistic_obj_face_t(other)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvqcert, 1);
            m_viewid    = 0;
            m_view_token= 0;
            m_clock     = 0;
            m_drand_height = 0;
            m_parent_height = 0;
            m_parent_viewid = 0;
            m_expired   = (uint32_t)-1;
            m_validator.low_addr    = 0;
            m_validator.high_addr   = 0;
            m_auditor.low_addr      = 0;
            m_auditor.high_addr     = 0;
            m_consensus = 0;
            m_cryptos   = 0;
            m_modified_count = 0;
            m_nonce      = (uint64_t)-1;
            m_view_token = 0; // xtime_utl::get_fast_randomu();
            
            *this = other;
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        xvqcert_t::~xvqcert_t()
        {
            statistic_del();
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
            xprintf(local_param_buf,sizeof(local_param_buf),"{xvqcert:viewid=%" PRIu64 ",viewtoken=%u,clock=%" PRIu64 ",validator=0x%" PRIx64 " : %" PRIx64 ",auditor=0x%" PRIx64 " : %" PRIx64 ",parent=%" PRIu64 ":%" PRIu64 ",nonce=%ld,expire=%d,drand=%ld,m_cryptos=%d,refcount=%d,this=%" PRIu64 ",consensus_flags=%x}",
            get_viewid(),get_viewtoken(),get_clock(),get_validator().high_addr,get_validator().low_addr,get_auditor().high_addr,get_auditor().low_addr,m_parent_height,m_parent_viewid,m_nonce,m_expired,m_drand_height,m_cryptos,get_refcount(),(uint64_t)this,get_consensus_flags());
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

        void    xvqcert_t::set_expired(const  uint64_t expired)
        {
            if(is_allow_modify())
            {
                m_expired  = expired;
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
                ////lowest 3bit  clear and reset
                m_consensus = ((m_consensus & 0xF8) | flag);
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
            // if(m_extend_data.empty())
            // {
                m_extend_data = extention;
                add_modified_count();
                return;
            // }
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

        size_t xvqcert_t::get_object_size_real() const {
            size_t total_size = sizeof(*this);
            total_size += get_size(m_header_hash) + get_size(m_input_root_hash) + get_size(m_output_root_hash) + get_size(m_justify_cert_hash) + get_size(m_verify_signature) +
                          get_size(m_audit_signature) + get_size(m_extend_data) + get_size(m_extend_cert);
            xdbg("-----cache size----- xvqcert_t total_size:%zu this:%d,xvqcert_t:%d,:%d,:%d,:%d,:%d,:%d,:%d,:%d",
                 total_size,
                 sizeof(*this),
                 get_size(m_header_hash),
                 get_size(m_input_root_hash),
                 get_size(m_output_root_hash),
                 get_size(m_justify_cert_hash),
                 get_size(m_verify_signature),
                 get_size(m_audit_signature),
                 get_size(m_extend_data),
                 get_size(m_extend_cert));
            return total_size;
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
                if (get_consensus_flags() != enum_xconsensus_flag_simple_cert) {
                    xwarn("xvqcert_t::is_valid,some property not set correctly,dump=%s",dump().c_str());
                    return false;
                }
            }
            else if(m_viewid != 0) //any non-genesis cert
            {
                if(0 == m_clock)
                {
                    xwarn("xvqcert_t::is_valid,clock can not be 0 for non-genesis cert,dump=%s",dump().c_str());
                    return false;
                }
            }
            else //geneis cert
            {
                if(0 != m_clock)
                {
                    xwarn("xvqcert_t::is_valid,clock must be 0 for genesis cert,dump=%s",dump().c_str());
                    return false;
                }
                if(0 != m_drand_height)
                {
                    xwarn("xvqcert_t::is_valid,m_drand_height must be 0 for genesis cert,dump=%s",dump().c_str());
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
            
            if( get_consensus_flags() == enum_xconsensus_flag_audit_cert || get_consensus_flags() == enum_xconsensus_flag_extend_and_audit_cert)
            {
                if( ( 0 == m_auditor.low_addr) || (0 == m_auditor.high_addr) ) //if ask audit but dont have set threshold for audit
                {
                    xerror("xvqcert_t::is_valid,audit and threshold cant be empty for audit required block");
                    return false;
                }
            }
            else if( ( 0 != m_auditor.low_addr) || (0 != m_auditor.high_addr) )//if NOT ask audit but DO have set threshold for audit
            {
                xerror("xvqcert_t::is_valid,audit and threshold must be empty for non-audit block.flag=%d,auditor=%ld,%ld",get_consensus_flags(),m_auditor.low_addr,m_auditor.high_addr);
                return false;
            }
            // if( (get_consensus_flags() & enum_xconsensus_flag_commit_cert) != 0)//not support this flag yet,so not allow use now
            // {
            //     xerror("xvqcert_t::is_valid,not support flag of commit_cert at current version");
            //     return false;//change behavior once we support basic-mode of xBFT
            // }
            return true;
        }
        
        bool    xvqcert_t::is_deliver()  const
        {
            if(is_valid() == false) {
                xwarn("xvqcert_t::is_deliver, is_valid fail.");
                return false;
            }
 
            if( (0 == m_viewid) && (0 == m_clock) ) //genesis block
                return true;
            
            int consensus_flag = get_consensus_flags();
            if (consensus_flag == enum_xconsensus_flag_extend_cert || consensus_flag == enum_xconsensus_flag_extend_and_audit_cert) {
                if(m_extend_cert.empty()
                    || m_extend_data.empty()
                    || false == m_verify_signature.empty()
                    || false == m_audit_signature.empty()) {
                    xerror("xvqcert_t::is_deliver,invalid cert for extend_cert.flag=%d,%zu,%zu,%zu,%zu",consensus_flag,m_extend_cert.size(),m_extend_data.size(),m_verify_signature.size(),m_audit_signature.size());
                    return false;
                }
            } else if (consensus_flag == enum_xconsensus_flag_extend_vote) {
                if(false == m_extend_cert.empty()
                    || m_extend_data.empty()
                    || m_verify_signature.empty()
                    || false == m_audit_signature.empty()) {
                    xerror("xvqcert_t::is_deliver,invalid cert for not extend_vote.flag=%d,%zu,%zu,%zu,%zu",consensus_flag,m_extend_cert.size(),m_extend_data.size(),m_verify_signature.size(),m_audit_signature.size());
                    return false;
                } 
            } else {
                if(false == m_extend_cert.empty()
                    || false == m_extend_data.empty()) {
                    xerror("xvqcert_t::is_deliver,invalid cert for other.flag=%d,%zu,%zu,%zu,%zu",consensus_flag,m_extend_cert.size(),m_extend_data.size(),m_verify_signature.size(),m_audit_signature.size());
                    return false;
                }    
            }

            if (consensus_flag == enum_xconsensus_flag_audit_cert) {
                if (m_audit_signature.empty()) {
                    xerror("xvqcert_t::is_deliver,invalid cert for audit cert.flag=%d,%zu,%zu,%zu,%zu",consensus_flag,m_extend_cert.size(),m_extend_data.size(),m_verify_signature.size(),m_audit_signature.size());
                    return false;
                }
            } else {
                if (false == m_audit_signature.empty()) {
                    xerror("xvqcert_t::is_deliver,invalid cert for not audit cert.flag=%d,%zu,%zu,%zu,%zu",consensus_flag,m_extend_cert.size(),m_extend_data.size(),m_verify_signature.size(),m_audit_signature.size());
                    return false;
                }    
            }

            if (consensus_flag == enum_xconsensus_flag_simple_cert) {
                if (false == m_verify_signature.empty() || false == m_audit_signature.empty()) {
                    xerror("xvqcert_t::is_deliver,invalid cert for simple_cert.flag=%d,%zu,%zu,%zu,%zu",consensus_flag,m_extend_cert.size(),m_extend_data.size(),m_verify_signature.size(),m_audit_signature.size());
                    return false;
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
            :xvexemodule_t(type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vinput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
        
        // xvinput_t::xvinput_t(const std::vector<xventity_t*> & entitys,const std::string & raw_resource_data,enum_xobject_type type)
        //     :xvexemodule_t(entitys,raw_resource_data,type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vinput)
        // {
        //     XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        // }
    
        xvinput_t::xvinput_t(std::vector<xventity_t*> && entitys,xstrmap_t* resource_obj,enum_xobject_type type)
            :xvexemodule_t(entitys,resource_obj,type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vinput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
    
        xvinput_t::xvinput_t(const std::vector<xventity_t*> & entitys,xstrmap_t* resource_obj, enum_xobject_type type)
            :xvexemodule_t(entitys,resource_obj,type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vinput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvinput, 1);
        }
    
        xvinput_t::~xvinput_t()
        {
            statistic_del();
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

        size_t xvinput_t::get_object_size_real() const {
            size_t total_size = sizeof(*this);
            int32_t ex_alloc_aize = get_ex_alloc_size();
            total_size += get_size(m_root_hash) + ex_alloc_aize;
            xdbg("------cache size------ xvinput_t total_size:%zu this:%d,m_root_hash:%d,ex_alloc_size:%d", total_size, sizeof(*this), get_size(m_root_hash), ex_alloc_aize);
            return total_size;
        }

        //---------------------------------xvoutput_t---------------------------------//
        xvoutput_t::xvoutput_t(enum_xobject_type type)
            :xvexemodule_t(type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_voutput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
    
        xvoutput_t::xvoutput_t(std::vector<xventity_t*> && entitys,enum_xobject_type type)
            :xvexemodule_t(entitys, std::string(),type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_voutput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
       
        xvoutput_t::xvoutput_t(const std::vector<xventity_t*> & entitys,const std::string & raw_resource_data, enum_xobject_type type)
            :xvexemodule_t(entitys, raw_resource_data,type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_voutput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
    
        xvoutput_t::xvoutput_t(const std::vector<xventity_t*> & entitys,xstrmap_t* resource_obj, enum_xobject_type type)//xvqcert_t used for genreate hash for resource
            :xvexemodule_t(entitys,resource_obj,type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_voutput)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvoutput, 1);
        }
    
        xvoutput_t::~xvoutput_t()
        {
            statistic_del();
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

        const std::string xvoutput_t::get_output_offdata_hash() const
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
            return outentity->get_output_offdata_hash();
        }

        const std::string xvoutput_t::get_account_indexs()
        {
            return query_resource(RESOURCE_ACCOUNT_INDEXS);
        }
        
        std::string xvoutput_t::dump() const
        {
            char local_param_buf[128];
            
            xprintf(local_param_buf,sizeof(local_param_buf),"{entitys=%zu}",
                    get_entitys().size());
            return std::string(local_param_buf);
        }

        size_t xvoutput_t::get_object_size_real() const {
            size_t total_size = sizeof(*this);
            int32_t ex_alloc_aize = get_ex_alloc_size();
            total_size += get_size(m_root_hash) + ex_alloc_aize;
            xdbg("------cache size------ xvoutput_t total_size:%zu this:%d,m_root_hash:%d,ex_alloc_size:%d",
                 total_size,
                 sizeof(*this),
                 get_size(m_root_hash),
                 ex_alloc_aize);
            return total_size;
        }

        //---------------------------------xvblock_t---------------------------------//
        const std::string  xvblock_t::create_header_path(const std::string & account,const uint64_t height)
        {
            std::string empty_subname;
            const std::string hash_account = xstring_utl::tostring(xhash64_t::digest(account));
            return xvblock_t::get_object_path(hash_account, height,empty_subname);
        }
        
        xvblock_t::xvblock_t()
        : xdataobj_t((enum_xdata_type)enum_xobject_type_vblock), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vblock)
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
            
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
        }
        
        xvblock_t::xvblock_t(enum_xdata_type type)
        : xdataobj_t(type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vblock)
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
            bool is_character_cert_header_only = _vheader.get_block_characters() & enum_xvblock_character_certify_header_only;            
            std::string vinput_bin;
            std::string voutput_bin;
            if(_vheader.get_block_class() != enum_xvblock_class_nil)
            {
                //input check
                if (!is_character_cert_header_only) // cert header only mode not need set resources hash
                {
                    //makeup hash for input & output resource for compatibility
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
                    xassert(!_vinput->get_root_hash().empty());
                    _vcert.set_input_root_hash(_vinput->get_root_hash());
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

                _vinput->serialize_to_string(is_character_cert_header_only, vinput_bin);
                _voutput->serialize_to_string(is_character_cert_header_only, voutput_bin);      

                m_vinput_data = is_character_cert_header_only ? vinput_bin : _vinput->get_resources_data();
                m_voutput_data = is_character_cert_header_only ? voutput_bin : _voutput->get_resources_data();          
            }
            
            if(is_character_cert_header_only)
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
            }

            // simple unit not need set header hash
            if (!_vheader.is_character_simple_unit()) {
                std::string vheader_bin;
                //now header are completely ready
                _vheader.serialize_to_string(vheader_bin);                
                if(is_character_cert_header_only) {
                    //link cert and header
                    if(_vcert.set_header_hash(vheader_bin) == false )
                    {
                        xassert(0);
                        return false;
                    }
                } else {                   
                    //link cert and header,input,output
                    const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
                    if(_vcert.set_header_hash(vheader_input_output) == false )
                    {
                        xassert(0);
                        return false;
                    }
                }
            }
            
            return true;
        }
    
        xvblock_t::xvblock_t(xvheader_t & _vheader,xvqcert_t & _vcert,xvinput_t * _vinput,xvoutput_t * _voutput,enum_xdata_type type)
        : xdataobj_t(type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vblock)
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
             
            set_unit_flag(enum_xdata_flag_acompress);//default do copmression
         
            if(xvblock_t::prepare_block(_vcert,_vheader,_vinput,_voutput))
            {
                _vheader.add_ref();
                m_vheader_ptr = &_vheader;
                
                _vcert.add_ref();
                m_vqcert_ptr = &_vcert;
                
                if(NULL == _vinput) //for nil block
                {
                    // m_vinput_ptr = new xvinput_t();
                }
                else
                {
                    _vinput->add_ref();
                    m_vinput_ptr = _vinput;
                }

                if(NULL == _voutput) //for nil block
                {
                    // m_voutput_ptr = new xvoutput_t();
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
        : xdataobj_t(type), xstatistic::xstatistic_obj_face_t(other)
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
            
            m_cert_hash         = other.m_cert_hash;
            m_vheader_ptr       = other.m_vheader_ptr;
            m_vqcert_ptr        = other.m_vqcert_ptr;
            m_prev_block        = other.m_prev_block;
            m_next_block        = other.m_next_block;
            m_vinput_ptr        = other.m_vinput_ptr;
            m_voutput_ptr       = other.m_voutput_ptr;
            m_parent_account    = other.m_parent_account;
            m_next_next_viewid  = other.m_next_next_viewid;
            m_vote_extend_data   = other.m_vote_extend_data;
            
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
            
            if(m_prev_block != NULL)
                m_prev_block->add_ref();
            if(m_next_block != NULL)
                m_next_block->add_ref();
            
            xdataobj_t::operator=(other);
            return *this;
        }
        
        xvblock_t::~xvblock_t()
        {
            statistic_del();
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

            if(m_prev_block != NULL)
                m_prev_block->release_ref();
            if(m_next_block != NULL)
                m_next_block->release_ref();
            
            if(m_next_next_qcert != NULL)
                m_next_next_qcert->release_ref();
        };
        
        std::string xvblock_t::dump() const //just for debug purpose
        {
            
// #ifdef DEBUG 
//             char local_param_buf[512];
//         xprintf(local_param_buf,sizeof(local_param_buf),"{xvblock:account=%s,height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,class=%d,clock=%" PRIu64 ",parent=%ld,%ld,flags=0x%x,validator=0x%" PRIx64 " : %" PRIx64 ",auditor=0x%" PRIx64 " : %" PRIx64 ",refcount=%d,this=%" PRIx64 ",ver:0x%x,block_hash=%s -> last_block=%s}",
//         get_account().c_str(),get_height(),get_viewid(),get_viewtoken(),get_block_class(),get_clock(),get_cert()->get_parent_block_height(),get_cert()->get_parent_block_viewid(), get_block_flags(),get_cert()->get_validator().high_addr,get_cert()->get_validator().low_addr,get_cert()->get_auditor().high_addr,get_cert()->get_auditor().low_addr,get_refcount(),(uint64_t)this,get_block_version(),xstring_utl::to_hex(m_cert_hash).c_str(),xstring_utl::to_hex(get_last_block_hash()).c_str());
            
// #else
            if(check_block_flag(enum_xvblock_flag_authenticated) && (false == m_dump_info.empty()) )
            {
                return (m_dump_info + "flags=0x" + xstring_utl::uint642hex((uint64_t)get_block_flags()));
            }
            
            char local_param_buf[400];
            xprintf(local_param_buf,sizeof(local_param_buf),"{xvblock:%s,height=%" PRIu64 ",viewid=%" PRIu64 ",viewtoken=%u,class=%d,type=%d,clock=%" PRIu64 ",validator=0x%" PRIx64 ":%" PRIx64 ",auditor=0x%" PRIx64 ":%" PRIx64 ",ver:0x%x,parent:%ld,%ld,hash:%s->%s}",
            get_account().c_str(),get_height(),get_viewid(),get_viewtoken(),get_block_class(),get_block_type(),get_clock(),get_cert()->get_validator().high_addr,get_cert()->get_validator().low_addr,get_cert()->get_auditor().high_addr,get_cert()->get_auditor().low_addr,
            get_block_version(),get_cert()->get_parent_block_height(),get_cert()->get_parent_block_viewid(),xstring_utl::to_hex(get_block_hash()).c_str(),xstring_utl::to_hex(get_last_block_hash()).c_str());

// #endif
            return std::string(local_param_buf);
        }
        
        const std::string&  xvblock_t::dump2()  //just for debug and trace purpose
        {
            if(false == m_dump_info.empty())
                return m_dump_info;
            
            m_dump_info = dump();
            return m_dump_info;
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


        uint64_t xvblock_t::get_second_level_gmtime() const {
            // only table-block has second_level_gmtime
            if (get_block_level() != base::enum_xvblock_level_table) {
                xassert(false);
                return 0;
            }
            uint64_t gmtime = 0;
            auto & extra_str = get_header()->get_extra_data();
            // second level gmtime is introduced after v3.0.0 
            if (!extra_str.empty()) {
                base::xtableheader_extra_t he;
                he.deserialize_from_string(extra_str);
                gmtime = he.get_second_level_gmtime();
            }

            if (0 == gmtime) {
                return get_timestamp(); // XTODO return clock level gmtime for old version
            }
            return gmtime;
        }

        bool xvblock_t::is_fullunit() const {
            xassert(get_block_level() == base::enum_xvblock_level_unit);
            if (get_block_level() == base::enum_xvblock_level_unit) {
                if (get_block_class() == base::enum_xvblock_class_full) {
                    return true;
                }
                if (get_block_class() == base::enum_xvblock_class_nil && get_block_type() == base::enum_xvblock_type_fullunit) {
                    return true;
                }
            }
            return false;
        }
        bool xvblock_t::is_lightunit() const {
            xassert(get_block_level() == base::enum_xvblock_level_unit);
            if (get_block_level() == base::enum_xvblock_level_unit) {
                if (get_block_class() == base::enum_xvblock_class_light) {
                    return true;
                }
                if (get_block_class() == base::enum_xvblock_class_nil && get_block_type() == base::enum_xvblock_type_lightunit) {
                    return true;
                }
            }
            return false;
        }
        bool xvblock_t::is_emptyunit() const {
            xassert(get_block_level() == base::enum_xvblock_level_unit);
            if (get_block_level() == base::enum_xvblock_level_unit) {
                if (get_block_class() == base::enum_xvblock_class_nil) {
                    if (get_block_type() != base::enum_xvblock_type_lightunit && get_block_type() != base::enum_xvblock_type_fullunit) {
                        return true;
                    }
                }
            }
            return false;
        }
        bool xvblock_t::is_state_changed_unit() const {
            xassert(get_block_level() == base::enum_xvblock_level_unit);
            if (get_block_class() == base::enum_xvblock_class_light) {
                return true;
            } else if (get_block_class() == base::enum_xvblock_class_full 
                && base::xvblock_fork_t::is_block_match_version(get_block_version(), base::enum_xvblock_fork_version_unit_opt)) {
                return true;
            } else {
                if (get_block_type() == base::enum_xvblock_type_lightunit || get_block_type() == base::enum_xvblock_type_fullunit) {
                    return true;
                }
            }
            return false;
        }

        std::string const& xvblock_t::get_input_data_hash() const {            
            if (get_header()->is_character_cert_header_only()) {
                return get_header()->get_input_hash();
            } else {
                return get_input()->get_resources_hash();
            }            
        }
        bool xvblock_t::should_has_input_data() const {
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                return false;
            }
            return false == get_input_data_hash().empty();
        }

        std::string const& xvblock_t::get_output_data_hash() const {
            if (get_header()->is_character_cert_header_only()) {
                return get_header()->get_output_hash();
            } else {
                return get_output()->get_resources_hash();
            }
        }
        bool xvblock_t::should_has_output_data() const {
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                return false;
            }
            return false == get_output_data_hash().empty();
        }
        std::string xvblock_t::get_output_offdata_hash() const {
            if (get_block_class() == base::enum_xvblock_class_nil) {
                return {};
            }            
            if (get_header()->is_character_cert_header_only()) {
                auto & extra_str = get_header()->get_extra_data();
                if (!extra_str.empty()) {
                    xtableheader_extra_t header_extra;// TODO(jimmy)
                    header_extra.deserialize_from_string(get_header()->get_extra_data());
                    return header_extra.get_output_offdata_hash();
                }
                return {};
            } else {
                return get_output()->get_output_offdata_hash();
            }
        }
        bool xvblock_t::should_has_output_offdata() const {
            return false == get_output_offdata_hash().empty();
        }
        bool xvblock_t::set_input_data(const std::string & input_data, bool check_hash) {
            if (get_block_class() == base::enum_xvblock_class_nil) {
                return true;
            }
            if (check_hash) {
                auto & _hash = get_input_data_hash();
                if (_hash != get_cert()->hash(input_data)) {
                    xwarn("xvblock_t::set_input_data fail-hash unmatch");
                    return false;
                }
            }
            // set immediately for old version
            if (false == get_header()->is_character_cert_header_only()) {
                if (m_vinput_ptr == nullptr) {
                    xassert(false);
                    return false;
                }
                if (false == m_vinput_ptr->set_resources_data(input_data)) {
                    xassert(false);
                    return false;
                }
            }
            if (m_vinput_data != input_data) {
                m_vinput_data = input_data;
            }
            return true;
        }

        bool xvblock_t::set_output_data(const std::string & output_data, bool check_hash) {            
            if (get_block_class() == base::enum_xvblock_class_nil) {
                return true;
            }
            if (check_hash) {
                auto & _hash = get_output_data_hash();
                if (_hash != get_cert()->hash(output_data)) {
                    xwarn("xvblock_t::set_output_data fail-hash unmatch");
                    return false;
                }
            }

            // set immediately for old version
            if (false == get_header()->is_character_cert_header_only()) {
                if (m_voutput_ptr == nullptr) {
                    xassert(false);
                    return false;
                }
                if (false == m_voutput_ptr->set_resources_data(output_data)) {
                    xassert(false);
                    return false;
                }
            }
            if (m_voutput_data != output_data) {
                m_voutput_data = output_data;
            }            
            return true;
        }

        bool   xvblock_t::set_output_offdata(const std::string & raw_data, bool check_hash) //check whether match hash first
        {
            if (get_block_class() == base::enum_xvblock_class_nil) {
                return true;
            }            
            if (check_hash) {
                auto _hash = get_output_offdata_hash();
                if (_hash != get_cert()->hash(raw_data)) {
                    xerror("xvblock_t::set_output_offdata fail-hash unmatch");
                    return false;
                }
            }
            m_output_offdata = raw_data;
            return true;
        }        

        bool   xvblock_t::set_input_output(base::xvinput_t* _input_object, base::xvoutput_t* _output_object) {
            if (get_block_class() == base::enum_xvblock_class_nil) {
                xassert(false);
                return false;
            }
            if (nullptr == _input_object || nullptr == _output_object) {
                xassert(nullptr != _input_object && nullptr != _output_object);
                return false;
            }

            std::string input_data;
            std::string output_data;
            if (!get_header()->is_character_cert_header_only()) { // old version check
                std::string vheader_bin;
                get_header()->serialize_to_string(vheader_bin);

                std::string input_object_bin;
                _input_object->serialize_to_string(false, input_object_bin);
                std::string output_object_bin;
                _output_object->serialize_to_string(false, output_object_bin);

                const std::string vheader_input_output = vheader_bin + input_object_bin + output_object_bin;
                const std::string vheader_input_output_hash = get_cert()->hash(vheader_input_output);
                if(get_cert()->get_header_hash() != vheader_input_output_hash) {
                    xerror("xvblock_t::set_input_output,header hash unmatch,[vheader+vinput+ voutput=%s] but ask %s",
                        base::xstring_utl::to_hex(vheader_input_output_hash).c_str(), base::xstring_utl::to_hex(get_header_hash()).c_str());
                    return false;
                }
                input_data = _input_object->get_resources_data();
                output_data = _output_object->get_resources_data();                
            }
            else {
                _input_object->serialize_to_string(true, input_data);
                _output_object->serialize_to_string(true, output_data);
                std::string calc_hash = get_cert()->hash(input_data);
                if (get_header()->get_input_hash() != calc_hash) {
                    xerror("xvblock_t::set_input_output,input hash unmatch,%s,%s",
                        base::xstring_utl::to_hex(calc_hash).c_str(), base::xstring_utl::to_hex(get_header()->get_input_hash()).c_str());
                    return false;
                }
                calc_hash = get_cert()->hash(output_data);
                if (get_header()->get_output_hash() != calc_hash) {
                    xerror("xvblock_t::set_input_output,output hash unmatch,%s,%s",
                        base::xstring_utl::to_hex(calc_hash).c_str(), base::xstring_utl::to_hex(get_header()->get_output_hash()).c_str());
                    return false;
                }                  
            }

            m_vinput_data = input_data;
            m_voutput_data = output_data;

            // save input and output object for storing txindexs
            if (nullptr == m_vinput_ptr) {
                _input_object->add_ref();
                xvinput_t * old_ptr = xatomic_t::xexchange(m_vinput_ptr,_input_object);
                if(old_ptr != NULL){
                    xcontext_t::instance().delay_release_object(old_ptr);
                }                    
            }
            if (nullptr == m_voutput_ptr) {
                _output_object->add_ref();
                xvoutput_t * old_ptr = xatomic_t::xexchange(m_voutput_ptr,_output_object);
                if(old_ptr != NULL){
                    xcontext_t::instance().delay_release_object(old_ptr);
                }
            }

            return true;
        }

        xvinput_t* xvblock_t::load_input(std::error_code & ec)  const
        {
            if (nullptr != m_vinput_ptr) {
                return m_vinput_ptr;
            }
            // create input output object on demand for new version
            if (m_vinput_data.empty()) {
                if (should_has_input_data()) {
                    ec = error::xerrc_t::block_input_output_data_not_exist;
                    xassert(false);
                }
                return nullptr;
            }
            xvinput_t* input_ptr = create_input_object(true, m_vinput_data);
            if (nullptr == input_ptr) {
                ec = error::xerrc_t::block_input_output_create_object_fail;
                xassert(false);
                return nullptr;
            }
            xvinput_t * old_ptr = xatomic_t::xexchange(m_vinput_ptr,input_ptr);
            if(old_ptr != NULL){
                xcontext_t::instance().delay_release_object(old_ptr);
            }
            return m_vinput_ptr;      
            // if (get_block_class() == base::enum_xvblock_class_nil) {
            //     return nullptr;
            // }
            // if (get_header()->is_character_cert_header_only()) {
            //     if (get_header()->get_input_hash().empty()) {//has no input
            //         return nullptr;
            //     }
            //     if (m_vinput_ptr == nullptr) {//create input on-demand
            //         if (m_vinput_data.empty()) {
            //             ec = error::xerrc_t::block_input_output_data_not_exist;
            //             xassert(false);
            //             return nullptr;
            //         }
                    
            //         xvinput_t* input_ptr = create_input_object(true, m_vinput_data);
            //         if (nullptr == input_ptr) {
            //             ec = error::xerrc_t::block_input_output_create_object_fail;
            //             xassert(false);
            //             return nullptr;
            //         }
            //         xvinput_t * old_ptr = xatomic_t::xexchange(m_vinput_ptr,input_ptr);
            //         if(old_ptr != NULL){
            //             old_ptr->release_ref();
            //             old_ptr = NULL;
            //         }
            //     }
            // } else {
            //     if (m_vinput_ptr == nullptr) { // should never happen
            //         ec = error::xerrc_t::block_input_output_create_object_fail;
            //         xassert(false);
            //         return nullptr;
            //     }
            //     if (!m_vinput_ptr->get_resources_hash().empty()) {
            //         if (false == m_vinput_ptr->has_resource_data()) {// set input resource on-demand
            //             if (m_vinput_data.empty()) {
            //                 ec = error::xerrc_t::block_input_output_data_not_exist;
            //                 xassert(false);
            //                 return nullptr;
            //             }
            //             if (false == m_vinput_ptr->set_resources_data(m_vinput_data)) {
            //                 ec = error::xerrc_t::block_input_output_create_object_fail;
            //                 xassert(false);
            //                 return nullptr;
            //             }
            //         }
            //     }
            // }

            // xdbg("xvblock_t::load_input succ.%s",get_header()->dump().c_str());
            // xobject_ptr_t<xvinput_t> object_ptr;
            // m_vinput_ptr->add_ref();
            // object_ptr.attach(m_vinput_ptr);
            // return object_ptr;
        }

        xvoutput_t* xvblock_t::load_output(std::error_code & ec)  const
        {
            if (nullptr != m_voutput_ptr) {
                return m_voutput_ptr;
            }
            // create input output object on demand for new version
            if (m_voutput_data.empty()) {
                if (should_has_output_data()) {
                    ec = error::xerrc_t::block_input_output_data_not_exist;
                    xassert(false);
                }
                return nullptr;
            }            
            xvoutput_t* output_ptr = create_output_object(true, m_voutput_data);
            if (nullptr == output_ptr) {
                ec = error::xerrc_t::block_input_output_create_object_fail;
                xassert(false);
                return nullptr;
            }
            xvoutput_t * old_ptr = xatomic_t::xexchange(m_voutput_ptr,output_ptr);
            if(old_ptr != NULL){
                xcontext_t::instance().delay_release_object(old_ptr);
            }
            return m_voutput_ptr;
            // if (get_block_class() == base::enum_xvblock_class_nil) {
            //     return nullptr;
            // }
            // if (get_header()->is_character_cert_header_only()) {
            //     if (get_header()->get_output_hash().empty()) {//has no output
            //         return nullptr;
            //     }
            //     if (m_voutput_ptr == nullptr) {//create output on-demand
            //         if (m_voutput_data.empty()) {
            //             ec = error::xerrc_t::block_input_output_data_not_exist;
            //             xerror("xvblock_t::load_output fail-data empty.%s",dump().c_str());
            //             return nullptr;
            //         }
                    
            //         xvoutput_t* output_ptr = create_output_object(true, m_voutput_data);
            //         if (nullptr == output_ptr) {
            //             ec = error::xerrc_t::block_input_output_create_object_fail;
            //             xassert(false);
            //             return nullptr;
            //         }
            //         xvoutput_t * old_ptr = xatomic_t::xexchange(m_voutput_ptr,output_ptr);
            //         if(old_ptr != NULL){
            //             old_ptr->release_ref();
            //             old_ptr = NULL;
            //         }
            //     }
            // } else {
            //     if (m_voutput_ptr == nullptr) { // should never happen
            //         ec = error::xerrc_t::block_input_output_create_object_fail;
            //         xassert(false);
            //         return nullptr;
            //     }
            //     if (!m_voutput_ptr->get_resources_hash().empty()) {
            //         if (false == m_voutput_ptr->has_resource_data()) { // set output resource on-demand
            //             if (m_voutput_data.empty()) {
            //                 ec = error::xerrc_t::block_input_output_data_not_exist;
            //                 xassert(false);
            //                 return nullptr;
            //             }
            //             if (false == m_voutput_ptr->set_resources_data(m_voutput_data)) {
            //                 ec = error::xerrc_t::block_input_output_create_object_fail;
            //                 xassert(false);
            //                 return nullptr;
            //             }
            //         }
            //     }
            // }

            // xdbg("xvblock_t::load_output succ.%s",get_header()->dump().c_str());
            // xobject_ptr_t<xvoutput_t> object_ptr;
            // m_voutput_ptr->add_ref();
            // object_ptr.attach(m_voutput_ptr);
            // return object_ptr;
        }

        std::string xvblock_t::query_input_resource(std::string const & key) const {
            if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
                std::error_code ec;// TODO(jimmy)
                auto input_object = load_input(ec);
                if (nullptr != input_object) {
                    return input_object->query_resource(key);
                }
                if (ec) {
                    xerror("xvblock_t::query_input_resource fail.%s,key=%s",dump().c_str(),key.c_str());
                }
            }
            return {};
        }
        std::string xvblock_t::query_output_resource(std::string const & key) const {
            if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
                std::error_code ec;// TODO(jimmy)
                auto output_object = load_output(ec);
                if (nullptr != output_object) {
                    return output_object->query_resource(key);
                }
                if (ec) {
                    xerror("xvblock_t::query_output_resource fail.%s,key=%s",dump().c_str(),key.c_str());
                }
            }
            return {};
        }
        std::string xvblock_t::query_output_entity(std::string const & key) const {
            if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
                std::error_code ec;// TODO(jimmy)
                auto output_object = load_output(ec);
                if (nullptr != output_object) {
                    return output_object->get_primary_entity()->query_value(key);
                }
                if (ec) {
                    xerror("xvblock_t::query_output_entity fail.%s,key=%s",dump().c_str(),key.c_str());
                }
            }
            return {};
        }
        const std::string xvblock_t::get_account_indexs() const {
            return query_output_resource(xvoutput_t::RESOURCE_ACCOUNT_INDEXS);
        }
        const std::string xvblock_t::get_binlog() const {
            // TODO(jimmy) move to xvunit_t
            if (get_block_level() == base::enum_xvblock_level_unit
                && get_block_class() == base::enum_xvblock_class_nil
                && (get_block_type() == base::enum_xvblock_type_lightunit || get_block_type() == base::enum_xvblock_type_fullunit) ) {// && get_block_type() == base::enum_xvblock_type_lightunit
                xunit_header_extra_t _unit_extra;
                _unit_extra.deserialize_from_string(get_header()->get_extra_data());
                xassert(!_unit_extra.get_binlog().empty());
                return _unit_extra.get_binlog(); // simple unit always has binlog
            }
            auto binlog_hash = get_binlog_hash();
            return query_output_resource(binlog_hash);
        }
        const std::string xvblock_t::get_full_state() const {
            // TODO(jimmy) move to xvunit_t
            if (get_block_level() == base::enum_xvblock_level_unit
                && get_block_class() == base::enum_xvblock_class_nil
                ) {// && get_block_type() == base::enum_xvblock_type_fullunit
                // xunit_header_extra_t _unit_extra;
                // _unit_extra.deserialize_from_string(get_header()->get_extra_data());
                // xassert(!_unit_extra.get_binlog().empty());
                // return _unit_extra.get_binlog();
                return {}; // simple unit has not state
            }
            std::string state_hash = get_fullstate_hash();
            if (!state_hash.empty())
            {
                const std::string full_state = query_output_resource(state_hash);
                return full_state;
            }
            return std::string();
        }

        const std::string xvblock_t::get_binlog_hash() const {
            return query_output_entity(xvoutentity_t::key_name_binlog_hash());
        }
        int64_t xvblock_t::get_pledge_balance_change_tgas() const {
            int64_t tgas_balance_change = 0;
            std::string value;
            if (get_block_class() != base::enum_xvblock_class_nil) {
                if (get_header()->is_character_cert_header_only()) {
                    auto & extra_str = get_header()->get_extra_data();
                    if (!extra_str.empty()) {
                        xtableheader_extra_t header_extra;
                        header_extra.deserialize_from_string(get_header()->get_extra_data());
                        value = header_extra.get_pledge_balance_change_tgas();
                    }
                } else {
                    value = query_output_entity(base::xvoutentity_t::key_name_tgas_pledge_change());
                }
            }
            if (!value.empty()) {
                tgas_balance_change = base::xstring_utl::toint64(value);
            }
            return tgas_balance_change;
        }
    
        xvinput_t* xvblock_t::get_input() const
        {
            return m_vinput_ptr;
        }
    
        xvoutput_t* xvblock_t::get_output() const
        {
            return m_voutput_ptr;
        }
        
        const std::string xvblock_t::get_fullstate_hash() const
        {
            // TODO(jimmy) move to xvunit_t
            if (get_block_level() == base::enum_xvblock_level_unit
                && get_block_class() == base::enum_xvblock_class_nil
                && (get_block_type() == base::enum_xvblock_type_lightunit || get_block_type() == base::enum_xvblock_type_fullunit)) {
                xunit_header_extra_t _unit_extra;
                _unit_extra.deserialize_from_string(get_header()->get_extra_data());
                return _unit_extra.get_state_hash();    
            }
            // XTODO new version, output root = bstate snapshot hash
            if (get_header()->is_character_cert_header_only()) {
                #ifdef DEBUG
                if (get_block_class() != enum_xvblock_class_nil) {
                    assert(!get_output_root_hash().empty());
                }
                #endif
                return get_output_root_hash();
            }

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
      


        uint64_t xvblock_t::get_block_size() {
            std::string block_object_bin;
            serialize_to_string(block_object_bin);
            uint64_t block_size = (uint64_t)block_object_bin.size();
            uint64_t block_input_size = 0;
            uint64_t block_output_size = 0;
            if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
                block_input_size = (uint64_t)get_input_data().size();
                block_output_size = (uint64_t)get_output_data().size();
            }
            xdbg("xvblock_t::get_block_size block=%s header_size=%ld,input_size=%ld,output_size=%ld",dump().c_str(),block_size, block_input_size, block_output_size);
            return block_size + block_input_size + block_output_size;
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
        const std::string xvblock_t::build_block_hash() const {
            if (!get_header()->is_character_simple_unit()) {
                return get_cert()->build_block_hash();
            } else {
                std::string  header_bin_data;
                get_header()->serialize_to_string(header_bin_data);                        
                std::string  cert_bin_data;
                get_cert()->serialize_to_string(cert_bin_data);  
                return get_cert()->hash(header_bin_data + cert_bin_data);
            }
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
                    m_cert_hash = build_block_hash();                    
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
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                return true;
            }
            if (!get_header()->is_character_cert_header_only()) {
                if (get_input() == nullptr) {
                    xwarn("xvblock_t::is_input_ready fail-has input.%s",dump().c_str()); // may happen in consensus
                    return false;
                }
            }
            // XTODO hash already checked when set. delete full_check_resources future
            if (should_has_input_data()) {
                return has_input_data();
            }
            return true;
        }
        
        bool  xvblock_t::is_output_ready(bool full_check_resources) const                  //nil-block return true because it dont need input
        {
            if (get_header()->get_block_class() == base::enum_xvblock_class_nil) {
                return true;
            }
            if (!get_header()->is_character_cert_header_only()) {
                if (get_output() == nullptr) {
                    xwarn("xvblock_t::is_output_ready fail-has output.%s",dump().c_str()); // may happen in consensus
                    return false;
                }
            }
            // XTODO hash already checked when set. delete full_check_resources future
            if (should_has_output_data()) {
                return has_output_data();
            }
            return true;
        }

        bool  xvblock_t::is_output_offdata_ready(bool full_check_resources) const {
            // XTODO hash already checked when set. delete full_check_resources future
            if (should_has_output_offdata()) {
                return has_output_offdata();
            }
            return true;
        }

        bool xvblock_t::is_body_and_offdata_ready(bool full_check_resources) const {
            if (is_input_ready(full_check_resources)
                && is_output_ready(full_check_resources)
                && is_output_offdata_ready(full_check_resources)) {
                return true;
            }
            xwarn("xvblock_t::is_body_and_offdata_ready fail.%s,%d,%d,%d", dump().c_str(),has_input_data(),has_output_data(),has_output_offdata());
            return false;
        }

        const std::string xvblock_t::build_header_hash() const {
            bool is_character_cert_header_only = get_header()->is_character_cert_header_only();
            // calc and check header hash
            std::string vheader_bin;
            get_header()->serialize_to_string(vheader_bin);
            std::string calc_header_hash;
            if (is_character_cert_header_only) {
                calc_header_hash = get_cert()->hash(vheader_bin);
            } else {
                std::string input_object_bin;                    
                std::string output_object_bin;                    
                if (get_block_class() != base::enum_xvblock_class_nil) {
                    std::error_code ec;
                    auto _input_object = load_input(ec);
                    auto _output_object = load_output(ec);
                    if (_input_object == nullptr || _output_object == nullptr) {
                        xerror("xvblock_t::is_valid,input or output object load fail %s",dump().c_str());
                        return std::string();                            
                    }
                    _input_object->serialize_to_string(false, input_object_bin);
                    _output_object->serialize_to_string(false, output_object_bin);                       
                }
                const std::string vheader_input_output = vheader_bin + input_object_bin + output_object_bin;
                calc_header_hash = get_cert()->hash(vheader_input_output);
            }
            return calc_header_hash;
        }
        
        bool  xvblock_t::is_valid(bool deep_test) const  //just check height/view/hash/account and last_hash/last_qc_hash
        {
            if( (get_header() == NULL) || (get_cert() == NULL) /* || (get_input() == NULL) || (get_output() == NULL)*/){
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

            if (deep_test && get_height() != 0) {
                // XTODO already check body hash with header, not need check hash again, only check exist
                if (false == is_body_and_offdata_ready(false)) {
                    xwarn("xvblock_t::is_valid,fail-body not ready,%s", dump().c_str());
                    return false;
                }

                std::string calc_header_hash = build_header_hash();                
                if (calc_header_hash != get_cert()->get_header_hash()) {
                    xwarn("xvblock_t::is_valid,fail-header hash unmatch,%s", dump().c_str());
                    return false;
                }
            }

            return true;
        }
        
        bool    xvblock_t::is_deliver(bool deep_test) const //test everytiing
        {
            if (false == check_block_flag(enum_xvblock_flag_authenticated)) {
                xwarn("xvblock_t::is_deliver fail-not authenticated.%s", dump().c_str());
                return false;
            }
            if (false == is_valid(deep_test)) {
                xwarn("xvblock_t::is_deliver fail-not valid.%s", dump().c_str());
                return false;
            }

                if(m_cert_hash.empty()) //cert_hash must has been generated already
                {
                    xerror("xvblock_t::is_deliver,hash of cert is empty,but block mark as enum_xvblock_flag_authenticated");
                    return false;
                }
 
                if(deep_test){
                    const std::string cert_hash = build_block_hash();
                    if(cert_hash != m_cert_hash) {
                        xerror("xvblock_t::is_valid,hash of cert not match,cert_hash=%s vs m_cert_hash(%s)",cert_hash.c_str(), m_cert_hash.c_str());
                        return false;
                    }
                }
                
                //genesis block dont need verify certification
            if(false == get_cert()->is_deliver())
                {
                xwarn("xvblock_t::is_deliver fail-not deliver.%s", dump().c_str());
            return false;
            }
            xdbg("xvblock_t::is_deliver ok.%s", dump().c_str());
            return true;
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

        void      xvblock_t::set_not_serialize_input_output(bool value) {
            m_not_serialize_input_output = value;
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
            if(get_block_class() != enum_xvblock_class_nil 
                && !m_not_serialize_input_output
                && !get_header()->is_character_cert_header_only())
            {
                std::string vinput_bin;
                get_input()->serialize_to_string(false,vinput_bin);
                std::string voutput_bin;
                get_output()->serialize_to_string(false,voutput_bin);
                
                stream.write_compact_var(vinput_bin);
                stream.write_compact_var(voutput_bin);
                
                // #ifdef __DEBUG_BLOCK_CONTENT__  // TODO(jimmy) no need check again
                // if(get_header()->get_block_characters() & enum_xvblock_character_certify_header_only)
                // {
                //     const std::string vheader_bin_hash = get_cert()->hash(vheader_bin);
                //     const std::string vinput_bin_hash  = get_cert()->hash(vinput_bin);
                //     const std::string voutput_bin_hash = get_cert()->hash(voutput_bin);
                //     xassert(vheader_bin_hash == get_cert()->get_header_hash());
                //     xassert(vinput_bin_hash  == get_input_hash());
                //     xassert(voutput_bin_hash == get_output_hash());
                // }
                // else //qcert.header_hash = hash(header+input+output)
                // {
                //     const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
                //     const std::string vheader_input_output_hash = get_cert()->hash(vheader_input_output);
                //     xassert(vheader_input_output_hash == get_cert()->get_header_hash());
                // }
                // #endif //endif __DEBUG_BLOCK_CONTENT__
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
                    m_vqcert_ptr = qcert_ptr;
                }
            }
            if (NULL == m_vqcert_ptr) {
                xassert(false);
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
                xassert(false);
                return enum_xerror_code_bad_block;
            }

            #if DEBUG
                const std::string vcert_hash = build_block_hash();
                if( (m_cert_hash.empty() == false) && (vcert_hash != m_cert_hash) ) //test match
                {
                    xerror("xvblock_t::do_read, vqcert not match with stored hash, vcert_hash:%s but ask %s",vcert_hash.c_str(),m_cert_hash.c_str());
                }   
            #endif

            //------------------------------read input&output------------------------------//
            std::string vinput_bin;
            std::string voutput_bin;
            if(get_block_class() != enum_xvblock_class_nil 
                && false == get_header()->is_character_cert_header_only())
            {
                if (stream.size() == 0) {
                    xdbg("xvblock_t::do_read non nil block but no input output data.");
                    return begin_size - stream.size();
                }
                stream.read_compact_var(vinput_bin);
                xassert(vinput_bin.empty() == false);
                if(vinput_bin.empty())
                    return enum_xerror_code_bad_block;
                
                stream.read_compact_var(voutput_bin);
                xassert(voutput_bin.empty() == false);
                if(voutput_bin.empty())
                    return enum_xerror_code_bad_block;

                //------------------------------data verify------------------------------//
                const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
                const std::string vheader_input_output_hash = get_cert()->hash(vheader_input_output);
                if(get_cert()->get_header_hash() != vheader_input_output_hash)
                {
                    xerror("xvblock_t::do_read, xvheader_t not match with xvqcert,[vheader+vinput+ voutput=%s] but ask %s",vheader_input_output_hash.c_str(), get_header_hash().c_str());
                    return enum_xerror_code_bad_block;
                }

                xvinput_t*  vinput_ptr = xvblock_t::create_input_object(false, vinput_bin);
                xassert(vinput_ptr != NULL); //should has value
                if(vinput_ptr != NULL)
                {
                    m_vinput_ptr = vinput_ptr;
                }

                xvoutput_t*  voutput_ptr = xvblock_t::create_output_object(false, voutput_bin);
                xassert(voutput_ptr != NULL); //should has value
                if(voutput_ptr != NULL)
                {
                    m_voutput_ptr = voutput_ptr;
                }
            }
            
            //------------------------------data verify------------------------------//
            // if(get_header()->is_character_cert_header_only())
            // {
            //     const std::string vheader_bin_hash = get_cert()->hash(vheader_bin);
            //     const std::string vinput_bin_hash  = get_cert()->hash(vinput_bin);
            //     const std::string voutput_bin_hash = get_cert()->hash(voutput_bin);
            //     if(vheader_bin_hash != get_cert()->get_header_hash()) //check with cert
            //     {
            //         xerror("xvblock_t::do_read, xvheader_t not match with xvqcert, vheader_hash:%s but ask %s",vheader_bin_hash.c_str(),get_header_hash().c_str());
            //         return enum_xerror_code_bad_block;
            //     }
            //     if(vinput_bin_hash != get_input_hash()) //nil block may nil input hash
            //     {
            //         xerror("xvblock_t::do_read, corrupt xvinput_t, vinput_bin_hash:%s but ask %s",vinput_bin_hash.c_str(),get_input_hash().c_str());
            //         return enum_xerror_code_bad_block;
            //     }
            //     if(voutput_bin_hash != get_output_hash())//nil block may nil input hash
            //     {
            //         xerror("xvblock_t::do_read, corrupt xvoutput_t, voutput_bin_hash:%s but ask %s",voutput_bin_hash.c_str(),get_output_hash().c_str());
            //         return enum_xerror_code_bad_block;
            //     }
            // }
            // else //qcert.header_hash = hash(header+input+output)
            // {
            //     const std::string vheader_input_output      = vheader_bin + vinput_bin + voutput_bin;
            //     const std::string vheader_input_output_hash = get_cert()->hash(vheader_input_output);
            //     if(get_cert()->get_header_hash() != vheader_input_output_hash)
            //     {
            //         xerror("xvblock_t::do_read, xvheader_t not match with xvqcert,[vheader+vinput+ voutput=%s] but ask %s",vheader_input_output_hash.c_str(), get_header_hash().c_str());
            //         return enum_xerror_code_bad_block;
            //     }
            // // }
            // //------------------------------create input/output object------------------------------//
            // if(vinput_bin.empty() == false)
            // {
            //     xvinput_t*  vinput_ptr = xvblock_t::create_input_object(vinput_bin);
            //     xassert(vinput_ptr != NULL); //should has value
            //     if(vinput_ptr != NULL)
            //     {
            //         m_vinput_ptr = vinput_ptr;
            //     }
            // }
            // else //for nil block
            // {
            //     m_vinput_ptr = new xvinput_t(); //creat an empty object
            // }
            
            // if(voutput_bin.empty() == false)
            // {
            //     xvoutput_t*  voutput_ptr = xvblock_t::create_output_object(voutput_bin);
            //     xassert(voutput_ptr != NULL); //should has value
            //     if(voutput_ptr != NULL)
            //     {
            //         m_voutput_ptr = voutput_ptr;
            //     }
            // }
            // else //for nil block
            // {
            //     m_voutput_ptr = new xvoutput_t(); //creat an empty object
            // }
            
            //to prevent be attacked by bad data,throw error
            if( (NULL == m_vheader_ptr) || (NULL == m_vqcert_ptr) ) //||  (NULL == m_vinput_ptr) || (NULL == m_voutput_ptr)
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

        void xvblock_t::set_vote_extend_data(const std::string & vote_data) {
            m_vote_extend_data = vote_data;
        }

        const std::string & xvblock_t::get_vote_extend_data() const {
            return m_vote_extend_data;
        }

        void xvblock_t::set_excontainer(std::shared_ptr<xvblock_excontainer_base> excontainer) {
            xauto_lock<xspinlock_t> locker(m_spin_lock);
            m_excontainer = excontainer;
        }
        std::shared_ptr<xvblock_excontainer_base> xvblock_t::get_excontainer() const {
            std::shared_ptr<xvblock_excontainer_base> out_ptr;
            xauto_lock<xspinlock_t> locker(m_spin_lock);
            out_ptr = m_excontainer;
            return out_ptr;
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
        
        xvinput_t*       xvblock_t::create_input_object(bool include_resource, const std::string  & serialized_data)
        {
            if(serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)serialized_data.data(),(uint32_t)serialized_data.size());
        
            xvinput_t* input_ptr = new xvinput_t();
            if(input_ptr->serialize_from(include_resource, _stream) <= 0)
            {
                xerror("xvblock_t::create_input_object,bad serialized_data that not follow spec");
                input_ptr->release_ref();
                return NULL;
            }
            return input_ptr;
        }
        
        xvoutput_t*       xvblock_t::create_output_object(bool include_resource, const std::string  & serialized_data)
        {
            if(serialized_data.empty())
                return NULL;
            
            xstream_t _stream(xcontext_t::instance(),(uint8_t*)serialized_data.data(),(uint32_t)serialized_data.size());
            
            xvoutput_t* output_ptr = new xvoutput_t();
            if(output_ptr->serialize_from(include_resource, _stream) <= 0)
            {
                xerror("xvblock_t::create_output_object,bad serialized_data that not follow spec");
                output_ptr->release_ref();
                return NULL;
            }
            return output_ptr;
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
        base::xvblock_t*  xvblock_t::create_block_object(const std::string & vblock_serialized_data, bool check_input_output)
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

            block_ptr->dump2(); //genereate dump information before return, to improve performance
            return block_ptr;
        }

        size_t xvblock_t::get_object_size_real() const {
            size_t total_size = sizeof(*this);

            xdbg(
                "------cache size------ xvblock_t addr:%s total_size:%zu "
                "this:%d,m_cert_hash:%d,m_dump_info:%d,m_parent_account:%d,m_vote_extend_data:%d,m_output_offdata:%d,m_proposal:%d,m_excontainer:%d",
                get_account().c_str(),
                total_size,
                sizeof(*this),
                get_size(m_cert_hash),
                get_size(m_dump_info),
                get_size(m_parent_account),
                get_size(m_vote_extend_data),
                get_size(m_output_offdata),
                get_size(m_proposal),
                (m_excontainer != nullptr) ? sizeof(m_excontainer) : 0);

            total_size +=
                get_size(m_cert_hash) + get_size(m_dump_info) + get_size(m_parent_account) + get_size(m_vote_extend_data) + get_size(m_output_offdata) + get_size(m_proposal);

            // todo add m_excontainer alloc size.
            // avoid double counting for m_vheader_ptr, m_vinput_ptr, m_voutput_ptr and m_vbstate_ptr, _vqcert_ptr, m_prev_block, m_next_block and m_next_next_qcert
            return total_size;
        }

    };//end of namespace of base
};//end of namespace of top
