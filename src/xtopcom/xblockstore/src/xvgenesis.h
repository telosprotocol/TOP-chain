// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"

namespace top
{
    namespace store
    {
        //create genesis block for each user_account
        class xgenesis_header : public base::xvheader_t
        {
        public:
            xgenesis_header(const std::string & account);
        protected:
            virtual ~xgenesis_header();
        private:
            xgenesis_header();
        };
        
        class xgenesis_cert : public base::xvqcert_t
        {
        public:
            xgenesis_cert(const std::string & account);
        protected:
            virtual ~xgenesis_cert();
        private:
            xgenesis_cert();
        };
        
        class xgenesis_block : public base::xvblock_t
        {
        public:
            static base::xvblock_t*  create_genesis_block(const std::string & account);
        public:
            xgenesis_block(xgenesis_header & header,xgenesis_cert & cert);
        protected:
            virtual ~xgenesis_block();
        private:
            xgenesis_block();
        };
        
    };//end of namespace of vstore
};//end of namespace of top
