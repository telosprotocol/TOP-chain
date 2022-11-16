// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconspdu.h"
#include "xbase/xcontext.h"

namespace top
{
    namespace xconsensus
    {
        xvblockmeta::xvblockmeta()
        {
            _viewid = 0;
            _height = 0;
        }
        
        xvblockmeta::xvblockmeta(const xvblockmeta & obj)
        {
            _viewid = 0;
            _height = 0;
            *this = obj;
        }
        
        xvblockmeta & xvblockmeta::operator = (const xvblockmeta & obj)
        {
            _viewid     = obj._viewid;
            _height     = obj._height;
            _blockhash  = obj._blockhash;
            return *this;
        }
        
        xvblockmeta::~xvblockmeta()
        {
        };
        
        xcsmsg_t::xcsmsg_t()
            :base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine)
        {
        }
        
        xcsmsg_t::~xcsmsg_t()
        {
        }
        
        int32_t  xcsmsg_t::serialize_to_string(std::string & bin_data) //serialize header and object,return how many bytes is writed
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            int32_t ret = serialize_to(_stream);
            if (ret <= 0) {
                xerror("xcsmsg_t::serialize_to_string fail. ret=%d", ret);
                return ret;
            }
            
            bin_data.clear();
            bin_data.assign((const char*)_stream.data(), _stream.size());
            return (int)bin_data.size();
        }
        
        int32_t  xcsmsg_t::serialize_from_string(const std::string & bin_data)//serialize header and object,return how many bytes is readed
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(int32_t)bin_data.size());
            int32_t ret = serialize_from(_stream);
            if (ret <= 0) {
                xerror("xcsmsg_t::serialize_from_string fail. ret=%d,bin_data_size=%d", ret, bin_data.size());
            }
            return ret;
        }
        
        xproposal_msg_t::xproposal_msg_t()
        {
            m_expired_ms = 15000; //as default 15 seconds
        }
        
        //using slow path
        xproposal_msg_t::xproposal_msg_t(base::xvblock_t & proposal,base::xvqcert_t* last_block_cert)
        {
            m_expired_ms = 15000; //as default 15 seconds
            
            // only take proposal. proposal -> input ->output
            m_input_proposal = proposal.get_proposal();
//            m_input_resource  = proposal.get_input()->get_resources_data();
//            m_output_resource = proposal.get_output()->get_resources_data();
            
            //note: not carry raw data of output at proposal
            proposal.serialize_to_string(m_block_object);
            
            if(last_block_cert != NULL)
                last_block_cert->serialize_to_string(m_last_block_cert);
        }
 
        xproposal_msg_t::~xproposal_msg_t()
        {
        }
    
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t     xproposal_msg_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream << m_expired_ms;
            stream.write_short_string(m_last_block_cert);
            stream << m_block_object;
            stream << m_input_proposal;
            stream << m_input_resource;
            stream << m_output_resource;
 
            return (stream.size() - begin_size);
        }
        int32_t     xproposal_msg_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();

            stream >> m_expired_ms;
            stream.read_short_string(m_last_block_cert);
            stream >> m_block_object;
            stream >> m_input_proposal;
            stream >> m_input_resource;
            stream >> m_output_resource;
 
            return (begin_size - stream.size());
        }
        
        xproposal_msg_v2_t::xproposal_msg_v2_t()
        {
            m_expired_ms = 15000; //as default 15 seconds
        }
        
        //using slow path
        xproposal_msg_v2_t::xproposal_msg_v2_t(base::xvblock_t & proposal)
        {
            m_expired_ms = 15000; //as default 15 seconds
            
            // only take proposal. proposal -> input ->output
            m_input_proposal = proposal.get_proposal();

            proposal.set_not_serialize_input_output(true);
            proposal.serialize_to_string(m_block_object);
            proposal.set_not_serialize_input_output(false);
        }
 
        xproposal_msg_v2_t::~xproposal_msg_v2_t()
        {
        }
    
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t     xproposal_msg_v2_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream << m_version;
            stream << m_expired_ms;
            stream << m_block_object;
            stream << m_input_proposal;
 
            return (stream.size() - begin_size);
        }
        int32_t     xproposal_msg_v2_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();

            stream >> m_version;
            if (m_version != 0) {
                xassert(false);
                return -1;
            }
            stream >> m_expired_ms;
            stream >> m_block_object;
            stream >> m_input_proposal;
 
            return (begin_size - stream.size());
        }
 
        xvote_msg_t::xvote_msg_t()
        {
        }
 
        xvote_msg_t::xvote_msg_t(base::xvqcert_t & agree_by_cert_obj, const std::string & vote_extend_data)
        {
            if(agree_by_cert_obj.is_valid())
            {
                if( (false == agree_by_cert_obj.get_verify_signature().empty()) || (false == agree_by_cert_obj.get_audit_signature().empty()) )
                {
                    std::string _bin_data;
                    agree_by_cert_obj.serialize_to_string(_bin_data);
                    m_justify_source = _bin_data;
                    m_vote_extend_data = vote_extend_data;
                }
                else
                {
                    xerror("xvote_msg_t,pass un-deliver certification");
                }
            }
            else
            {
                xerror("xvote_msg_t,pass invalid certification");
            }
        }
        
        xvote_msg_t::xvote_msg_t(base::xvblock_t & report_by_block_object, const std::string & vote_extend_data)
        {
            if(report_by_block_object.is_valid(false))
            {
                if( (false == report_by_block_object.get_cert()->get_verify_signature().empty()) || (false == report_by_block_object.get_cert()->get_audit_signature().empty()) )
                {
                    std::string _bin_data;
                    report_by_block_object.get_cert()->serialize_to_string(_bin_data);
                    m_justify_source = _bin_data;
                    m_vote_extend_data = vote_extend_data;
                }
                else
                {
                    xerror("xvote_msg_t,pass un-deliver certification");
                }
            }
            else
            {
                xerror("xvote_msg_t,pass invalid block");
            }
        }
        
        xvote_msg_t::~xvote_msg_t()
        {
        }

        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t  xvote_msg_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream.write_short_string(m_justify_source);
            if (!m_vote_extend_data.empty()) {
                stream.write_short_string(m_vote_extend_data);
            }
            
            return (stream.size() - begin_size);
        }
        int32_t  xvote_msg_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();

            stream.read_short_string(m_justify_source);
            if (stream.size() > 0){
                stream.read_short_string(m_vote_extend_data);
            }
            
            return (begin_size - stream.size());
        }

        xvote_report_t::xvote_report_t()
        {
            m_error_code = 0;
            
            m_latest_cert_height = 0;
            m_latest_cert_viewid = 0;
            m_latest_lock_height = 0;
            m_latest_commit_height = 0;

        }
        
        xvote_report_t::xvote_report_t(const int32_t error_code,const std::string & error_detail)
        {
            m_error_code    = error_code;
            m_error_detail  = error_detail;
            
            m_latest_cert_height = 0;
            m_latest_cert_viewid = 0;
            m_latest_lock_height = 0;
            m_latest_commit_height = 0;
        }
        
        xvote_report_t::~xvote_report_t()
        {
        }
        
        void   xvote_report_t::set_latest_cert_block(base::xvblock_t * latest_cert_block,bool report_cert_data)
        {
            if(latest_cert_block != NULL)
            {
                m_latest_cert_height        = latest_cert_block->get_height();
                m_latest_cert_viewid        = latest_cert_block->get_viewid();
                m_latest_cert_hash          = latest_cert_block->get_block_hash();

                m_latest_cert_data.clear();
                if(report_cert_data)
                    latest_cert_block->get_cert()->serialize_to_string(m_latest_cert_data);
            }
        }
    
        void   xvote_report_t::set_latest_cert_block(const uint64_t height,const uint64_t viewid,const std::string & block_hash)
        {
            m_latest_cert_height        = height;
            m_latest_cert_viewid        = viewid;
            m_latest_cert_hash          = block_hash;
        }
        
        void   xvote_report_t::set_latest_lock_block(base::xvblock_t * latest_lock_block)
        {
            if(latest_lock_block != NULL)
            {
                m_latest_lock_height        = latest_lock_block->get_height();
                m_latest_lock_hash          = latest_lock_block->get_block_hash();
            }
        }
        
        void   xvote_report_t::set_latest_commit_block(base::xvblock_t * latest_commit_block)
        {
            if(latest_commit_block != NULL)
            {
                m_latest_commit_height = latest_commit_block->get_height();
                m_latest_commit_hash   = latest_commit_block->get_block_hash();
            }
        }

        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t  xvote_report_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream << m_error_code;
            stream.write_tiny_string(m_error_detail); //error detail can not over 256 bytes
            
            stream.write_compact_var(m_latest_cert_height);
            stream.write_compact_var(m_latest_cert_viewid);
            stream.write_tiny_string(m_latest_cert_hash);
            stream.write_compact_var(m_latest_cert_data);
            
            stream.write_compact_var(m_latest_lock_height);
            stream.write_tiny_string(m_latest_lock_hash);
            
            stream.write_compact_var(m_latest_commit_height);
            stream.write_tiny_string(m_latest_commit_hash);
            
            return (stream.size() - begin_size);
        }
        int32_t  xvote_report_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            stream >> m_error_code;
            stream.read_tiny_string(m_error_detail);
            
            stream.read_compact_var(m_latest_cert_height);
            stream.read_compact_var(m_latest_cert_viewid);
            stream.read_tiny_string(m_latest_cert_hash);
            stream.read_compact_var(m_latest_cert_data);
            
            stream.read_compact_var(m_latest_lock_height);
            stream.read_tiny_string(m_latest_lock_hash);
            
            stream.read_compact_var(m_latest_commit_height);
            stream.read_tiny_string(m_latest_commit_hash);
            
            return (begin_size - stream.size());
        }
        
        xcommit_msg_t::xcommit_msg_t()
        {
            m_proof_cert_height = 0;
            m_commit_error_code = enum_xconsensus_error_fail;
        }
        xcommit_msg_t::xcommit_msg_t(const int32_t error_code)
        {
            m_proof_cert_height = 0;
            m_commit_error_code = error_code;
        }
        xcommit_msg_t::~xcommit_msg_t()
        {
        }
 
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t  xcommit_msg_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream << m_commit_error_code;
            stream << m_proof_cert_height;
            
            stream.write_tiny_string(m_commit_error_reason);
            stream.write_short_string(m_proof_certificate);
            stream << m_commit_output;
            
            return (stream.size() - begin_size);
        }
        
        int32_t  xcommit_msg_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream >> m_commit_error_code;
            stream >> m_proof_cert_height;
            
            stream.read_tiny_string(m_commit_error_reason);
            stream.read_short_string(m_proof_certificate);
            stream >> m_commit_output;
            
            return (begin_size - stream.size());
        }
 
        xsync_request_t::xsync_request_t()
        {
            m_sync_targets = 0;
            m_reserved     = 0;
        }
       
        xsync_request_t::xsync_request_t(const uint32_t targets,const uint32_t cookie,const uint64_t target_block_height,const std::string & target_block_hash)
        {
            m_sync_targets        = targets;
            m_sync_cookie         = cookie;
            m_target_block_height = target_block_height;
            m_target_block_hash   = target_block_hash;
            m_reserved           = 0;
        }
        
        xsync_request_t::~xsync_request_t()
        {
        }

        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t   xsync_request_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            stream << m_sync_targets;
            stream << m_reserved;
            stream << m_sync_cookie;
            stream << m_target_block_height;
            
            stream.write_tiny_string(m_target_block_hash);
            return (stream.size() - begin_size);
        }
        int32_t   xsync_request_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream >> m_sync_targets;
            stream >> m_reserved;
            stream >> m_sync_cookie;
            stream >> m_target_block_height;
            
            stream.read_tiny_string(m_target_block_hash);
            return (begin_size - stream.size());
        }

        xsync_respond_t::xsync_respond_t()
        {
            m_sync_targets = 0;
        }
        
        xsync_respond_t::xsync_respond_t(const uint32_t targets,const uint32_t sync_cookie)
        {
            m_sync_targets      = targets;
            m_sync_cookie       = sync_cookie;
            m_reserved          = 0;
        }
        
        xsync_respond_t::~xsync_respond_t()
        {
        }
        
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t     xsync_respond_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            stream << m_sync_targets;
            stream << m_reserved;
            stream << m_sync_cookie;
            
            stream.write_short_string(m_block_object);
            stream << m_input_resource;
            stream << m_output_resource;
            if (m_sync_targets & enum_xsync_target_block_output_offdata) {
                stream << m_output_offdata;
            }
            
            return (stream.size() - begin_size);
        }
        
        int32_t     xsync_respond_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream >> m_sync_targets;
            stream >> m_reserved;
            stream >> m_sync_cookie;
            
            stream.read_short_string(m_block_object);
            stream >> m_input_resource;
            stream >> m_output_resource;
            if ( (m_sync_targets & enum_xsync_target_block_output_offdata) && (stream.size() > 0)) {
                stream >> m_output_offdata;
            }
            
            return (begin_size - stream.size());
        }

        xsync_respond_v2_t::xsync_respond_v2_t()
        {
            m_version = 0;
            m_sync_targets = 0;
        }
        
        xsync_respond_v2_t::xsync_respond_v2_t(const uint32_t targets,const uint32_t sync_cookie)
        {
            m_version = 0;
            m_sync_targets      = targets;
            m_sync_cookie       = sync_cookie;
        }
        
        xsync_respond_v2_t::~xsync_respond_v2_t()
        {
        }
        
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t     xsync_respond_v2_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            stream << m_version;
            stream << m_sync_targets;
            stream << m_sync_cookie;
            
            stream << m_block_object;
            stream << m_input_resource;
            stream << m_output_resource;
            if (m_sync_targets & enum_xsync_target_block_output_offdata) {
                stream << m_output_offdata;
            }
            
            return (stream.size() - begin_size);
        }
        
        int32_t     xsync_respond_v2_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();
            
            stream >> m_version;
            if (m_version != 0) {
                return -1;
            }
            stream >> m_sync_targets;
            stream >> m_sync_cookie;
            
            stream >> m_block_object;
            stream >> m_input_resource;
            stream >> m_output_resource;
            if ( (m_sync_targets & enum_xsync_target_block_output_offdata) && (stream.size() > 0)) {
                stream >> m_output_offdata;
            }
            
            return (begin_size - stream.size());
        }
    };//end of namespace of xconsensus
    
};//end of namespace of top
