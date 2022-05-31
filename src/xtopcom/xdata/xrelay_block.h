// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

//#include "xrelay_block_header.hpp"

#include "xevm_common/fixed_hash.h"
#include "xtransaction_v3.h"
#include "xevm_common/rlp.h"
//#include "xdata/xpartial_merkle_tree.h"

namespace top {
namespace data {
    /*  use relay_block
       A.create genesis block or elections block
       1.create xrelay_block
         xrelay_block(uint64_t block_version, evm_common::h256  prev_hash, evm_common::u256  chain_bits, 
                uint64_t table_height, uint64_t block_height, uint64_t epochID, uint64_t timestamp);
       2.add elections info vector
         set_elections_next(std::vector<xrelay_election> &elections);
       3.calc hash in block  
          build_finish（）
       4.get block hash to signaure
            get_block_hash() 
       5. save block to map for merkle tree
           save_block_trie()
        
       A.create tx block 
       1.create xrelay_block
         xrelay_block(uint64_t block_version, evm_common::h256  prev_hash, evm_common::u256  chain_bits, 
                uint64_t table_height, uint64_t block_height, uint64_t epochID, uint64_t timestamp);
       2.set_transaction and set_receipts
       3.calc hash in block  
          build_finish（）
       4.get block hash to signaure
            get_block_hash() 
       5. save block to map for merkle tree
           save_block_trie()
    */
    struct xrelay_receipt_log {
        //address of the contract that transaction execution
        evm_common::h160                m_contract_address;
        // supplied by the contract, usually ABI-encoded
        evm_common::bytes               m_data;
        // list of topics provided by the contract
        evm_common::h256s               m_topics;
        void streamRLP(evm_common::RLPStream &_s) {
            _s.appendList(3)  << m_contract_address << m_topics << m_data;
        }
        bool decodeRLP(evm_common::RLP const& _r) {
            assert(_r.itemCount() == 3);
            //todo check error

            m_contract_address = _r[0].toHash<evm_common::h160>();
            m_topics           = _r[1].toVector<evm_common::h256>();
            m_data             = _r[2].toBytes();
            return true;
        }
    };

    struct  xrelay_receipt {
        xrelay_receipt(){}
        xrelay_receipt(uint8_t type, uint8_t status, const evm_common::u256 & gas_used, const evm_common::h2048 & logs_bloom, std::vector<xrelay_receipt_log> logs)
          : m_type(type), m_status(status), m_gasUsed(gas_used), m_logsBloom(logs_bloom), m_logs(logs) {
        }
        // Transaction types. LegacyTxType:0 , AccessListTxType:1 , DynamicFeeTxType:2
        uint8_t                         m_type;
        //status code,check transactions status
        uint8_t                         m_status;
        //gas of block used
        evm_common::u256                m_gasUsed;
        // bloom filter for the logs of the block
        evm_common::h2048               m_logsBloom;
        //receipt log
        std::vector<xrelay_receipt_log> m_logs;
        //rlp result
        void streamRLP(evm_common::RLPStream &_s) {
            _s.appendList(4);
            _s << m_status;
            _s << m_gasUsed << m_logsBloom;
            _s.appendList(m_logs.size());
            for (auto &log : m_logs) {
                log.streamRLP(_s);
            }
        }
        bool decodeRLP(evm_common::RLP const& _r) {
            assert(_r.itemCount() == 4);
            //todo check error
            m_status    = _r[0].toInt<uint64_t>();
            m_gasUsed   = _r[1].toInt<evm_common::u256>();
            m_logsBloom = _r[2].toHash<evm_common::h2048>();
            
            evm_common::RLP const& rlp_logsList = evm_common::RLP(_r[3].data());
            unsigned itemCount = rlp_logsList.itemCount();
            for (unsigned i = 0; i < itemCount; i++) {
                evm_common::RLP const& rlp_log = evm_common::RLP(rlp_logsList[i].data());
                xrelay_receipt_log log;
                log.decodeRLP(rlp_log);
                m_logs.emplace_back(log);
            }
            return true;
        }
    };

    struct xrelay_election {
        //election key 
        evm_common::h256        public_key;
         //election stake
        uint64_t                stake;

        void streamRLP(evm_common::RLPStream &_s) {
            _s.appendList(2)  << public_key << stake;
        }
        bool decodeRLP(evm_common::RLP const& _r) {
            assert(_r.itemCount() == 2);
            //todo check error
            public_key = _r[0].toHash<evm_common::h256>();
            stake      = _r[1].toInt<uint64_t>();
            return true;
        }
    };

    struct xrelay_signature {
        xrelay_signature() {}
        xrelay_signature(const std::string & sign_str);
        evm_common::h256    r;
        evm_common::h256    s;
        evm_common::byte    v;
        void streamRLP(evm_common::RLPStream &_s) {
            _s.appendList(3)  << r << s << v;
        }
        bool decodeRLP(evm_common::RLP const& _r) {
            assert(_r.itemCount() == 3);
            //todo check error
            r = _r[0].toHash<evm_common::h256>();
            s = _r[1].toHash<evm_common::h256>();
            v = _r[2].toInt<uint8_t>();
            return true;
        }
    };

    struct xrelay_block_inner_header{

        //computable hash(inner_header data, no rlp )
        evm_common::h256    m_inner_hash; 
        //version of header
        uint64_t            m_version;
        //block height
        uint64_t            m_height;
        // Epoch id of this block's epoch. Used for retrieving validator information
        uint64_t            m_epochID;
        //Timestamp at which the block was built.
        uint64_t            m_timestamp;
        //Hash of the next epoch block producers set
        evm_common::h256    m_elections_hash;
        // root hash of the  transactions in the given block. now is null
        evm_common::h256    m_txs_merkle_root;
        // root hash of the  receipts in the given block.
        evm_common::h256    m_receipts_merkle_root;
        // root hash of the  chain state . now is null
        evm_common::h256    m_state_merkle_root;
        // Merkle root of block hashes up to the current block.
        evm_common::h256    m_block_merkle_root;

        static const unsigned inner_fileds = 9;
        /**
         * @brief 
         * 
         * @param innerFlag  false:rlp all data, true: rlp data to create inner hash
         * @return evm_common::bytes 
         */
        void streamRLP(evm_common::RLPStream &_s) const {
            _s.appendList(inner_fileds);
            _s << m_version;
            _s << m_height;
            _s << m_epochID;
            _s << m_timestamp;
            _s << m_elections_hash;
            _s << m_txs_merkle_root;
            _s << m_receipts_merkle_root;
            _s << m_state_merkle_root;
            _s << m_block_merkle_root;
        }

        bool decodeRLP(evm_common::RLP const& _r) {
            assert(_r.itemCount() == inner_fileds);

            m_version               = _r[0].toInt<uint64_t>(); 
            m_height                = _r[1].toInt<uint64_t>(); 
            m_epochID               = _r[2].toInt<uint64_t>(); 
            m_timestamp             = _r[3].toInt<uint64_t>(); 
            m_elections_hash        = _r[4].toHash<evm_common::h256>();
            m_txs_merkle_root       = _r[5].toHash<evm_common::h256>(); 
            m_receipts_merkle_root  = _r[6].toHash<evm_common::h256>(); 
            m_state_merkle_root     = _r[7].toHash<evm_common::h256>();
            m_block_merkle_root     = _r[8].toHash<evm_common::h256>(); 
            return true;
        }

    };

    class xrelay_block_header {
    friend class xrelay_block;
    public:
       static const size_t block_header_fileds = 7;

       xrelay_block_header(){}

       void                             streamRLP(evm_common::RLPStream &rlp_stream,bool withSignature = false);
       void                             streamRLP_header_to_contract(evm_common::RLPStream &rlp_stream);
       bool                             decodeRLP(evm_common::RLP const& _r, bool withSignature = false);

    public:
       void                             make_elections_root_hash();
       void                             set_txs_root_hash(evm_common::h256 hash);
       void                             set_receipts_root_hash(evm_common::h256 hash);
       void                             set_state_root_hash(evm_common::h256 hash);
       void                             set_block_root_hash(evm_common::h256 hash);
       void                             set_inner_header(xrelay_block_inner_header &inner_hedaer);
       void                             set_prev_hash(evm_common::h256 hash);
       void                             set_block_hash(evm_common::h256 hash);
       void                             set_chain_bits(evm_common::u256 chain_bits);
       void                             set_table_height(uint64_t table_height);
       void                             set_elections_next(std::vector<xrelay_election> &elections);
       void                             add_signature(xrelay_signature signature);

       const xrelay_block_inner_header  &get_inner_header() const {return m_inner_header; }
       const evm_common::h256           &get_inner_header_hash() const {return m_inner_header.m_inner_hash;}
       const evm_common::h256           &get_elections_root_hash() const { return m_inner_header.m_elections_hash;}
       const evm_common::h256           &get_txs_root_hash() const { return m_inner_header.m_txs_merkle_root;}
       const evm_common::h256           &get_receipts_root_hash() const { return m_inner_header.m_receipts_merkle_root;}
       const evm_common::h256           &get_state_root_hash() const { return m_inner_header.m_state_merkle_root;}
       const evm_common::h256           &get_block_root_hash() const { return m_inner_header.m_block_merkle_root;}
       const evm_common::h256           &get_prev_block_hash() const { return m_prev_hash;}
       const evm_common::h256           &get_block_hash() const { return m_block_hash;}
       const evm_common::u256           &get_bchain_bits() const { return m_chain_bits;}
       const uint64_t                   &get_table_height() const { return m_table_height;}
       const std::vector<xrelay_election>  &get_elections_sets() const { return m_next_elections;}
       const std::vector<xrelay_signature>  &get_signatures_sets() const { return m_block_signatures;}

       void                              make_inner_hash();
    private:
        //innder header that sent to relayer
        xrelay_block_inner_header       m_inner_header;
        //hash ot prev block
        evm_common::h256                m_prev_hash;
        //hash of this block
        evm_common::h256                m_block_hash;
        //include all chainid that all transaction  in block 
        evm_common::u256                m_chain_bits;
        // height of table which produce this block
        uint64_t                        m_table_height;
        //vector of elections info 
        std::vector<xrelay_election>    m_next_elections;
        //vector of this blcok's signatures
        std::vector<xrelay_signature>   m_block_signatures;
        
    };

    class xrelay_block {

    public:
        static const unsigned block_fileds = 2;

        xrelay_block() {}
        xrelay_block(uint64_t block_version, evm_common::h256  prev_hash, evm_common::u256  chain_bits, 
                     uint64_t table_height, uint64_t block_height, uint64_t epochID, uint64_t timestamp);
        ~xrelay_block() {}
    
    public:
        /**
         * @brief  get rlp data
         * 
         * @param rlp_stream 
         * @param withSingaure 0:rlp data include signature info, 1: without signature info
         */
        void                                streamRLP(evm_common::RLPStream &rlp_stream,bool withSignature = false);
        bool                                decodeRLP(evm_common::RLP const& _r,bool withSignature = false);
        void                                set_elections_next(std::vector<xrelay_election> &elections);
        void                                set_transaction(const std::vector<top::data::xtransaction_v3_ptr_t> &transactions);
        void                                set_receipts(const std::vector<xrelay_receipt> &receipts);
        void                                set_header(xrelay_block_header &block_hedaer);   
        void                                add_signature(xrelay_signature signature);
        bool                                build_finish();
        void                                save_block_trie();

                   
    public:
       
        const xrelay_block_header           &get_header() const { return m_header;}
        const evm_common::h256              &get_block_hash() { return m_header.get_block_hash();}
        const xrelay_block_inner_header     &get_inner_header() const {return m_header.get_inner_header(); }
        const evm_common::h256              &get_inner_header_hash() const {return m_header.get_inner_header_hash();}
        const evm_common::h256              &get_elections_root_hash() const { return m_header.get_elections_root_hash();}
        const evm_common::h256              &get_txs_root_hash() const { return m_header.get_txs_root_hash();}
        const evm_common::h256              &get_receipts_root_hash() const { return m_header.get_receipts_root_hash();}
        const evm_common::h256              &get_state_root_hash() const { return m_header.get_state_root_hash();}
        const evm_common::h256              &get_block_root_hash() const { return m_header.get_block_root_hash();}
        const std::vector<xrelay_receipt>   &get_block_receipts_set() const { return m_receipts;}
    private:
        
        void                                make_receipts_root_hash();
        void                                make_txs_root_hash();
        void                                make_state_root_hash();
        void                                make_block_root_hash();
        void                                make_block_hash();

    private:
        
        //this part used by relayer or light clients;
        xrelay_block_header                           m_header;
        // vector of receipts in this block
        std::vector<xrelay_receipt>                   m_receipts;
        // vector of transaction in this block
        std::vector<top::data::xtransaction_v3_ptr_t> m_transactions;

       // evm_common::bytes                             m_block_bytes;
       // evm_common::bytes                             m_body_bytes;
       // xPartialMerkleTree                           m_block_merkle_tree;

    };

    evm_common::h256 combine_hash(evm_common::h256 hash1, evm_common::h256 hash2);
} // namespace data
} // namespace top
