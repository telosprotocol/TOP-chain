// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"
#include "xvledger/xvtransact.h"
#include "xvledger/xvblockstore.h"

namespace top
{
    namespace test
    {
        class tep0_tx : public base::xvtransact_t
        {
        public:
            tep0_tx(const std::string & caller,const int change_balance); //>0 for deposit, < 0 for withdraw
            ~tep0_tx();
        private:
            tep0_tx();
            tep0_tx(const tep0_tx &);
            tep0_tx& operator = (const tep0_tx &);
        public:
            virtual bool                     is_valid()   const override; //verify signature,content,format etc
            virtual const std::string        get_hash()   const override; //get tx hash
            virtual const base::xvaction_t&  get_action() const override; //build and return
            
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(base::xstream_t & stream) override;     //write whole object to binary
            virtual int32_t     do_read(base::xstream_t & stream)  override;      //read from binary and regeneate content of xdataobj_t
            
            
        private:
            std::string         m_raw_tx_hash;
            base::xvaction_t*   m_raw_action;
        };
    
        class xunitheader_t : public base::xvheader_t
        {
        public:
            xunitheader_t(const std::string & account,uint64_t height,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,base::enum_xvblock_class block_class);
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
        
        class xunitblock_t : public base::xvblock_t
        {
        public:
            static xunitblock_t*  create_unitblock(const std::string & account,uint64_t height,uint64_t clock,uint64_t viewid,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height);
            
            static xunitblock_t*  create_genesis_block(const std::string & account);
        public:
            xunitblock_t(xunitheader_t & header,xunitcert_t & cert,base::xvinput_t* input,base::xvoutput_t* output);
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
        
     
        
    }
}

