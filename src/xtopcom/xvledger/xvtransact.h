// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xvtxindex.h"
#include "xvaction.h"

namespace top
{
    namespace base
    {
        //virtual transaction object
        class xvtransact_t : public xdataunit_t
        {
        protected:
            xvtransact_t();
            virtual ~xvtransact_t();
        private:
            xvtransact_t(xvtransact_t &&);
            xvtransact_t(const xvtransact_t &);
            xvtransact_t & operator = (xvtransact_t &&);
            xvtransact_t & operator = (const xvtransact_t &);

        public:
            virtual bool               is_valid()   const = 0; //verify signature,content,format etc
            virtual const std::string  get_hash()   const = 0; //get tx hash
            virtual const xvaction_t&  get_action() const = 0; //build and return
        };

        class xvtransaction_store_t : public xdataunit_t
        {
        public:
            xvtransaction_store_t();
        protected:
            virtual ~xvtransaction_store_t();
            virtual int32_t    do_write(base::xstream_t & stream) override;
            virtual int32_t    do_read(base::xstream_t & stream) override;

        public:
            void            set_send_block_info(const xvtxindex_ptr & txindex);
            void            set_recv_block_info(const xvtxindex_ptr & txindex);
            void            set_confirm_block_info(const xvtxindex_ptr & txindex);
            void            set_raw_tx(xdataunit_t* tx);

        public:
            bool            is_self_tx() const {return m_is_self_tx;}
            const std::string &     get_send_addr() const {return m_send_addr;}
            uint64_t        get_send_block_height() const {return m_send_block_height;}
            const std::string &     get_recv_addr() const {return m_recv_addr;}
            const std::string &     get_recv_block_hash() const {return m_recv_block_hash;}
            uint64_t        get_recv_block_height() const {return m_recv_block_height;}
            uint64_t        get_confirm_block_height() const {return m_confirm_block_height;}
            xdataunit_t*    get_raw_tx() const {return m_raw_tx;}  // TODO(jimmy) define xtransaction_t in xvledger

        public:
            xdataunit_t*        m_raw_tx{nullptr};
            bool                m_is_self_tx{false};
            std::string         m_send_addr;
            uint64_t            m_send_block_height{0};
            std::string         m_send_block_hash;
            std::string         m_recv_addr;
            uint64_t            m_recv_block_height{0};
            std::string         m_recv_block_hash;
            uint64_t            m_confirm_block_height{0};
            std::string         m_confirm_block_hash;
        };
        using xvtransaction_store_ptr_t = xobject_ptr_t<xvtransaction_store_t>;
    }//end of namespace of base

}//end of namespace top
