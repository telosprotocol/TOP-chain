// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// TODO(jimmy) #include "xbase/xvledger.h"

namespace top
{
    namespace mock
    {
        class xunitheader_t : public base::xvheader_t
        {
        public:
            xunitheader_t(const std::string & account,uint64_t height,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,const std::string & body_input);
        protected:
            virtual ~xunitheader_t();
        private:
            xunitheader_t();
            xunitheader_t(const xunitheader_t &);
            xunitheader_t & operator = (const xunitheader_t &);
        };

        class xunitcert_t : public base::xvqcert_t
        {
        public:
            xunitcert_t();
        protected:
            virtual ~xunitcert_t();
        private:
            xunitcert_t(const xunitcert_t &);
            xunitcert_t & operator = (const xunitcert_t &);
        };

        class xclockblock_t : public base::xvblock_t
        {
        public:
            static xclockblock_t*  create_clockblock(const std::string & account,uint64_t height,uint64_t clock,uint64_t viewid,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,const std::string & body_input,const std::string & body_output);
        public:
            xclockblock_t(xunitheader_t & header,xunitcert_t & cert);
        protected:
            virtual ~xclockblock_t();
        private:
            xclockblock_t();
            xclockblock_t(const xclockblock_t &);
            xclockblock_t & operator = (const xclockblock_t &);
        };

        class xunitblock_t : public base::xvblock_t
        {
        public:
            static xunitblock_t*  create_unitblock(const std::string & account,uint64_t height,uint64_t clock,uint64_t viewid,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,const std::string & body_input,const std::string & body_output);
        public:
            xunitblock_t(xunitheader_t & header,xunitcert_t & cert);
        protected:
            virtual ~xunitblock_t();
        private:
            xunitblock_t();
            xunitblock_t(const xunitblock_t &);
            xunitblock_t & operator = (const xunitblock_t &);
        public:
            #ifdef DEBUG //tracking memory of proposal block
            virtual int32_t   add_ref() override;
            virtual int32_t   release_ref() override;
            #endif
        };

        //test only,each chainid shared one xunitblockstore_t
        class xunitblockstore_t : public base::xvblockstore_t
        {
        public:
            xunitblockstore_t();
        protected:
            virtual ~xunitblockstore_t();
        private:
            xunitblockstore_t(const xunitblockstore_t &);
            xunitblockstore_t & operator = (const xunitblockstore_t &);
        public:
            base::xvblock_t*            create_clock_block(const std::string & account,const std::string & block_input,const std::string & block_output);

            base::xvblock_t*            create_proposal_block(const std::string & account,const std::string & block_input,const std::string & block_output);

            //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
            virtual std::string         get_store_path() const override;//each store may has own space at DB/disk

        public: //return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
            virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const std::string & account) override;//genesis block
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const std::string & account)override;//highest view# for any status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const std::string & account)    override;//block with locked status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const std::string & account) override;//block with committed status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const std::string & account)  override;//block with executed status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const std::string & account) override;//block connected to genesis
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_full_block(const std::string & account)  override;//block has full state,genesis is a full block
            //just load vblock object but not load header and body those need load seperately if need.
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const std::string & account,const uint64_t height,bool ask_full_load = true) override;

            virtual bool                load_block_input(base::xvblock_t* block)  override;//load and assign input data into block
            virtual bool                load_block_output(base::xvblock_t* block) override;//load and assign output data into block

            virtual bool                store_block(base::xvblock_t* block)  override; //return false if fail to store
            virtual bool                delete_block(base::xvblock_t* block) override; //return false if fail to delete

        public://better performance,and return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect

            virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const base::xvaccount_t & account)  override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const base::xvaccount_t & account)   override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const base::xvaccount_t & account)override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const base::xvaccount_t & account)override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_full_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,bool ask_full_load = true)override;

            virtual bool                load_block_input(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                load_block_output(const base::xvaccount_t & account,base::xvblock_t* block) override;

            virtual bool                store_block(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                delete_block(const base::xvaccount_t & account,base::xvblock_t* block) override;

        public://batch process api
            virtual base::xblock_mptrs  get_latest_blocks(const base::xvaccount_t & account) override;
            virtual bool                store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks) override;

        protected:
            virtual const std::string   load_value_by_path(const std::string & full_path_as_key);
            virtual bool                delete_value_by_path(const std::string & full_path_as_key);
            virtual bool                store_value_by_path(const std::string & full_path_as_key,const std::string & value);

        protected:
            virtual int32_t             do_write(base::xstream_t & stream) override;//serialize whole object to binary
            virtual int32_t             do_read(base::xstream_t & stream) override; //serialize from binary and regeneate content of xdataobj_t
        private:
            // < account: <height,block*> > sort from lower height to higher,and the first one block is genesis block
            std::map< std::string,std::map<uint64_t,base::xvblock_t*> > m_blocks;
            std::map<std::string,std::string >                        m_dumy_store;
        };

    }
}

