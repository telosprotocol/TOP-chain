// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xdata/xrelay_block.h"
#include "base/log.h"
#include "xevm_common/xtriehash.h"

#include "xdata/xrelay_block_store.h"

NS_BEG2(top, data)

using namespace top::evm_common;

top::evm_common::h256 combine_hash(top::evm_common::h256 hash1, top::evm_common::h256 hash2)
{
    top::evm_common::h256 hash_result { 0 };
    top::evm_common::h512 hash_512 { 0 };
    utl::xsha2_256_t hash_calc;
    std::vector<uint8_t> hash_vector;

    for (int i = 0; i < 32; i++) {
        hash_512[i] = hash1[i];
        hash_512[i + 32] = hash2[i];
    }
    hash_calc.update(hash_512.data(), 64);
    hash_calc.get_hash(hash_vector);
    top::evm_common::bytesConstRef((const unsigned char*)hash_vector.data(), 32).copyTo(hash_result.ref());
    return hash_result;
}



void xrelay_block_header::make_inner_hash()
{
    RLPStream rlpdata;
    m_inner_header.streamRLP(rlpdata, 1);
    m_inner_header.m_inner_hash = sha3(rlpdata.out());
}


void    xrelay_block_header::make_elections_root_hash()
{
    if (m_next_elections.size() > 0) {
        xPartialMerkleTree  merkleTree;
        auto &elections = m_next_elections;
        for (auto &_election: elections) {
            merkleTree.insert(_election.public_key);
        }
        m_inner_header.m_elections_hash = merkleTree.get_root();
    }
}

void  xrelay_block_header::set_txs_root_hash(evm_common::h256 hash)
{
    m_inner_header.m_txs_merkle_root = hash;
}

void xrelay_block_header::set_state_root_hash(evm_common::h256 hash)
{
    m_inner_header.m_state_merkle_root = hash;
}


void xrelay_block_header::set_receipts_root_hash(evm_common::h256 hash)
{
    m_inner_header.m_receipts_merkle_root = hash;
}

void xrelay_block_header::set_block_root_hash(evm_common::h256 hash)
{
    m_inner_header.m_block_merkle_root = hash;
}

void xrelay_block_header::streamRLP(RLPStream &rlp_stream)
{
    rlp_stream.appendList(block_header_fileds);
    m_inner_header.streamRLP(rlp_stream);
    rlp_stream << m_prev_hash << m_block_hash << m_chain_bits << m_table_height;
    
    rlp_stream.appendList(m_next_elections.size());
    for(auto & election : m_next_elections) {
        rlp_stream.appendList(2);
        rlp_stream << election.public_key << election.stake;
    }
}

///////////////////xrelay_block start /////////
xrelay_block::xrelay_block(uint64_t block_version, evm_common::h256  prev_hash, evm_common::u256  chain_bits, 
                    uint64_t table_height, uint64_t block_height, uint64_t epochID, uint64_t timestamp)
{
    m_header.m_prev_hash                    = prev_hash;
    m_header.m_table_height                 = table_height;
    m_header.m_chain_bits                   = chain_bits;
    m_header.m_table_height                 = table_height;
    m_header.m_inner_header.m_version       = block_version;
    m_header.m_inner_header.m_height        = block_height;
    m_header.m_inner_header.m_epochID       = epochID;
    m_header.m_inner_header.m_timestamp     = timestamp;
}


void xrelay_block::set_elections_next(std::vector<xrelay_election> &elections)
{   
    for (auto &election : elections) {
        m_header.m_next_elections.emplace_back(election);
    }
}

void xrelay_block::set_transaction(const std::vector<top::data::xtransaction_v3_ptr_t> &transactions)
{
    for (auto & tx: transactions) {
        m_transactions.emplace_back(tx);
    }
}

void xrelay_block::set_receipts(const std::vector<xrelay_receipt> &receipts)
{
    for (auto & receipt: receipts) {
        m_receipts.emplace_back(receipt);
    }
}

void xrelay_block::add_signature(xrelay_signature signature)
{
   for (auto &iter : m_header.m_block_signatures) {
        if (iter.r == signature.r && iter.s == signature.s && iter.v == signature.v ) {
           xwarn("xrelay_block::add_signature signature[%s][%s]is exist.", signature.r.hex().c_str(), signature.s.hex().c_str());
           return;
        }
    }
    m_header.m_block_signatures.emplace_back(signature);
}


void    xrelay_block::make_txs_root_hash()
{
    //todo, now is empty
    m_header.set_txs_root_hash(h256{0});
}


void    xrelay_block::make_state_root_hash()
{
    //todo, now is empty
    m_header.set_state_root_hash(h256{0});
}

void xrelay_block::make_receipts_root_hash()
{
    std::vector<bytes> rlp_receipts;
    for (auto & receipt: m_receipts) {
        RLPStream _s;
        receipt.streamRLP(_s);
        rlp_receipts.push_back(_s.out());
    }
    h256 receiptsRoot = orderedTrieRoot(rlp_receipts);
    xinfo("receipts root hash[%s]", receiptsRoot.hex().c_str());
    m_header.set_receipts_root_hash(receiptsRoot);
}

void   xrelay_block::streamRLP(evm_common::RLPStream &rlp_stream)
{
    rlp_stream.appendList(block_fileds);
    rlp_stream.appendList(m_receipts.size());
    for (auto &receipt: m_receipts) {
        receipt.streamRLP(rlp_stream);
    }
    rlp_stream.appendList(m_transactions.size());
    for (auto &tx :m_transactions) {
        //todo
    }

}


void xrelay_block::serialize_data_from_rlp(const evm_common::bytes &data)
{


}

void    xrelay_block::make_block_hash()
{
   evm_common::RLPStream _s(5); 
    _s << m_header.get_inner_header_hash() <<  m_header.m_prev_hash <<  m_header.m_chain_bits 
       << m_header.m_table_height;
    
    m_header.m_block_hash = sha3(_s.out());
}

void xrelay_block::make_block_root_hash()
{

    xdbg_info("xrelay_block::make_block_root_hash last block hash[%s]", m_header.m_prev_hash.hex().c_str());
    m_block_merkle_tree = xrelay_block_store::get_instance().get_block_merkle_tree(m_header.m_prev_hash);
    m_block_merkle_tree.insert(m_header.m_prev_hash);
    m_header.set_block_root_hash(m_block_merkle_tree.get_root());
}


bool    xrelay_block::build_finish()
{
    //todo, add checklist later on

    //calc hash 
    m_header.make_elections_root_hash();
    make_txs_root_hash();
    make_receipts_root_hash();
    make_state_root_hash();
    m_header.make_block_root_hash();
    m_header.make_inner_hash();
    make_block_hash();

    return true;
}


void  xrelay_block::save_block_trie()
{
    xrelay_block_store::get_instance().save_block_merkle_tree( m_header.m_block_hash, m_block_merkle_tree);
    xrelay_block_store::get_instance().save_block_ordinal_block_hash(m_block_merkle_tree.size(),  m_header.m_block_hash);
}



NS_END2