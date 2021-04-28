// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xvtxindex.h"

namespace top
{
    namespace base
    {

        class xvtransaction_store_t : public xdataunit_t
        {
        public:
            xvtransaction_store_t();
        protected:
            virtual ~xvtransaction_store_t();
            virtual int32_t    do_write(base::xstream_t & stream) override;
            virtual int32_t    do_read(base::xstream_t & stream) override;

        public:
            void            set_send_unit_info(const xvtxindex_ptr & txindex);
            void            set_recv_unit_info(const xvtxindex_ptr & txindex);
            void            set_confirm_unit_info(const xvtxindex_ptr & txindex);
            void            set_raw_tx(xdataunit_t* tx);

        public:
            bool            is_self_tx() const {return m_is_self_tx;}
            uint64_t        get_send_unit_height() const {return m_send_unit_height;}
            uint64_t        get_recv_unit_height() const {return m_recv_unit_height;}
            uint64_t        get_confirm_unit_height() const {return m_confirm_unit_height;}
            xdataunit_t*    get_raw_tx() const {return m_raw_tx;}  // TODO(jimmy) define xtransaction_t in xvledger

        public:
            xdataunit_t*        m_raw_tx{nullptr};
            bool                m_is_self_tx{false};
            uint64_t            m_send_unit_height{0};
            std::string         m_send_unit_hash;
            uint64_t            m_recv_unit_height{0};
            std::string         m_recv_unit_hash;
            uint64_t            m_confirm_unit_height{0};
            std::string         m_confirm_unit_hash;
        };
        using xvtransaction_store_ptr_t = xobject_ptr_t<xvtransaction_store_t>;
    }//end of namespace of base

}//end of namespace top
