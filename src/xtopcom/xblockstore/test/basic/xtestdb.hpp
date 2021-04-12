// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xblockstore/xblockstore_face.h"
#include "xvledger/xvdbstore.h"

namespace top
{
    namespace test
    {
        class xstoredb_t : public base::xvdbstore_t
        {
        public:
            xstoredb_t(const std::string & store_path);
        protected:
            virtual ~xstoredb_t();
        private:
            xstoredb_t();
            xstoredb_t(const xstoredb_t &);
            xstoredb_t & operator = (const xstoredb_t &);
            
        public://block object manage
            virtual bool             set_vblock(const std::string & store_path,base::xvblock_t* block) override;
            virtual bool             set_vblock_header(const std::string & store_path,base::xvblock_t* block) override;
            
            virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, const uint64_t height) const override;
            virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account,const uint64_t height) const override;
            virtual bool             get_vblock_input(const std::string & store_path,base::xvblock_t* for_block)  const override;
            virtual bool             get_vblock_output(const std::string & store_path,base::xvblock_t* for_block) const override;
            
        public://key-value manage
            virtual bool                set_value(const std::string & key, const std::string& value) override;
            virtual bool                delete_value(const std::string & key) override;
            virtual const std::string   get_value(const std::string & key) const override;
            virtual bool                find_values(const std::string & key,std::vector<std::string> & values) override;//support wild search
            
        public:
            virtual std::string         get_store_path() const  override {return m_store_path;}
        private:
            std::string                         m_store_path;
            std::map<std::string,std::string >  m_dumy_store;
        };
    };
};
