// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/fixed_hash.h"
#include "xethreceipt.h"
#include "xethtransaction.h"
#include "xevm_common/rlp.h"

namespace top {
namespace data {

    enum enum_block_cache_type {
        cache_tx_block,
        cache_poly_tx_block,
        cache_poly_election_block,
        cache_error_block,
    };
    struct xrelay_election_node_t {
        xrelay_election_node_t() = default;
        xrelay_election_node_t(const evm_common::h256& pubkey_x, const evm_common::h256& pubkey_y)
            : public_key_x(pubkey_x)
            , public_key_y(pubkey_y)
        {
        }

        void streamRLP(evm_common::RLPStream& _s) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);
        std::string get_pubkey_str() const;

        evm_common::h256 public_key_x; // election key
        evm_common::h256 public_key_y;
    };

    struct xrelay_election_group_t {
        xrelay_election_group_t() = default;
        void streamRLP(evm_common::RLPStream& _s) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);
        size_t size() const { return elections_vector.size(); }
        void dump() const;
        const bool empty() const { return elections_vector.empty(); }

        uint64_t election_epochID { 0 }; // epoch id for these elections
        std::vector<xrelay_election_node_t> elections_vector;
    };

    struct xrelay_signature_t {
        xrelay_signature_t() = default;
        xrelay_signature_t(const std::string& sign_str);
        void streamRLP(evm_common::RLPStream& _s) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);
        std::string to_string() const;

        evm_common::h256 r;
        evm_common::h256 s;
        evm_common::byte v { 0 };
    };

    struct xrelay_signature_node_t {
        xrelay_signature_node_t() = default;
        xrelay_signature_node_t(const std::string& sign_str);
        void streamRLP(evm_common::RLPStream& _s) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);

        bool exist { false }; // check signature if exist
        xrelay_signature_t signature;
    };

    struct xrelay_signature_group_t {
        xrelay_signature_group_t() = default;
        xbytes_t    encodeBytes() const;
        void        decodeBytes(xbytes_t const& _d, std::error_code & ec);   
        void streamRLP(evm_common::RLPStream& _s) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);
        size_t size() const { return signature_vector.size(); }
        void dump() const;
        const bool empty() const { return signature_vector.empty(); }

        uint64_t signature_epochID { 0 }; // epoch id for signature
        std::vector<xrelay_signature_node_t> signature_vector;
    };

    // extend datas
    struct xrelay_extend_data {
        xrelay_extend_data() = default;
        void streamRLP(evm_common::RLPStream& _s) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);

        uint64_t signature_viewID { 0 };
        evm_common::u256 chain_bits { 0 };
        evm_common::h256 build_additional_hash();
    };

    class xrelay_block_header {
        friend class xrelay_block;

    public:
        xrelay_block_header() = default;
        static const size_t block_header_fileds = 7;

        xbytes_t encodeBytes() const;
        void decodeBytes(xbytes_t const& _d, std::error_code& ec);

    public:
        void set_txs_root_hash(evm_common::h256 hash);
        void set_receipts_root_hash(evm_common::h256 hash);
        void set_block_merkle_root_hash(evm_common::h256 hash);
        void set_prev_hash(evm_common::h256 hash);
        void set_block_height(uint64_t height);
        void set_timestamp(uint64_t timestamp);
        void set_elections_next(const xrelay_election_group_t& elections);

    public:
        const evm_common::h256& get_txs_root_hash() const { return m_txs_merkle_root; }
        const evm_common::h256& get_receipts_root_hash() const { return m_receipts_merkle_root; }
        const evm_common::h256& get_block_merkle_root_hash() const { return m_block_merkle_root; }
        const uint64_t get_block_height() const { return m_height; }
        const uint64_t get_timestamp() const { return m_timestamp; }
        const evm_common::h256& get_prev_block_hash() const { return m_prev_hash; }
        const uint8_t get_header_version() const { return m_version; }
        const xrelay_election_group_t& get_elections_sets() const { return m_next_elections_groups; }
        evm_common::h256 get_header_hash();

    protected:
        void streamRLP(evm_common::RLPStream& rlp_stream) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);

    private:
        uint8_t m_version { 0 };
        uint64_t m_height { 0 }; // block height
        uint64_t m_timestamp { 0 }; // Timestamp at which the block was built.
        evm_common::h256 m_txs_merkle_root; // root hash of the  transactions in the given block. now is null
        evm_common::h256 m_receipts_merkle_root; // root hash of the  receipts in the given block.
        evm_common::h256 m_block_merkle_root; // Merkle root of block hashes up to the current block.
        evm_common::h256 m_prev_hash { 0 }; // hash ot prev block
        xrelay_election_group_t m_next_elections_groups; // vector of elections info
    };

    class xrelay_block {

    public:
        static const unsigned block_fileds = 6;

        xrelay_block() {}
        xrelay_block(evm_common::h256  prev_hash, uint64_t block_height, uint64_t timestamp, evm_common::u256 chain_bits);
        xrelay_block(evm_common::h256  prev_hash, uint64_t block_height, uint64_t timestamp, evm_common::u256 chain_bits, 
                     xrelay_election_group_t &election_group);
        xrelay_block(evm_common::h256  prev_hash, uint64_t block_height, uint64_t timestamp, evm_common::u256 chain_bits,
                     std::vector<xeth_transaction_t> transactions,  std::vector<xeth_receipt_t> &receipts);

        ~xrelay_block() { }

    public:
        xbytes_t encodeBytes() const;
        void decodeBytes(xbytes_t const& _d, std::error_code& ec);
        // todo
        xbytes_t streamRLP_header_to_contract();

        void set_chain_bits(const evm_common::u256& chain_bits);
        void set_epochid(const uint64_t epochid);
        void set_viewid(const uint64_t viewID);
        void set_elections_next(const xrelay_election_group_t& elections);
        void set_transactions(const std::vector<xeth_transaction_t>& transactions);
        void set_receipts(const std::vector<xeth_receipt_t>& receipts);
        void set_header(xrelay_block_header& block_hedaer);
        void set_signature_nodes(std::vector<xrelay_signature_node_t>& signature_nodes);
        void set_signature_groups(const xrelay_signature_group_t& signature_groups);
        void set_block_merkle_root_hash(evm_common::h256 hash);
        void make_merkle_root_hash(const std::vector<evm_common::h256>& hash_vector);
        void build_finish();

    public:
        const evm_common::u256& get_chain_bits() const { return m_exteend_data.chain_bits; }
        const uint64_t get_epochid() const { return m_signatures_groups.signature_epochID; }
        const uint64_t get_viewid() const { return m_exteend_data.signature_viewID; }
        uint8_t get_block_version() const { return m_version; }
        xrelay_block_header& get_header() { return m_header; }
        const evm_common::h256& get_block_hash() const { return m_block_hash; }
        const evm_common::h256& get_txs_root_hash() const { return m_header.get_txs_root_hash(); }
        const evm_common::h256& get_receipts_root_hash() const { return m_header.get_receipts_root_hash(); }
        const evm_common::h256& get_block_merkle_root_hash() const { return m_header.get_block_merkle_root_hash(); }
        const uint64_t get_block_height() const { return m_header.get_block_height(); }
        const uint64_t get_timestamp() const { return m_header.get_timestamp(); }
        const std::vector<xeth_receipt_t>& get_all_receipts() const { return m_receipts; }
        const std::vector<xeth_transaction_t>& get_all_transactions() const { return m_transactions; }
        const xrelay_election_group_t& get_elections_sets() const { return m_header.get_elections_sets(); }
        const evm_common::h256 build_signature_hash();
        const std::vector<xrelay_signature_node_t>& get_signatures_sets() const { return m_signatures_groups.signature_vector; }
        const xrelay_signature_group_t& get_signatures_group() const { return m_signatures_groups; }

        std::string dump() const;
        enum_block_cache_type check_block_type() const;
        std::string get_block_type_string();

    protected:
        void streamRLP(evm_common::RLPStream& rlp_stream) const;
        void decodeRLP(evm_common::RLP const& _r, std::error_code& ec);

    private:
        bool check_poly_block_validity() const;
        void make_receipts_root_hash();
        void make_txs_root_hash();
        void make_block_hash();

    private:
        uint8_t m_version { 0 }; // version of block
        xrelay_block_header m_header; // this part used by relayer or light clients;
        evm_common::h256 m_block_hash { 0 }; // hash of this block
        xrelay_signature_group_t m_signatures_groups; // vector of this blcok's signatures
        std::vector<xeth_receipt_t> m_receipts; // vector of receipts in this block
        std::vector<xeth_transaction_t> m_transactions; // vector of transaction in this block
        xrelay_extend_data m_exteend_data;
    };

} // namespace data
} // namespace top
