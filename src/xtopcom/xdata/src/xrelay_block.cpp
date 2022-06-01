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

void xrelay_receipt_log::streamRLP(evm_common::RLPStream &_s) {
    _s.appendList(3)  << m_contract_address << m_topics << m_data;
}

bool xrelay_receipt_log::decodeRLP(evm_common::RLP const& _r) {
    assert(_r.itemCount() == 3);
    //todo check error
    m_contract_address = _r[0].toHash<evm_common::h160>();
    m_topics           = _r[1].toVector<evm_common::h256>();
    m_data             = _r[2].toBytes();
    return true;
}

void xrelay_receipt::streamRLP(evm_common::RLPStream &_s) {
    _s.appendList(4);
    _s << m_status;
    _s << m_gasUsed << m_logsBloom;
    _s.appendList(m_logs.size());
    for (auto &log : m_logs) {
        log.streamRLP(_s);
    }
}

bool xrelay_receipt::decodeRLP(evm_common::RLP const& _r) {
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

void xrelay_election::streamRLP(evm_common::RLPStream &_s) {
    _s.appendList(3)  << public_key_x << public_key_y << stake;
}

bool xrelay_election::decodeRLP(evm_common::RLP const& _r) {
    assert(_r.itemCount() == 3);
    //todo check error
    public_key_x = _r[0].toHash<evm_common::h256>();  
    public_key_y = _r[1].toHash<evm_common::h256>();
    stake        = _r[2].toInt<uint64_t>();
    return true;
}

void xrelay_signature::streamRLP(evm_common::RLPStream &_s) {
    _s.appendList(3)  << r << s << v;
}

bool xrelay_signature::decodeRLP(evm_common::RLP const& _r) {
    assert(_r.itemCount() == 3);
    //todo check error
    r = _r[0].toHash<evm_common::h256>();
    s = _r[1].toHash<evm_common::h256>();
    v = _r[2].toInt<uint8_t>();
    return true;
}

void xrelay_block_inner_header::streamRLP(evm_common::RLPStream &_s) const {
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

bool xrelay_block_inner_header::decodeRLP(evm_common::RLP const& _r) {
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

void xrelay_block_header::make_inner_hash()
{
    RLPStream rlpdata;
    m_inner_header.streamRLP(rlpdata);
    m_inner_header.m_inner_hash = sha3(rlpdata.out());
}


void    xrelay_block_header::make_elections_root_hash()
{
    std::vector<bytes> rlp_elections;
    if(m_next_elections.size() > 0) {
        for (auto &_election: m_next_elections) {
            RLPStream _s;
            _election.streamRLP(_s);
            rlp_elections.push_back(_s.out());
        }
    }
    h256 electionsRoot = orderedTrieRoot(rlp_elections);
    m_inner_header.m_elections_hash = electionsRoot;
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


void xrelay_block_header::streamRLP(evm_common::RLPStream &rlp_stream, bool withSignature)
{
    //todo check error
    size_t decLen = block_header_fileds;
    if(!withSignature) {
        decLen--;
        std::cout << " xrelay_block_header::withoutignature " << decLen << std::endl;
    }

    rlp_stream.appendList(decLen);
    m_inner_header.streamRLP(rlp_stream);
    rlp_stream << m_prev_hash ;
    rlp_stream << m_block_hash;
    rlp_stream << m_chain_bits;
    rlp_stream << m_table_height;

    rlp_stream.appendList(m_next_elections.size());
    for(auto & election : m_next_elections) {
        election.streamRLP(rlp_stream);
    }

    if(withSignature) {
        rlp_stream.appendList(m_block_signatures.size());
        for(auto & signature : m_block_signatures) {
            signature.streamRLP(rlp_stream);
        }
    }
}


bool xrelay_block_header::decodeRLP(evm_common::RLP const& _r, bool blocksignature)
{
  //  assert(_r.itemCount() == block_header_fileds);
    //todo check error
    evm_common::RLP const& rlp_innder = RLP(_r[0].data());
    m_inner_header.decodeRLP(rlp_innder);

    size_t idx = 1;
    m_prev_hash     = _r[idx].toHash<evm_common::h256>(); idx++;
    m_block_hash    = _r[idx].toHash<evm_common::h256>(); idx++;
    m_chain_bits    = _r[idx].toInt<evm_common::u256>(); idx++;
    m_table_height  = _r[idx].toInt<uint64_t>(); idx++;

    evm_common::RLP const& rlp_electionsList = RLP(_r[idx].data()); idx++;
    unsigned itemCount = rlp_electionsList.itemCount();
    for(unsigned i = 0; i < itemCount; i++ ) {
        evm_common::RLP const& rlp_election = RLP(rlp_electionsList[i].data());
        xrelay_election election;
        election.decodeRLP(rlp_election);
        m_next_elections.emplace_back(election);
    }

    if(blocksignature) {
        evm_common::RLP const& rlp_signaturesList = RLP(_r[idx].data()); idx++;
        itemCount = rlp_signaturesList.itemCount();
        for(unsigned i = 0; i < itemCount; i++ ) {
            evm_common::RLP const& rlp_signature = RLP(rlp_signaturesList[i].data());
            xrelay_signature block_signature;
            block_signature.decodeRLP(rlp_signature);
            m_block_signatures.emplace_back(block_signature);
        }
    }

    return true;
}

void xrelay_block_header::streamRLP_header_to_contract(evm_common::RLPStream &rlp_stream)
{
     //no rlp block_hash
    size_t decLen = block_header_fileds -1;

    rlp_stream.appendList(decLen);
    m_inner_header.streamRLP(rlp_stream);
    rlp_stream << m_prev_hash ;
    rlp_stream << m_chain_bits;
    rlp_stream << m_table_height;

    rlp_stream.appendList(m_next_elections.size());
    for(auto & election : m_next_elections) {
        election.streamRLP(rlp_stream);
    }

    rlp_stream.appendList(m_block_signatures.size());
    for(auto & signature : m_block_signatures) {
        signature.streamRLP(rlp_stream);
    }

}



void xrelay_block_header::set_inner_header(xrelay_block_inner_header &inner_hedaer)
{
   m_inner_header = inner_hedaer;
}

void  xrelay_block_header::set_prev_hash(evm_common::h256 hash)
{
    m_prev_hash = hash;
}

void  xrelay_block_header::set_block_hash(evm_common::h256 hash)
{
    m_block_hash = hash;
}

void  xrelay_block_header::set_chain_bits(evm_common::u256 chain_bits)
{
    m_chain_bits = chain_bits;
}

void  xrelay_block_header::set_table_height(uint64_t table_height)
{
    m_table_height = table_height;
}

void xrelay_block_header::set_elections_next(std::vector<xrelay_election> &elections)
{   
    
    for (auto &election : elections) {
        m_next_elections.emplace_back(election);
    }
}

void xrelay_block_header::add_signature(xrelay_signature signature)
{
   for (auto &iter : m_block_signatures) {
        if (iter.r == signature.r && iter.s == signature.s && iter.v == signature.v ) {
           xwarn("xrelay_block::add_signature signature[%s][%s]is exist.", signature.r.hex().c_str(), signature.s.hex().c_str());
           return;
        }
    }
    m_block_signatures.emplace_back(signature);
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

    m_header.set_elections_next(elections);
   
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

    m_header.add_signature(signature);
}


void xrelay_block::make_txs_root_hash()
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

void   xrelay_block::streamRLP(evm_common::RLPStream &rlp_stream,bool withSignature)
{
    rlp_stream.appendList(block_fileds);
    m_header.streamRLP(rlp_stream, withSignature);
    rlp_stream.appendList(m_receipts.size());
    for (auto &receipt: m_receipts) {
        receipt.streamRLP(rlp_stream);
    }
   
   /*
    //todo
    rlp_stream.appendList(m_transactions.size());
    for (auto &tx :m_transactions) {
        
    }*/
}

bool xrelay_block::decodeRLP(evm_common::RLP const& _r, bool withSignature)
{
    evm_common::RLP const& rlp_header = RLP(_r[0].data());
    m_header.decodeRLP(rlp_header, withSignature);

    evm_common::RLP const& rlp_receiptsList = RLP(_r[1].data());
    unsigned itemCount = rlp_receiptsList.itemCount();
    for (unsigned i = 0; i < itemCount; i++) {
        evm_common::RLP const& rlp_receipt = RLP(rlp_receiptsList[i].data());
        xrelay_receipt receipt;
        receipt.decodeRLP(rlp_receipt);
        m_receipts.emplace_back(receipt);
    }
    return true;
}

void    xrelay_block::make_block_hash()
{
   evm_common::RLPStream _s(4); 
    _s << m_header.get_inner_header_hash() <<  m_header.m_prev_hash <<  m_header.m_chain_bits 
       << m_header.m_table_height;
    
    m_header.m_block_hash = sha3(_s.out());
}

void xrelay_block::make_block_root_hash()
{

    xdbg_info("xrelay_block::make_block_root_hash last block hash[%s]", m_header.m_prev_hash.hex().c_str());
   // m_block_merkle_tree = xrelay_block_store::get_instance().get_block_merkle_tree(m_header.m_prev_hash);
   // m_block_merkle_tree.insert(m_header.m_prev_hash);
    h256 block_root_hash{0};
    m_header.set_block_root_hash(block_root_hash);
}


bool    xrelay_block::build_finish()
{
    //todo, add checklist later on

    //calc hash 
    m_header.make_elections_root_hash();
    make_txs_root_hash();
    make_receipts_root_hash();
    make_state_root_hash();
    make_block_root_hash();
    m_header.make_inner_hash();
    make_block_hash();

    return true;
}



void xrelay_block::set_header(xrelay_block_header &block_hedaer)
{
    m_header = block_hedaer;
}


void  xrelay_block::save_block_trie()
{
   // xrelay_block_store::get_instance().save_block_merkle_tree( m_header.m_block_hash, m_block_merkle_tree);
  //  xrelay_block_store::get_instance().save_block_ordinal_block_hash(m_block_merkle_tree.size(),  m_header.m_block_hash);
}

void xrelay_block::streamRLP_header_to_contract(evm_common::RLPStream &rlp_stream)
{
    m_header.streamRLP_header_to_contract(rlp_stream);
}


xrelay_signature::xrelay_signature(const std::string & sign_str) {
    xassert(sign_str.size() == 65);
    uint8_t compact_signature[65];
    memcpy(compact_signature, sign_str.data(), sign_str.size());
    v = compact_signature[0];
    top::evm_common::bytes r_bytes(compact_signature + 1, compact_signature + 33);
    r = top::evm_common::fromBigEndian<top::evm_common::u256>(r_bytes);
    top::evm_common::bytes s_bytes(compact_signature + 33, compact_signature + 65);
    s = top::evm_common::fromBigEndian<top::evm_common::u256>(s_bytes);
}



NS_END2