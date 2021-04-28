// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvblock.h"
#include "xvtxindex.h"

namespace top
{
    namespace base
    {
        //xvtxstore_t manage index and associated transaction
        class xvtxstore_t : public xobject_t
        {
            friend class xvchain_t;
        public:
            static  const std::string   name(){return "xvtxstore";} //"xvblockstore"
            virtual std::string         get_obj_name() const override {return name();}

        protected:
            xvtxstore_t();
            virtual ~xvtxstore_t();
        private:
            xvtxstore_t(xvtxstore_t &&);
            xvtxstore_t(const xvtxstore_t &);
            xvtxstore_t & operator = (const xvtxstore_t &);
        public:
            //caller need to cast (void*) to related ptr
            virtual void*               query_interface(const int32_t _enum_xobject_type_) override;
 
        public://read & load interface
            virtual xauto_ptr<xvtxindex_t>  load_tx_idx(const std::string & raw_tx_hash,enum_transaction_subtype type);
            virtual const std::string       load_tx_bin(const std::string & raw_tx_hash);
            virtual xauto_ptr<xdataunit_t>  load_tx_obj(const std::string & raw_tx_hash);
            
        public: //write interface
            virtual bool                store_txs(xvblock_t * block_ptr,bool store_raw_tx_bin);
            virtual bool                store_tx_bin(const std::string & raw_tx_hash,const std::string & raw_tx_bin);
            virtual bool                store_tx_obj(const std::string & raw_tx_hash,xdataunit_t * raw_tx_obj);
        protected:
//            using xobject_t::add_ref;
//            using xobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
