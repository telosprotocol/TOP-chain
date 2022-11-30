// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xblockstore/xblockstore_face.h"
#include "xvledger/xvdbstore.h"
#include "xvledger/xveventbus.h"

namespace top
{
    namespace mock
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
            virtual std::string   get_value(const std::string & key) const override;

        public:
            virtual std::string         get_store_path() const  override {return m_store_path;}
        private:
            std::string                         m_store_path;
            std::map<std::string,std::string >  m_dumy_store;
        };

        class xveventbus_impl : public base::xveventbus_t
        {
        public:
            xveventbus_impl(){};
        protected:
            virtual ~xveventbus_impl(){};
        private:
            xveventbus_impl(xveventbus_impl &&);
            xveventbus_impl(const xveventbus_impl &);
            xveventbus_impl & operator = (const xveventbus_impl &);

        public: //api for event
            void   push_event(const mbus::xevent_ptr_t& e) override //push event into mbus system
            {

            }

        public://declares clasic events
            virtual mbus::xevent_ptr_t  create_event_for_store_index_to_db(base::xvbindex_t * target_index) {
                return nullptr;
            }
            virtual mbus::xevent_ptr_t  create_event_for_revoke_index_to_db(base::xvbindex_t * target_index) {
                return nullptr;
            }
            virtual mbus::xevent_ptr_t  create_event_for_store_block_to_db(base::xvblock_t * target_block) {
                return nullptr;
            }
            virtual mbus::xevent_ptr_t  create_event_for_store_committed_block(base::xvbindex_t * target_index) {
                return nullptr;
            }          
        };
    };
};
