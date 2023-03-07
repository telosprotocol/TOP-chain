// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include "xconsobj.h"
#include "xbase/xdata.h"


namespace top
{
    //put common pdu for consensus
    namespace xconsensus
    {
        enum enum_consensus_msg_type //under pdu  enum_xpdu_type_consensus
        {
            enum_consensus_msg_type_void        = 0, //reserved for old version
            
            enum_consensus_msg_type_view        = 1, //view-change/new-view related
            enum_consensus_msg_type_proposal    = 2, //allow nil
            enum_consensus_msg_type_vote        = 3, //vote :yes
            enum_consensus_msg_type_commit      = 4, //decided or committed a final block
            enum_consensus_msg_type_sync_reqt   = 5, //request  sync command
            enum_consensus_msg_type_sync_resp   = 6, //response sync command
            enum_consensus_msg_type_vote_report = 7, //diagnostic or debug purpose
            enum_consensus_msg_type_proposal_v2 = 8, //allow nil
            enum_consensus_msg_type_sync_resp_v2= 9, //response sync command v2
            enum_consensus_msg_type_preproposal = 10,
            
            enum_xclockview_msg_type_clock_reqt = 11, //request pull clock certificaiton from peer
            enum_xclockview_msg_type_clock_resp = 12, //respond to send clock certification to peer

            enum_consensus_msg_type_timeout = 13,
            //////////////define new msg type as below////////////////////
        };
        
        //meta data for summary of a xvblock_t
        class xvblockmeta
        {
        public:
            xvblockmeta();
            xvblockmeta(const xvblockmeta & obj);
            xvblockmeta & operator = (const xvblockmeta & obj);
            ~xvblockmeta();
        public:
            bool operator()(const xvblockmeta * front, const xvblockmeta * back)
            {
                return (front->_viewid < back->_viewid);
            }
            
            bool operator()(const xvblockmeta & front, const xvblockmeta & back)
            {
                return (front._viewid < back._viewid);
            }
        public:
            uint64_t        _viewid;
            uint64_t        _height;
            std::string     _blockhash;
        };
        
        class xcsmsg_t : public base::xdataunit_t
        {
        protected:
            xcsmsg_t();
            virtual ~xcsmsg_t();
        private:
            xcsmsg_t(const xcsmsg_t&);
            xcsmsg_t & operator = (const xcsmsg_t&);
            
        public://not safe for multiple threads,serialize_to/from write and read addtion head of dataobj
            int32_t     serialize_to_string(std::string & bin_data); //serialize header and object,return how many bytes is writed
            int32_t     serialize_from_string(const std::string & bin_data);//serialize header and object,return how many bytes is readed
        };
                
        class xproposal_msg_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_proposal;}
        public:
            xproposal_msg_t();
            xproposal_msg_t(base::xvblock_t & proposal,base::xvqcert_t* last_block_cert);
     
            virtual ~xproposal_msg_t();
        private:
            xproposal_msg_t(const xproposal_msg_t&);
            xproposal_msg_t & operator = (const xproposal_msg_t&);
        public:
            const std::string & get_block_object()    const {return m_block_object;}
            const std::string & get_input_proposal()  const {return m_input_proposal;}
            const std::string & get_input_resource()  const {return m_input_resource;}
            const std::string & get_ouput_resource()  const {return m_output_resource;}

            const std::string & get_last_block_cert() const {return m_last_block_cert;}
 
            const uint16_t      get_expired_ms() const {return m_expired_ms;}
            void                set_expired_ms(const uint16_t _expired_ms) {m_expired_ms = _expired_ms;}
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;
            virtual int32_t     do_read(base::xstream_t & stream)  override;
        private:
            uint16_t            m_expired_ms;                //duration(ms) to expire for this proposal
            std::string         m_block_object;              //header and certificate of this proposal block
            std::string         m_input_proposal;            //input'proposal for proposal block
            std::string         m_input_resource;            //input'resource for  proposal block
            std::string         m_output_resource;           //output'resource for proposal  block
            std::string         m_last_block_cert;           //the cert for last block
        };

        class xproposal_msg_v2_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_proposal_v2;}
        public:
            xproposal_msg_v2_t();
            xproposal_msg_v2_t(base::xvblock_t & proposal);
     
            virtual ~xproposal_msg_v2_t();
        private:
            xproposal_msg_v2_t(const xproposal_msg_v2_t&);
            xproposal_msg_v2_t & operator = (const xproposal_msg_v2_t&);
        public:
            const std::string & get_block_object()    const {return m_block_object;}
            const std::string & get_input_proposal()  const {return m_input_proposal;}
 
            const uint16_t      get_expired_ms() const {return m_expired_ms;}
            void                set_expired_ms(const uint16_t _expired_ms) {m_expired_ms = _expired_ms;}
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;
            virtual int32_t     do_read(base::xstream_t & stream)  override;
        private:
            uint8_t             m_version{0};
            uint16_t            m_expired_ms;                //duration(ms) to expire for this proposal
            std::string         m_block_object;              //header and certificate of this proposal block
            std::string         m_input_proposal;            //input'proposal for proposal block
        };
        
        class xvote_msg_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_vote;}
        public:
            xvote_msg_t();
            xvote_msg_t(base::xvqcert_t & agree_by_cert_obj, const std::string & vote_extend_data);     //agree by cert
            xvote_msg_t(base::xvblock_t & agree_by_block_object, const std::string & vote_extend_data); //agree by block

            virtual ~xvote_msg_t();
        private:
            xvote_msg_t(const xvote_msg_t&);
            xvote_msg_t & operator = (const xvote_msg_t&);
        public:
            const std::string & get_justify_source()const {return m_justify_source;}
            const std::string & get_vote_extend_data()const {return m_vote_extend_data;}

        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;
            virtual int32_t     do_read(base::xstream_t & stream)  override;
        private:
            std::string         m_justify_source;       //quorum_certificaiton or whole block object ,depends on m_vote_type
            std::string         m_vote_extend_data;
        };
        
        //xvote_report_t is not an offical vote, it actually just inform leader why not agree it'proposal
        //and xvote_report_t using diffirent msg type that is not blocked by pacemaker layer
        class xvote_report_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_vote_report;}
        public:
            xvote_report_t();
            xvote_report_t(const int32_t error_code,const std::string & error_detail);
            virtual ~xvote_report_t();
        private:
            xvote_report_t(const xvote_report_t&);
            xvote_report_t & operator = (const xvote_report_t&);
        public:
            inline const int            get_error_code() const  {return m_error_code;}
            inline const std::string&   get_error_detail()const {return m_error_detail;}
            
            inline const uint64_t       get_latest_cert_height() const {return m_latest_cert_height;}
            inline const uint64_t       get_latest_cert_viewid() const {return m_latest_cert_viewid;}
            inline const std::string&   get_latest_cert_hash()   const {return m_latest_cert_hash;}
            inline const std::string&   get_latest_cert_data()   const {return m_latest_cert_data;}
            
            inline const uint64_t       get_latest_lock_height() const {return m_latest_lock_height;}
            inline const std::string&   get_latest_lock_hash()   const {return m_latest_lock_hash;}
            
            inline const uint64_t       get_latest_commit_height() const {return m_latest_commit_height;}
            inline const std::string&   get_latest_commit_hash()   const {return m_latest_commit_hash;}
            
            void            set_latest_cert_block(const uint64_t height,const uint64_t viewid,const std::string & block_hash);
            void            set_latest_cert_block(const uint64_t height,base::xvqcert_t * latest_cert);
            
            void            set_latest_cert_block(base::xvblock_t * latest_cert_block,bool report_cert_data);
            void            set_latest_lock_block(base::xvblock_t * latest_lock_block);
            void            set_latest_commit_block(base::xvblock_t * latest_commit_block);
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;
            virtual int32_t     do_read(base::xstream_t & stream)  override;
            
        private:
            int32_t         m_error_code;
            std::string     m_error_detail;
            
            uint64_t        m_latest_cert_height;   //latest cert block'height of node
            uint64_t        m_latest_cert_viewid;   //latest cert block'view#  of node
            std::string     m_latest_cert_hash;     //latest cert header hash  of node
            std::string     m_latest_cert_data;     //raw latest cert 'data
            
            uint64_t        m_latest_lock_height;   //latest lock block'height of node
            std::string     m_latest_lock_hash;     //latest lock heder hash of node, may use it to sync block
            
            uint64_t        m_latest_commit_height; //latest commit block'height of node
            std::string     m_latest_commit_hash;   //latest commit heder hash of node, may use it to sync block
        };
        
        //a full cycle of one round include: Leader:proposal -> Replica:vote -> Leader:commit
        class xcommit_msg_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_commit;}
        public:
            xcommit_msg_t();
            xcommit_msg_t(const int32_t error_code);
            virtual ~xcommit_msg_t();
            
        private:
            xcommit_msg_t(const xcommit_msg_t&);
            xcommit_msg_t & operator = (const xcommit_msg_t&);
            
        public:
            const int32_t       get_commit_error_code()   const {return m_commit_error_code;}
            const std::string&  get_commit_error_reason() const {return m_commit_error_reason;}
            const uint64_t      get_proof_cert_height()   const {return m_proof_cert_height;}
            const std::string&  get_proof_certificate()   const {return m_proof_certificate;}
            const std::string&  get_commit_output()       const {return m_commit_output;}
            
            void                set_commit_error_reason(const std::string & reason) {m_commit_error_reason = reason;}
            void                set_proof_certificate(const std::string & qcert_bin,uint64_t qcert_height)
            {
                m_proof_cert_height   = qcert_height;
                m_proof_certificate   = qcert_bin;
            }
            void                set_commit_output(const std::string & commit_output){m_commit_output = commit_output;}
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream)  override;
            virtual int32_t     do_read(base::xstream_t & stream)   override;
        private:
            int32_t             m_commit_error_code;       //refer error code
            std::string         m_commit_error_reason;     //detail reason of error
            uint64_t            m_proof_cert_height;       //block height that hold m_proof_certificate
            std::string         m_proof_certificate;       //bin data of xvqcert of commit qc
            std::string         m_commit_output;           //raw data of output for target block,paired with m_proof_certificate
        };
        
        
        enum enum_xsync_target
        {
            enum_xsync_target_block_object  = 0x01, //include header and certification
            enum_xsync_target_block_input   = 0x02,
            enum_xsync_target_block_output  = 0x04,
            enum_xsync_target_block_output_offdata  = 0x08,
        };
        //each xsync_request_t just only working with one block,only only respond when carry valid proof that qualified to request
        class xsync_request_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_sync_reqt;}
        public:
            xsync_request_t();
            xsync_request_t(const uint32_t targets,const uint32_t cookie,const uint64_t target_block_height,const std::string & target_block_hash);
            virtual ~xsync_request_t();
        private:
            xsync_request_t(const xsync_request_t&);
            xsync_request_t & operator = (const xsync_request_t&);
        public:
            const uint32_t        get_sync_targets()     const {return m_sync_targets;}//refer enum_xsync_target
            const uint32_t        get_sync_cookie()      const {return m_sync_cookie;}
            const std::string&    get_block_hash()       const {return m_target_block_hash;}
            const uint64_t        get_block_height()     const {return m_target_block_height;}
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream)  override;
            virtual int32_t     do_read(base::xstream_t & stream)   override;
            
        private://note: viewid and viewtoken as proof has been include xcspdut_t
            uint16_t            m_sync_targets;           //request targets
            uint16_t            m_reserved;               //reserved for future
            uint32_t            m_sync_cookie;            //token for sync
            uint64_t            m_target_block_height;    //height of target block
            std::string         m_target_block_hash;      //carried proof of sync,note:cert_hash is equal as block_hash
        };
        
        //each xsync_respond_t just only working with one block
        class xsync_respond_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_sync_resp;}
        public:
            xsync_respond_t();
            xsync_respond_t(const uint32_t targets,const uint32_t sync_cookie);
            virtual ~xsync_respond_t();
        private:
            xsync_respond_t(const xsync_respond_t&);
            xsync_respond_t & operator = (const xsync_respond_t&);
            
        public:
            const uint32_t        get_sync_targets()    const {return m_sync_targets;}//refer enum_xsync_requst_target
            const uint32_t        get_sync_cookie()     const {return m_sync_cookie;}
            const std::string&    get_block_object()    const {return m_block_object;}
            void                  set_block_object(const std::string & object_in){m_block_object = object_in;}
            
            const std::string&    get_input_resource()     const {return m_input_resource;}
            const std::string&    get_output_resource()    const {return m_output_resource;}
            const std::string&    get_output_offdata()    const {return m_output_offdata;}
            void                  set_input_resource(const std::string & input){ m_input_resource = input;}
            void                  set_output_resource(const std::string & output){ m_output_resource = output;}
            void                  set_output_offdata(const std::string & subblocks){ m_output_offdata = subblocks;}


        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;
            virtual int32_t     do_read(base::xstream_t & stream)  override;
        private:
            uint16_t            m_sync_targets;    //responds targets
            uint16_t            m_reserved;        //reserved for future
            uint32_t            m_sync_cookie;     //token for sync
            
            std::string         m_block_object;    //must have as proof,= vblock 'serialized data include header and certification
            std::string         m_input_resource;  //block 'input data,it might be nil  according m_sync_targets
            std::string         m_output_resource; //block 'outut data,it might be nil  according m_sync_targets
            std::string         m_output_offdata;
        };

        class xsync_respond_v2_t : public xcsmsg_t
        {
        public:
            static enum_consensus_msg_type  get_msg_type() {return enum_consensus_msg_type_sync_resp_v2;}
        public:
            xsync_respond_v2_t();
            xsync_respond_v2_t(const uint32_t targets,const uint32_t sync_cookie);
            virtual ~xsync_respond_v2_t();
        private:
            xsync_respond_v2_t(const xsync_respond_v2_t&);
            xsync_respond_v2_t & operator = (const xsync_respond_v2_t&);
            
        public:
            const uint32_t        get_sync_targets()    const {return m_sync_targets;}//refer enum_xsync_requst_target
            const uint32_t        get_sync_cookie()     const {return m_sync_cookie;}
            const std::string&    get_block_object()    const {return m_block_object;}
            void                  set_block_object(const std::string & object_in){m_block_object = object_in;}
            
            const std::string&    get_input_resource()     const {return m_input_resource;}
            const std::string&    get_output_resource()    const {return m_output_resource;}
            const std::string&    get_output_offdata()    const {return m_output_offdata;}
            void                  set_input_resource(const std::string & input){ m_input_resource = input;}
            void                  set_output_resource(const std::string & output){ m_output_resource = output;}
            void                  set_output_offdata(const std::string & subblocks){ m_output_offdata = subblocks;}


        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;
            virtual int32_t     do_read(base::xstream_t & stream)  override;
        private:
            uint8_t             m_version;
            uint16_t            m_sync_targets;    //responds targets
            uint32_t            m_sync_cookie;     //token for sync
            
            std::string         m_block_object;    //must have as proof,= vblock 'serialized data include header and certification
            std::string         m_input_resource;  //block 'input data,it might be nil  according m_sync_targets
            std::string         m_output_resource; //block 'outut data,it might be nil  according m_sync_targets
            std::string         m_output_offdata;
        };
        
    };//end of namespace of xconsensus
    
};//end of namespace of top
