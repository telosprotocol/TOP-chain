// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "xevm_common/fixed_hash.h"
#include "xethreceipt.h"
#include "xethtransaction.h"
#include "xevm_common/rlp.h"
//#include "xdata/xpartial_merkle_tree.h"

namespace top {
namespace data {
    /*  use relay_block
       A.create genesis block or elections block
       1.create xrelay_block
         xrelay_block(uint64_t block_version, evm_common::h256  prev_hash,uint64_t block_height, uint64_t epochID, uint64_t timestamp);
       2.add elections info vector
         set_elections_next(std::vector<xrelay_election_node_t> &elections);
       3.calc hash in block  
          build_finish（）
       4.get block hash to signaure
            get_block_hash() 
       5. save block to map for merkle tree
           save_block_trie()
        
       A.create tx block 
       1.create xrelay_block
         xrelay_block(uint64_t block_version, evm_common::h256  prev_hash, uint64_t block_height, uint64_t epochID, uint64_t timestamp);
       2.set_transaction and set_receipts
       3.calc hash in block  
          build_finish（）
       4.get block hash to signaure
            get_block_hash() 
       5. save block to map for merkle tree
           save_block_trie()
    */

    struct xrelay_election_node_t {
        xrelay_election_node_t() = default;
        xrelay_election_node_t(const evm_common::h256 & pubkey_x, const evm_common::h256 & pubkey_y, uint64_t stk) : stake(stk) ,public_key_x(pubkey_x), public_key_y(pubkey_y) {}
       
        void    streamRLP(evm_common::RLPStream &_s) const; 
        bool    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);
        std::string get_pubkey_str() const;

        uint64_t                stake{0};        //election stake
        evm_common::h256        public_key_x;    //election key 
        evm_common::h256        public_key_y;
    };

    struct xrelay_election_group_t {
        xrelay_election_group_t() = default;
        void    streamRLP(evm_common::RLPStream &_s) const; 
        bool    decodeRLP(evm_common::RLP const& _r, std::error_code & ec);
        size_t  size() const { return elections_vector.size();}
        void    dump() const;
        bool    empty() const { return elections_vector.empty();}
        
        uint64_t                            election_epochID{0};   //epoch id for these elections
        std::vector<xrelay_election_node_t> elections_vector;   

    };

    struct xrelay_signature_t {
        xrelay_signature_t() = default;
        xrelay_signature_t(const std::string & sign_str);
        void streamRLP(evm_common::RLPStream &_s) const;
        bool decodeRLP(evm_common::RLP const& _r, std::error_code & ec);
        std::string to_string() const;

        evm_common::h256    r;
        evm_common::h256    s;
        evm_common::byte    v;
    };

    struct xrelay_signature_node_t {
        xrelay_signature_node_t() = default;
        xrelay_signature_node_t(const std::string & sign_str);
        void streamRLP(evm_common::RLPStream &_s) const;
        bool decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

        bool                exist{false};          //check signature if exist
        xrelay_signature_t  signature;
    };

    class  xrelay_block_inner_header{

    public:
        xrelay_block_inner_header() = default;
        xbytes_t                          encodeBytes() const;
        bool                              decodeBytes(xbytes_t const& _d, std::error_code & ec);

        void                              make_inner_hash();
        const evm_common::h256           &get_inner_header_hash() const { return m_inner_hash;}
        const evm_common::h256           &get_txs_root_hash() const { return m_txs_merkle_root;}
        const evm_common::h256           &get_receipts_root_hash() const { return m_receipts_merkle_root;}
        const evm_common::h256           &get_block_root_hash() const { return m_block_merkle_root;}
        const uint64_t                   &get_block_height() const { return m_height;}
        const uint64_t                   &get_timestamp() const { return m_timestamp;}
        const uint64_t                   &get_epochID() const { return m_epochID;}

        void                              set_txs_merkle_root_hash(evm_common::h256 hash);
        void                              set_receipts_merkle_root_hash(evm_common::h256 hash);
        void                              set_block_merkle_root_hash(evm_common::h256 hash);
        void                              set_block_height(uint64_t height);
        void                              set_epochid(uint64_t id);
        void                              set_timestamp(uint64_t timestamp);
        void                              set_version(uint8_t version);

        
    protected:
        void                             streamRLP(evm_common::RLPStream &_s) const;
        bool                             decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

    private:
        evm_common::h256    m_inner_hash;              //computable hash(inner_header data, no rlp )
        uint8_t             m_version{0};
        uint64_t            m_height{0};               //block height
        uint64_t            m_epochID{0};              // Epoch id of this block's epoch. Used for retrieving validator information
        uint64_t            m_timestamp{0};            //Timestamp at which the block was built.
        evm_common::h256    m_txs_merkle_root;         // root hash of the  transactions in the given block. now is null
        evm_common::h256    m_receipts_merkle_root;    // root hash of the  receipts in the given block.
        evm_common::h256    m_block_merkle_root;       // Merkle root of block hashes up to the current block.
        static const unsigned inner_fileds = 6;        
    };

    class xrelay_block_header {
    friend class xrelay_block;
    public:
        xrelay_block_header() = default;
        static const size_t block_header_fileds = 5;

        xbytes_t                        encodeBytes(bool withSignature = false) const;
        bool                            decodeBytes(xbytes_t const& _d, std::error_code & ec,  bool withSignature = false);
        xbytes_t                        streamRLP_header_to_contract();

    public:

       void                             set_txs_root_hash(evm_common::h256 hash);
       void                             set_receipts_root_hash(evm_common::h256 hash);

       void                             set_block_root_hash(evm_common::h256 hash);
       void                             set_inner_header(xrelay_block_inner_header &inner_hedaer);
       void                             set_prev_hash(evm_common::h256 hash);
       void                             set_block_hash(evm_common::h256 hash);
       void                             set_block_height(uint64_t height);
       void                             set_epochid(uint64_t id);
       void                             set_timestamp(uint64_t timestamp);
       void                             set_elections_next(const xrelay_election_group_t &elections);
       void                             add_signature_nodes(std::vector<xrelay_signature_node_t> signature_nodes);

       const xrelay_block_inner_header  &get_inner_header() const {return m_inner_header; }
       const evm_common::h256           &get_inner_header_hash() const {return m_inner_header.get_inner_header_hash();}
       const evm_common::h256           &get_txs_root_hash() const { return m_inner_header.get_txs_root_hash();}
       const evm_common::h256           &get_receipts_root_hash() const { return m_inner_header.get_receipts_root_hash();}
       const evm_common::h256           &get_block_root_hash() const { return m_inner_header.get_block_root_hash();}
       const uint64_t                   &get_block_height() const { return m_inner_header.get_block_height();}
       const uint64_t                   &get_epochid() const {return m_inner_header.get_epochID();}
       const uint64_t                   &get_timestamp() const { return m_inner_header.get_timestamp();}
       const evm_common::h256           &get_prev_block_hash() const { return m_prev_hash;}
       const evm_common::h256           &get_block_hash() const { return m_block_hash;}
       const uint8_t                    &get_header_version() {return m_version;}

       const xrelay_election_group_t               &get_elections_sets() const { return m_next_elections_groups;}
       const std::vector<xrelay_signature_node_t>  &get_signatures_sets() const { return m_block_signatures_nodes;}
       void                                         make_inner_hash();

    protected:
       void                             streamRLP(evm_common::RLPStream &rlp_stream,bool withSignature) const;
       bool                             decodeRLP(evm_common::RLP const& _r,  std::error_code & ec, bool withSignature) ;

    private:
       
        uint8_t                                 m_version{0};
        evm_common::h256                        m_block_hash{0};             //hash of this block
        xrelay_block_inner_header               m_inner_header;             //innder header that sent to relayer
        evm_common::h256                        m_prev_hash{0};              //hash ot prev block
        xrelay_election_group_t                 m_next_elections_groups;       //vector of elections info 
        std::vector<xrelay_signature_node_t>    m_block_signatures_nodes;   //vector of this blcok's signatures
        
    };

    class xrelay_block {

    public:
        static const unsigned block_fileds = 3;

        xrelay_block() {}
        xrelay_block(evm_common::h256  prev_hash, uint64_t block_height, uint64_t epochID, uint64_t timestamp);
        ~xrelay_block() {}
    
    public:

        /**
         * @brief 
         * 
         * @param withSignature fasle: data include signature info, 1: without signature info
         * @return xbytes_t 
         */
        xbytes_t                            encodeBytes(bool withSignature = false) const; 
        bool                                decodeBytes(xbytes_t const& _d, std::error_code & ec,  bool withSignature = false);
        xbytes_t                            streamRLP_header_to_contract();

        void                                set_elections_next(const xrelay_election_group_t &elections);
        void                                set_transactions(const std::vector<xeth_transaction_t> &transactions);
        void                                set_receipts(const std::vector<xeth_receipt_t> &receipts);
        void                                set_header(xrelay_block_header &block_hedaer);   
        void                                add_signature_nodes(std::vector<xrelay_signature_node_t> signature_nodes);
        bool                                build_finish();
        void                                save_block_trie();

    public:
        uint8_t                             &get_block_version() {return m_version;}
        xrelay_block_header                 &get_header() { return m_header;}
        const evm_common::h256              &get_block_hash() { return m_header.get_block_hash();}
        const xrelay_block_inner_header     &get_inner_header() const {return m_header.get_inner_header(); }
        const evm_common::h256              &get_inner_header_hash() const {return m_header.get_inner_header_hash();}
        const evm_common::h256              &get_txs_root_hash() const { return m_header.get_txs_root_hash();}
        const evm_common::h256              &get_receipts_root_hash() const { return m_header.get_receipts_root_hash();}
        const evm_common::h256              &get_block_root_hash() const { return m_header.get_block_root_hash();}
        const uint64_t                      &get_block_height() const { return m_header.get_block_height();}
        const uint64_t                      &get_timestamp() const { return m_header.get_timestamp();}
        const std::vector<xeth_receipt_t>   &get_all_receipts() const { return m_receipts;}
        const std::vector<xeth_transaction_t>   &get_all_transactions() { return m_transactions ;}
        std::string                         dump() const;

    protected:
        void                                streamRLP(evm_common::RLPStream &rlp_stream,bool withSignature = false) const;
        bool                                decodeRLP(evm_common::RLP const& _r, std::error_code & ec, bool withSignature = false);

    private:
        void                                make_receipts_root_hash();
        void                                make_txs_root_hash();
        void                                make_block_root_hash();
        void                                make_block_hash();

    private:
       
        uint8_t                                       m_version{0};     //version of block
        xrelay_block_header                           m_header;         //this part used by relayer or light clients;
        std::vector<xeth_receipt_t>                   m_receipts;       // vector of receipts in this block
        std::vector<xeth_transaction_t>               m_transactions;   // vector of transaction in this block
       // xPartialMerkleTree                           m_block_merkle_tree;

    };

    evm_common::h256 combine_hash(evm_common::h256 hash1, evm_common::h256 hash2);
} // namespace data
} // namespace top
