// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvdbkey.h"

namespace top
{
    namespace base
    {
        enum enum_transaction_subtype {
            enum_transaction_subtype_invalid   = 0,
            enum_transaction_subtype_self      = 1,  // self operate
            enum_transaction_subtype_send      = 2,  // send to other account
            enum_transaction_subtype_recv      = 3,  // receive from other account
            enum_transaction_subtype_confirm   = 4,  // receive ack from other account
        };

        enum enum_tx_dbkey_type {
            enum_tx_dbkey_type_sendindex        = 0x01,  // send tx index
            enum_tx_dbkey_type_recvindex        = 0x02,  // recv tx index
            enum_tx_dbkey_type_confirmindex     = 0x04,  // confirm tx index
            enum_tx_dbkey_type_raw              = 0x08,  // raw tx content
            enum_tx_dbkey_type_all              = 0x0F,  // mask all tx dbkey
        };

        // the tx key include tx hash and tx subtype
        class xvtxkey_t {
        public:
            static std::string          transaction_subtype_to_string(enum_transaction_subtype type);
            static std::string          transaction_hash_subtype_to_string(const std::string & txhash, enum_transaction_subtype type);
            static enum_txindex_type    transaction_subtype_to_txindex_type(enum_transaction_subtype type);
        public:
            xvtxkey_t() = default;
            xvtxkey_t(const std::string & txhash, enum_transaction_subtype subtype)
            : m_txhash(txhash), m_subtype(subtype) {}

            bool                        is_self_tx() const {return get_tx_subtype() == enum_transaction_subtype_self;}
            bool                        is_send_tx() const {return get_tx_subtype() == enum_transaction_subtype_send;}
            bool                        is_recv_tx() const {return get_tx_subtype() == enum_transaction_subtype_recv;}
            bool                        is_confirm_tx() const {return get_tx_subtype() == enum_transaction_subtype_confirm;}
            const std::string &         get_tx_hash() const {return m_txhash;}
            std::string                 get_tx_dump_key() const;
            enum_transaction_subtype    get_tx_subtype() const {return m_subtype;}
            uint256_t                   get_tx_hash_256() const;
            std::string                 get_tx_hex_hash() const;
            std::string                 get_tx_subtype_str() const;

            int32_t do_write(base::xstream_t & stream);
            int32_t do_read(base::xstream_t & stream);

        private:
            std::string                 m_txhash;
            enum_transaction_subtype    m_subtype;
        };

        class xvtxindex_t : public xdataunit_t
        {
        public:
            xvtxindex_t();
            xvtxindex_t(uint64_t unit_height, const std::string & unit_hash, const std::string & txhash, enum_transaction_subtype type);
            xvtxindex_t(uint64_t unit_height, const std::string & unit_hash, const xvtxkey_t & txkey);
        protected:
            virtual ~xvtxindex_t();
            virtual int32_t    do_write(base::xstream_t & stream) override;
            virtual int32_t    do_read(base::xstream_t & stream) override;

        public:
            const xvtxkey_t &           get_tx_key() const {return m_tx_key;}
            bool                        is_self_tx() const {return m_tx_key.is_self_tx();}
            uint64_t                    get_unit_height() const {return m_unit_height;}
            const std::string &         get_unit_hash() const {return m_unit_hash;}
            std::string                 dump() const;

        private:
            xvtxkey_t                   m_tx_key;
            uint64_t                    m_unit_height;
            std::string                 m_unit_hash;
            // xtable_shortid_t            m_source_tableid;  // TODO(jimmy)
            // xtable_shortid_t            m_target_tableid;
            // uint64_t                    m_receiptid;
        };

        using xvtxindex_ptr_t = xobject_ptr_t<xvtxindex_t>;

        class xtx_extract_info_t {
        public:
            xtx_extract_info_t(const xvtxindex_ptr_t & txindex, const xobject_ptr_t<xdataunit_t> & raw_tx);

            const xvtxindex_ptr_t &             get_tx_index() const {return m_txindex;}
            const xobject_ptr_t<xdataunit_t> &  get_raw_tx() const {return m_raw_tx;}

        private:
            xvtxindex_ptr_t             m_txindex{nullptr};
            xobject_ptr_t<xdataunit_t>  m_raw_tx{nullptr};
        };

        class xvtransaction_store_t {
        public:
            xvtransaction_store_t() = default;
            ~xvtransaction_store_t() = default;

        public:
            void            set_send_unit_info(const xvtxindex_ptr_t & txindex);
            void            set_recv_unit_info(const xvtxindex_ptr_t & txindex);
            void            set_confirm_unit_info(const xvtxindex_ptr_t & txindex);
            void            set_raw_tx(const xobject_ptr_t<xdataunit_t> &  tx);

        public:
            bool            is_self_tx() const {return m_is_self_tx;}
            uint64_t        get_send_unit_height() const {return m_send_unit_height;}
            uint64_t        get_recv_unit_height() const {return m_recv_unit_height;}
            uint64_t        get_confirm_unit_height() const {return m_confirm_unit_height;}
            const xobject_ptr_t<xdataunit_t> &  get_raw_tx() const {return m_raw_tx;}  // TODO(jimmy) define xtransaction_t in xvledger

        public:
            xobject_ptr_t<xdataunit_t>  m_raw_tx{nullptr};
            bool                m_is_self_tx{false};
            uint64_t            m_send_unit_height{0};
            std::string         m_send_unit_hash;
            uint64_t            m_recv_unit_height{0};
            std::string         m_recv_unit_hash;
            uint64_t            m_confirm_unit_height{0};
            std::string         m_confirm_unit_hash;
        };
        using xvtransaction_store_ptr_t = std::shared_ptr<xvtransaction_store_t>;
    }//end of namespace of base

}//end of namespace top
