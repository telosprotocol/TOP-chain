// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvdbkey.h"

namespace top
{
    namespace base
    {
        enum enum_transaction_subtype
        {
            enum_transaction_subtype_invalid   = 0,
            enum_transaction_subtype_self      = 1,  // self operate
            enum_transaction_subtype_send      = 2,  // send to other account
            enum_transaction_subtype_recv      = 3,  // receive from other account
            enum_transaction_subtype_confirm   = 4,  // receive ack from other account
            enum_transaction_subtype_all       = 0xFF,
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

            int32_t do_write(base::xstream_t & stream) const;
            int32_t do_read(base::xstream_t & stream);

        private:
            std::string                 m_txhash;
            enum_transaction_subtype    m_subtype;
        };

        class xvtxkey_vec_t {
        public:
            int32_t serialize_to_string(std::string & str) const;
            int32_t do_write(base::xstream_t & stream) const;
            int32_t serialize_from_string(const std::string & _data);
            int32_t do_read(base::xstream_t & stream);
            void push_back(const xvtxkey_t & key);
            const std::vector<xvtxkey_t> & get_txkeys() const;
        private:
            std::vector<xvtxkey_t> m_vec;
        };

        class xvblock_t;
        class xvtxindex_t : public xdataunit_t
        {
            friend class xvtxstore_t;
        public:
            xvtxindex_t();
            xvtxindex_t(xvblock_t & owner, const std::string & txhash, enum_transaction_subtype type);
        protected:
            virtual ~xvtxindex_t();
        private:
            xvtxindex_t(const xvtxindex_t &);
            xvtxindex_t(xvtxindex_t &&);
            xvtxindex_t & operator= (const xvtxindex_t &);

        public:
            inline const std::string &         get_block_addr()     const {return m_block_addr;}
            inline const std::string &         get_block_hash()     const {return m_block_hash;}
            inline const uint64_t              get_block_height()   const {return m_block_height;}
            inline const int                   get_block_flags()    const {return (((int)m_block_flags) << 8);}

            inline const std::string &         get_tx_hash()        const {xassert(m_tx_hash!="");return m_tx_hash;}
            inline enum_transaction_subtype    get_tx_phase_type()  const {return (enum_transaction_subtype)m_tx_phase_type;}
            inline bool                        is_self_tx() const {return m_tx_phase_type == enum_transaction_subtype_self;}
            const uint64_t                     get_block_clock()   const;
            virtual std::string                dump()              const override;
        public:
            void set_tx_hash(std::string const & tx_hash);
            void set_block_addr(const std::string & block_addr) {m_block_addr = block_addr;}
            void set_block_height(uint64_t block_height) {m_block_height = block_height;}
            void set_block_hash(const std::string & hash) {m_block_hash = hash;}
        protected:
            virtual int32_t    do_write(base::xstream_t & stream) override;
            virtual int32_t    do_read(base::xstream_t & stream) override;

        private:
            std::string                 m_block_addr;   //associated block 'address
            std::string                 m_block_hash;   //associated block 'hash
            uint64_t                    m_block_height; //associated block 'height
            std::string                 m_tx_hash;      //raw tx 'hash
            uint8_t                     m_tx_phase_type;//refer enum_transaction_subtype
            uint8_t                     m_block_flags;  //refer enum_xvblock_flag,here just stored 8bit to save space
        };
        using xvtxindex_ptr             = xobject_ptr_t<xvtxindex_t>;//define short type

    }//end of namespace of base

}//end of namespace top
