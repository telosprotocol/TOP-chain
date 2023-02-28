// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvblock.h"
#include "../xvtxindex.h"
#include "xmetrics/xmetrics.h"

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

        int32_t xvtxkey_t::do_write(base::xstream_t & stream) const {
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

        int32_t xvtxkey_vec_t::serialize_to_string(std::string & str) const {
            base::xstream_t _stream(base::xcontext_t::instance());
            do_write(_stream);
            str.clear();
            str.assign((const char*)_stream.data(), _stream.size());
            return str.size();
        }

        int32_t xvtxkey_vec_t::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            stream << static_cast<uint32_t>(m_vec.size());
            for (auto key : m_vec) {
                key.do_write(stream);
            }
            return (stream.size() - begin_size);
        }
        
        int32_t xvtxkey_vec_t::serialize_from_string(const std::string & _data) {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)_data.data(),(uint32_t)_data.size());
            const int result = do_read(_stream);
            return result;
        }

        int32_t xvtxkey_vec_t::do_read(base::xstream_t & stream) {
            const int32_t begin_size = stream.size();
            uint32_t size;
            stream >> size;
            for (uint32_t i = 0; i < size; ++i) {
                xvtxkey_t key;
                key.do_read(stream);
                m_vec.push_back(key);
            }
            return (begin_size - stream.size());
        }

        void xvtxkey_vec_t::push_back(const xvtxkey_t & key) {
            m_vec.push_back(key);
        }

        const std::vector<xvtxkey_t> & xvtxkey_vec_t::get_txkeys() const {
            return m_vec;
        }

        xvtxindex_t::xvtxindex_t()
            : xdataunit_t(xdataunit_t::enum_xdata_type_undefine)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvtxindex, 1);
            m_block_flags = 0;
            m_block_height = 0;
            m_tx_phase_type = 0;
        }

        xvtxindex_t::xvtxindex_t(xvblock_t & owner, const std::string & txhash, enum_transaction_subtype type)
        : xdataunit_t(xdataunit_t::enum_xdata_type_undefine)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvtxindex, 1);
            m_block_addr    = owner.get_account();
            m_block_height  = owner.get_height();
            m_block_hash    = owner.get_block_hash();
            m_tx_hash       = txhash;
            m_tx_phase_type = type;
            m_block_flags   = (owner.get_block_flags() >> 8); //lowest 8bit is meaning less,so just skip it
        }

        xvtxindex_t::~xvtxindex_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvtxindex, -1);
        }

        void xvtxindex_t::set_tx_hash(std::string const & tx_hash)
        {
            xassert(m_tx_hash == "");
            m_tx_hash = tx_hash;
        }
        // void xvtxindex_t::set_tx_phase_type(enum_transaction_subtype tx_phase_type)
        // {
        //     xassert(m_tx_phase_type == enum_transaction_subtype_invalid);
        //     m_tx_phase_type = tx_phase_type;
        // }

        int32_t xvtxindex_t::do_write(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();

            stream.write_compact_var(m_block_addr);
            stream.write_compact_var(m_block_height);
            stream.write_compact_var(m_block_hash);
            stream.write_compact_var(std::string{""});

            stream << m_tx_phase_type;
            stream << m_block_flags;
            return (stream.size() - begin_size);
        }

        int32_t xvtxindex_t::do_read(base::xstream_t & stream)
        {
            const int32_t begin_size = stream.size();

            stream.read_compact_var(m_block_addr);
            stream.read_compact_var(m_block_height);
            stream.read_compact_var(m_block_hash);
            std::string _str{""};
            stream.read_compact_var(_str);

            stream >> m_tx_phase_type;
            stream >> m_block_flags;
            return (begin_size - stream.size());
        }

        const uint64_t xvtxindex_t::get_block_clock()   const
        {
            return 0;
        }

        std::string xvtxindex_t::dump() const
        {
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"tx=%s,addr=%s,height=%ld,hash=%s", 
                xvtxkey_t::transaction_hash_subtype_to_string(m_tx_hash, (enum_transaction_subtype)m_tx_phase_type).c_str(), m_block_addr.c_str(), m_block_height, base::xstring_utl::to_hex(m_block_hash).c_str());
            return std::string(local_param_buf);
        }

    };//end of namespace of base
};//end of namespace of top
