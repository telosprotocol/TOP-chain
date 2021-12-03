// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvblock.h"
#include "xvtxindex.h"

namespace top {
namespace data {
// fwd
struct xtransaction_cache_data_t;
class xtransaction_t;
using xtransaction_ptr_t = xobject_ptr_t<xtransaction_t>;
}  // namespace data
}  // namespace top

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
            virtual xauto_ptr<xvtxindex_t>  load_tx_idx(const std::string & raw_tx_hash,enum_transaction_subtype type) = 0;
            virtual const std::string       load_tx_bin(const std::string & raw_tx_hash) = 0 ;
            virtual xauto_ptr<xdataunit_t>  load_tx_obj(const std::string & raw_tx_hash) = 0;

        public:
            virtual void update_node_type(uint32_t combined_node_type) = 0;

        public: //write interface
            virtual bool                store_txs(xvblock_t * block_ptr) = 0;
            virtual bool                store_tx_bin(const std::string & raw_tx_hash,const std::string & raw_tx_bin) = 0;
            virtual bool                store_tx_obj(const std::string & raw_tx_hash,xdataunit_t * raw_tx_obj) = 0;
        
        public: // tx cache interface
            virtual bool tx_cache_add(std::string const & tx_hash, data::xtransaction_ptr_t tx_ptr) = 0;
            virtual bool tx_cache_get(std::string const & tx_hash, std::shared_ptr<data::xtransaction_cache_data_t> tx_cache_data) = 0;

        protected:
//            using xobject_t::add_ref;
//            using xobject_t::release_ref;
        };

    }//end of namespace of base
}//end of namespace top
