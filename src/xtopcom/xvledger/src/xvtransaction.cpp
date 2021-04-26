// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include <sstream>
#include "xbase/xutl.h"
#include "../xvtransaction.h"

namespace top
{
    namespace base
    {
        std::string xvtxkey_t::transaction_subtype_to_string(enum_transaction_subtype type) {
            switch (type) {
                case enum_transaction_subtype_self: return "self";
                case enum_transaction_subtype_send: return "send";
                case enum_transaction_subtype_recv: return "recv";
                case enum_transaction_subtype_confirm: return "confirm";
                default: {
                    xassert(0);
                    return "unknown";
                }
            }
        }

        std::string xvtxkey_t::transaction_hash_subtype_to_string(const std::string & txhash, enum_transaction_subtype type) {
            return base::xstring_utl::to_hex(txhash) + ":" + transaction_subtype_to_string(type);
        }

        enum_txindex_type xvtxkey_t::transaction_subtype_to_txindex_type(enum_transaction_subtype type) {
            switch (type) {
                case enum_transaction_subtype_self: return enum_txindex_type_send;
                case enum_transaction_subtype_send: return enum_txindex_type_send;
                case enum_transaction_subtype_recv: return enum_txindex_type_receive;
                case enum_transaction_subtype_confirm: return enum_txindex_type_confirm;
                default: {
                    xassert(0);
                    return enum_txindex_type_send;
                }
            }
        }

        std::string xvtxkey_t::get_tx_dump_key() const {
            return transaction_hash_subtype_to_string(m_txhash, m_subtype);
        }

        uint256_t xvtxkey_t::get_tx_hash_256() const {
            return uint256_t((uint8_t*)m_txhash.data());
        }
        std::string xvtxkey_t::get_tx_hex_hash() const {
            return base::xstring_utl::to_hex(m_txhash);
        }
        std::string xvtxkey_t::get_tx_subtype_str() const {
            return transaction_subtype_to_string(m_subtype);
        }

        int32_t xvtxkey_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            // use old serialize method for compatibility
            stream << m_txhash;
            stream << (uint8_t)m_subtype;
            return (stream.size() - begin_size);
        }
        int32_t xvtxkey_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream >> m_txhash;
            uint8_t subtype_uint8;
            stream >> subtype_uint8;
            m_subtype = (enum_transaction_subtype)subtype_uint8;
            return (begin_size - stream.size());
        }

        // TODO(jimmy) add object type
        xvtxindex_t::xvtxindex_t()
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine) {

        }
        xvtxindex_t::xvtxindex_t(uint64_t unit_height, const std::string & unit_hash, const std::string & txhash, enum_transaction_subtype type)
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine), m_tx_key(txhash, type), m_unit_height(unit_height), m_unit_hash(unit_hash) {
        }
        xvtxindex_t::xvtxindex_t(uint64_t unit_height, const std::string & unit_hash, const xvtxkey_t & txkey)
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine), m_tx_key(txkey), m_unit_height(unit_height), m_unit_hash(unit_hash) {
        }
        xvtxindex_t::~xvtxindex_t() {
        }
        int32_t xvtxindex_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            m_tx_key.do_write(stream);
            stream.write_compact_var(m_unit_height);
            stream.write_compact_var(m_unit_hash);
            return (stream.size() - begin_size);
        }

        int32_t xvtxindex_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            m_tx_key.do_read(stream);
            stream.read_compact_var(m_unit_height);
            stream.read_compact_var(m_unit_hash);
            return (begin_size - stream.size());
        }

        std::string xvtxindex_t::dump() const {
            std::stringstream ss;
            ss << "" << m_tx_key.get_tx_dump_key();
            ss << ",height=" << m_unit_height;
            ss << ",hash=" << base::xstring_utl::to_hex(m_unit_hash);
            return ss.str();
        }


        xtx_extract_info_t::xtx_extract_info_t(const xvtxindex_ptr_t & txindex, const xobject_ptr_t<xdataunit_t> & raw_tx) {
            m_txindex = txindex;
            m_raw_tx = raw_tx;
        }

        void xvtransaction_store_t::set_raw_tx(const xobject_ptr_t<xdataunit_t> & tx) {
            m_raw_tx = tx;
        }

        void xvtransaction_store_t::set_send_unit_info(const xvtxindex_ptr_t & txindex) {
            xassert(txindex->get_tx_key().is_self_tx() || txindex->get_tx_key().is_send_tx());
            xassert(txindex->get_unit_height() > 0);
            xassert(m_send_unit_height == 0 && m_send_unit_hash.empty());
            m_send_unit_height = txindex->get_unit_height();
            m_send_unit_hash = txindex->get_unit_hash();
            m_is_self_tx = txindex->get_tx_key().is_self_tx();
        }
        void xvtransaction_store_t::set_recv_unit_info(const xvtxindex_ptr_t & txindex) {
            xassert(txindex->get_tx_key().is_recv_tx());
            xassert(txindex->get_unit_height() > 0);
            xassert(m_recv_unit_height == 0 && m_recv_unit_hash.empty());
            m_recv_unit_height = txindex->get_unit_height();
            m_recv_unit_hash = txindex->get_unit_hash();
            m_is_self_tx = txindex->get_tx_key().is_self_tx();
        }
        void xvtransaction_store_t::set_confirm_unit_info(const xvtxindex_ptr_t & txindex) {
            xassert(txindex->get_tx_key().is_confirm_tx());
            xassert(txindex->get_unit_height() > 0);
            xassert(m_confirm_unit_height == 0 && m_confirm_unit_hash.empty());
            m_confirm_unit_height = txindex->get_unit_height();
            m_confirm_unit_hash = txindex->get_unit_hash();
            m_is_self_tx = txindex->get_tx_key().is_self_tx();
        }
    };//end of namespace of base
};//end of namespace of top
