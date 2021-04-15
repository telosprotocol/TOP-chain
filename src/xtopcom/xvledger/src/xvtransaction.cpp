// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvtransaction.h"

namespace top
{
    namespace base
    {
        xvtransaction_index_t::xvtransaction_index_t()
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine) {

        }
        xvtransaction_index_t::xvtransaction_index_t(uint64_t unit_height, const std::string & unit_hash, const std::string & txhash, enum_transaction_subtype type, xdataunit_t* raw_tx)
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine), m_unit_height(unit_height), m_unit_hash(unit_hash), m_tx_hash(txhash), m_tx_phase_type(type), m_raw_tx(raw_tx) {
            if (raw_tx != nullptr) {
                raw_tx->add_ref();
            }
        }
        xvtransaction_index_t::~xvtransaction_index_t() {
            if (nullptr != m_raw_tx) {
                m_raw_tx->release_ref();
            }
        }
        int32_t xvtransaction_index_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.write_compact_var(m_unit_height);
            stream.write_compact_var(m_unit_hash);
            stream.write_compact_var(m_tx_hash);
            stream.write_compact_var((uint8_t)m_tx_phase_type);
            uint8_t has_tx = m_raw_tx != nullptr;
            stream.write_compact_var(has_tx);
            if (has_tx) {
                m_raw_tx->serialize_to(stream);
            }
            return (stream.size() - begin_size);
        }

        int32_t xvtransaction_index_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            stream.read_compact_var(m_unit_height);
            stream.read_compact_var(m_unit_hash);
            stream.read_compact_var(m_tx_hash);
            uint8_t phase_type;
            stream.read_compact_var(phase_type);
            m_tx_phase_type = (enum_transaction_subtype)phase_type;
            uint8_t has_tx;
            stream.read_compact_var(has_tx);
            if (has_tx) {
                m_raw_tx = xdataunit_t::read_from(stream);
                xassert(m_raw_tx != nullptr);
            }
            return (begin_size - stream.size());
        }

        xvtransaction_store_t::xvtransaction_store_t()
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine) {

        }
        xvtransaction_store_t::~xvtransaction_store_t() {
            if (m_raw_tx != nullptr) {
                m_raw_tx->release_ref();
            }
        }

        int32_t xvtransaction_store_t::do_write(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint8_t has_tx = m_raw_tx != nullptr;
            stream.write_compact_var(has_tx);
            if (has_tx) {
                m_raw_tx->serialize_to(stream);
            }
            stream.write_compact_var(m_send_unit_height);
            stream.write_compact_var(m_recv_unit_height);
            stream.write_compact_var(m_confirm_unit_height);
            return (stream.size() - begin_size);
        }

        int32_t xvtransaction_store_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint8_t has_tx;
            stream.read_compact_var(has_tx);
            if (has_tx) {
                m_raw_tx = xdataunit_t::read_from(stream);
                xassert(m_raw_tx != nullptr);
            }
            stream.read_compact_var(m_send_unit_height);
            stream.read_compact_var(m_recv_unit_height);
            stream.read_compact_var(m_confirm_unit_height);
            return (begin_size - stream.size());
        }

        void xvtransaction_store_t::set_raw_tx(xdataunit_t* tx) {
            if (m_raw_tx == nullptr && tx != nullptr) {
                tx->add_ref();
                m_raw_tx = tx;
            }
        }

        void xvtransaction_store_t::set_send_unit_info(const xvtransaction_index_ptr_t & txindex) {
            m_send_unit_height = txindex->get_unit_height();
            m_send_unit_hash = txindex->get_unit_hash();
            m_is_self_tx = txindex->get_tx_phase_type() == enum_transaction_subtype_self;
            set_raw_tx(txindex->get_raw_tx());
        }
        void xvtransaction_store_t::set_recv_unit_info(const xvtransaction_index_ptr_t & txindex) {
            m_recv_unit_height = txindex->get_unit_height();
            m_recv_unit_hash = txindex->get_unit_hash();
            m_is_self_tx = false;
            set_raw_tx(txindex->get_raw_tx());
        }
        void xvtransaction_store_t::set_confirm_unit_info(const xvtransaction_index_ptr_t & txindex) {
            m_confirm_unit_height = txindex->get_unit_height();
            m_confirm_unit_hash = txindex->get_unit_hash();
            m_is_self_tx = false;
            set_raw_tx(txindex->get_raw_tx());
        }
    };//end of namespace of base
};//end of namespace of top
