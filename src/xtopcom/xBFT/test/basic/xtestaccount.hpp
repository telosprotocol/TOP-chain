// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xBFT/xconspdu.h"
#include "xBFT/xconsengine.h"
#include "xBFT/xconsaccount.h"

namespace top
{
    namespace test
    {
        class xtestaccount_t : public xconsensus::xcsaccount_t
        {
        public:
            xtestaccount_t(xconsensus::xcsobject_t & parent_object,const std::string & account_address);
            xtestaccount_t(base::xcontext_t & _context,const int32_t target_thread_id,const std::string & account_address);
        protected:
            virtual ~xtestaccount_t();
        private:
            xtestaccount_t();
            xtestaccount_t(const xtestaccount_t &);
            xtestaccount_t & operator = (const xtestaccount_t &);
        };
        
        /*
        class xtesttable_t : public xconsensus::xcstable_t
        {
        public:
            xtesttable_t(base::xcontext_t & _context,const int32_t target_thread_id)
                :xconsensus::xcstable_t(_context,target_thread_id)
            {
            }
        protected:
            virtual ~xtesttable_t()
            {
            }
        private:
            xtesttable_t();
            xtesttable_t(const xtesttable_t &);
            xtesttable_t & operator = (const xtesttable_t &);
        private:
            virtual xconsensus::xcsaccount_t*  create_account_object(base::xcontext_t & _context,const int32_t target_thread_id,const std::string & account_address)
            {
                return new xtestaccount_t(_context,target_thread_id,account_address);
            }
        };
        
        class xtestbook_t : public xconsensus::xcsbook_t
        {
        public:
            xtestbook_t(base::xcontext_t & _context,const int32_t target_thread_id)
                :xconsensus::xcsbook_t(_context,target_thread_id)
            {
            }
        protected:
            virtual ~xtestbook_t(){};
        private:
            xtestbook_t();
            xtestbook_t(const xtestbook_t &);
            xtestbook_t & operator = (const xtestbook_t &);
        private:
            virtual xconsensus::xcstable_t*  create_table_object(base::xcontext_t & _context,const int32_t target_thread_id)
            {
                return new xtesttable_t(_context,target_thread_id);
            }
        };
        
        class xtestledger_t : public xconsensus::xcsledger_t
        {
        public:
            xtestledger_t(base::xcontext_t & context,std::vector<uint32_t> thread_ids_array)
                :xconsensus::xcsledger_t(context,thread_ids_array)
            {
            }
        protected:
            virtual ~xtestledger_t()
            {
            }
        private:
            xtestledger_t();
            xtestledger_t(const xtestledger_t &);
            xtestledger_t & operator = (const xtestledger_t &);
        private:
            virtual xconsensus::xcsbook_t*  create_book_object(base::xcontext_t & _context,const int32_t target_thread_id)
            {
                return new xtestbook_t(_context,target_thread_id);
            }
        };
         */

    };
};
