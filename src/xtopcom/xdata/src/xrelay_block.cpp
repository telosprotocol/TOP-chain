// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xdata/xrelay_block.h"
#include "base/log.h"
#include "xevm_common/xtriehash.h"
#include "xdata/xrelay_block_store.h"
#include "xcommon/xerror/xerror.h"

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

void xrelay_election_node_t::streamRLP(evm_common::RLPStream &_s)  const {
    _s.appendList(3)  << public_key_x << public_key_y << stake;
}

bool xrelay_election_node_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if (!_r.isList() || _r.itemCount() != 3) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_election_node_t::decodeRLP fail item count,%d", _r.itemCount());
        return false;
    }

    public_key_x = _r[0].toHash<evm_common::h256>();  
    public_key_y = _r[1].toHash<evm_common::h256>();
    stake        = _r[2].toInt<uint64_t>();
    return true;
}

void xrelay_election_group_t::streamRLP(evm_common::RLPStream &_s)  const {
    if (elections_vector.size() > 0 ) {
        _s.appendList(2)  << election_epochID ;
        _s.appendList(elections_vector.size());
        for(auto & election : elections_vector) {
            election.streamRLP(_s);
        }
    } else {
        _s.appendList(0); 
    }
}

bool xrelay_election_group_t::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    
    if (!_r.isEmpty()) {
        if (!_r.isList() ||  _r.itemCount() != 2) {
            ec = common::error::xerrc_t::invalid_rlp_stream;
            xerror("xrelay_election_group_t::decodeRLP fail item count,%d", _r.itemCount());
            return false;
        }
        election_epochID = _r[0].toInt<uint64_t>();  
        evm_common::RLP const& electionList = RLP(_r[1].data()); 

        unsigned itemCount = electionList.itemCount();
        for(unsigned i = 0; i < itemCount; i++ ) {
            evm_common::RLP const& rlp_election = RLP(electionList[i].data());
            xrelay_election_node_t election;
            election.decodeRLP(rlp_election, ec);
            if (ec) {
                xerror("xrelay_election_group_t::decodeRLP  election fail.");
                return false;
            }
            elections_vector.emplace_back(election);
        }
    }

    return true;
}

xrelay_signature_t::xrelay_signature_t(const std::string & sign_str) {
    xassert(sign_str.size() == 65);
    uint8_t compact_signature[65];
    memcpy(compact_signature, sign_str.data(), sign_str.size());
    v = compact_signature[0];
    top::evm_common::bytes r_bytes(compact_signature + 1, compact_signature + 33);
    r = top::evm_common::fromBigEndian<top::evm_common::u256>(r_bytes);
    top::evm_common::bytes s_bytes(compact_signature + 33, compact_signature + 65);
    s = top::evm_common::fromBigEndian<top::evm_common::u256>(s_bytes);
    
}

void xrelay_signature_t::streamRLP(evm_common::RLPStream &_s) const {
    _s.appendList(3)  << r << s << v;
}

bool xrelay_signature_t::decodeRLP(evm_common::RLP const& _r,  std::error_code & ec) {
    if (!_r.isList() || _r.itemCount() != 3) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_election_group_t::decodeRLP fail item count,%d", _r.itemCount());
        return false;
    }

    r = _r[0].toHash<evm_common::h256>();
    s = _r[1].toHash<evm_common::h256>();
    v = _r[2].toInt<uint8_t>();
    return true;
}


xrelay_signature_node_t::xrelay_signature_node_t(const std::string & sign_str) {
    if (sign_str.length() != 65) {
        exist = false;
    } else {
        exist = true;
        signature = xrelay_signature_t(sign_str);
    }
}

void xrelay_signature_node_t::streamRLP(evm_common::RLPStream &_s) const {
    if (exist) {
        _s.appendList(2) << exist;
        signature.streamRLP(_s);
    } else {
        _s.appendList(1) << exist;
    }
}

bool xrelay_signature_node_t::decodeRLP(evm_common::RLP const& _r,  std::error_code & ec) {
    if (!_r.isList() || _r.itemCount() < 1) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_signature_node_t::decodeRLP fail item count,%d", _r.itemCount());
        return false;
    }
        
    exist = _r[0].toInt<bool>();
    if (exist) {
       signature.decodeRLP(RLP(_r[1].data()), ec);
    }
    
    return true;
}



xbytes_t xrelay_block_inner_header::encodeBytes() const {
    xbytes_t _bytes;
    _bytes.push_back(m_version);

    if (m_version == 0) {
        evm_common::RLPStream _s;
        streamRLP(_s);
        _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    }
    xinfo("xrelay_block_inner_header::encodeBytes %s.", toHex(_bytes).c_str());
    return _bytes;
}

void xrelay_block_inner_header::streamRLP(evm_common::RLPStream &_s) const {
    _s.appendList(inner_fileds);
    _s << m_height;
    _s << m_epochID;
    _s << m_timestamp;
    _s << m_txs_merkle_root;
    _s << m_receipts_merkle_root;
    _s << m_block_merkle_root;
}

bool xrelay_block_inner_header::decodeBytes(xbytes_t const& _d, std::error_code & ec) {

    if (_d.size() < 2) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block_inner_header::decodeBytes fail bytes,%zu", _d.size());
        return false;
    }

    m_version = (uint8_t)_d.front();
    if (m_version != 0) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block_inner_header::decodeBytes fail invalid version,%d", m_version);
        return false;
    }

    xbytes_t _d2(_d.begin() + 1, _d.end());
    RLP _r(_d2);
    return decodeRLP(_r, ec);
}

bool xrelay_block_inner_header::decodeRLP(evm_common::RLP const& _r, std::error_code & ec) {
    if (!_r.isList() || _r.itemCount() != inner_fileds) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block_inner_header::decodeRLP fail item count,%d", _r.itemCount());
        return false;
    }
    
    m_height                = _r[0].toInt<uint64_t>(); 
    m_epochID               = _r[1].toInt<uint64_t>(); 
    m_timestamp             = _r[2].toInt<uint64_t>(); 
    m_txs_merkle_root       = _r[3].toHash<evm_common::h256>(); 
    m_receipts_merkle_root  = _r[4].toHash<evm_common::h256>(); 
    m_block_merkle_root     = _r[5].toHash<evm_common::h256>(); 
    return true;
}

void xrelay_block_inner_header::make_inner_hash()
{
    xbytes_t bytes_data = encodeBytes();
    m_inner_hash = sha3(bytes_data);
    xinfo("xrelay_block_inner_header::make_inner_hash  data[%s]", toHex(bytes_data).c_str());
    xinfo("xrelay_block_inner_header::make_inner_hash  hash[%s]", m_inner_hash.hex().c_str());
}


void    xrelay_block_inner_header::set_txs_merkle_root_hash(evm_common::h256 hash)
{
    m_txs_merkle_root = hash;
}

void    xrelay_block_inner_header::set_receipts_merkle_root_hash(evm_common::h256 hash)
{
    m_receipts_merkle_root = hash;
}

void    xrelay_block_inner_header::set_block_merkle_root_hash(evm_common::h256 hash)
{
    m_block_merkle_root = hash;
}

void    xrelay_block_inner_header::set_block_height(uint64_t height)
{
    m_height = height;
}

void    xrelay_block_inner_header::set_epochid(uint64_t id)
{
    m_epochID = id;
}

void xrelay_block_inner_header::set_timestamp(uint64_t timestamp)
{
    m_timestamp = timestamp;
}

void xrelay_block_inner_header::set_version(uint8_t version)
{
    m_version = version;
}

///////////////////xrelay_block_header  start /////////


void  xrelay_block_header::set_txs_root_hash(evm_common::h256 hash)
{
    m_inner_header.set_txs_merkle_root_hash(hash);
}

void xrelay_block_header::set_receipts_root_hash(evm_common::h256 hash)
{
    m_inner_header.set_receipts_merkle_root_hash(hash);
}

void xrelay_block_header::set_block_root_hash(evm_common::h256 hash)
{
    m_inner_header.set_block_merkle_root_hash(hash);
}

xbytes_t xrelay_block_header::encodeBytes(bool withSignature) const {
    xbytes_t _bytes;
    _bytes.push_back(m_version);

    if (m_version == 0) {
        evm_common::RLPStream _s;
        streamRLP(_s, withSignature);
        _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    } else{
        xwarn("xrelay_block_header::encodeBytes block_version %d not support!", m_version);
    }
    return _bytes;
}

void xrelay_block_header::streamRLP(evm_common::RLPStream &rlp_stream, bool withSignature) const
{
    size_t decLen = block_header_fileds;
    if(!withSignature) {
        decLen--;
        xdbg("xrelay_block_header::withoutignature  decLen %d ", decLen);
    }

    rlp_stream.appendList(decLen);
    rlp_stream << m_inner_header.encodeBytes();
    rlp_stream << m_prev_hash;
    m_next_elections_groups.streamRLP(rlp_stream);
    if(withSignature) {
        rlp_stream.appendList(m_block_signatures_nodes.size());
        for(auto & signature : m_block_signatures_nodes) {
            signature.streamRLP(rlp_stream);
        }
    }
}

bool xrelay_block_header::decodeBytes(xbytes_t const& _d, std::error_code & ec, bool withSignature) {

    if (_d.size() < 2) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block_header::decodeBytes fail bytes,%zu", _d.size());
        return false;
    }

    m_version = (uint8_t)_d.front();
    if (m_version != 0) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block_header::decodeBytes fail invalid version,%d", m_version);
        return false;
    }

    xbytes_t _d2(_d.begin() + 1, _d.end());
    RLP _r(_d2);
    return decodeRLP(_r, ec, withSignature);
}

bool xrelay_block_header::decodeRLP(evm_common::RLP const& _r, std::error_code & ec, bool withSignature)
{
    if (!_r.isList() || _r.itemCount() < 3) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block_header::decodeRLP fail item count,%d", _r.itemCount());
        return false;
    }

    size_t idx = 0;
    m_inner_header.decodeBytes(_r[idx].toBytes(), ec);    idx++;
    m_prev_hash     = _r[idx].toHash<evm_common::h256>(); idx++;

    evm_common::RLP const& rlp_electionsList = RLP(_r[idx].data()); idx++;
    m_next_elections_groups.decodeRLP(rlp_electionsList, ec);

    if(withSignature) {
        evm_common::RLP const& rlp_signaturesList = RLP(_r[idx].data()); idx++;
        unsigned itemCount = rlp_signaturesList.itemCount();
        for(unsigned i = 0; i < itemCount; i++ ) {
            evm_common::RLP const& rlp_signature = RLP(rlp_signaturesList[i].data());
            xrelay_signature_node_t block_signature_node;
            block_signature_node.decodeRLP(rlp_signature, ec);
            m_block_signatures_nodes.emplace_back(block_signature_node);
        }
    }

    return true;
}

xbytes_t xrelay_block_header::streamRLP_header_to_contract()
{

    xbytes_t _bytes;
    _bytes.push_back(m_version);

    if (m_version == 0) {
        evm_common::RLPStream rlp_stream;
        size_t decLen = block_header_fileds;

        rlp_stream.appendList(decLen);
        rlp_stream << m_inner_header.encodeBytes();
        rlp_stream << m_prev_hash;
        m_next_elections_groups.streamRLP(rlp_stream);

        rlp_stream.appendList(m_block_signatures_nodes.size());
        for(auto & signature_node : m_block_signatures_nodes) {
            signature_node.streamRLP(rlp_stream);
        }
        
        _bytes.insert(_bytes.begin() + 1, rlp_stream.out().begin(), rlp_stream.out().end());
    } else{
         xwarn("xrelay_block_header::streamRLP_header_to_contract block_version %d not support!", m_version);
    }
    
    return _bytes;
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

void    xrelay_block_header::set_block_height(uint64_t height)
{
    m_inner_header.set_block_height(height);
}

void    xrelay_block_header::set_epochid(uint64_t id)
{
    m_inner_header.set_epochid(id);
}

void xrelay_block_header::set_timestamp(uint64_t timestamp)
{
    m_inner_header.set_timestamp(timestamp);
}


void xrelay_block_header::set_elections_next(const xrelay_election_group_t &elections)
{   
    m_next_elections_groups.election_epochID = elections.election_epochID;
    for (auto &election : elections.elections_vector) {
        m_next_elections_groups.elections_vector.emplace_back(election);
    }
}


void xrelay_block_header::add_signature_nodes(std::vector<xrelay_signature_node_t> signature_nodes)
{
    if (signature_nodes.size() > 0) {
        m_block_signatures_nodes = signature_nodes;
    }
}

void xrelay_block_header::make_inner_hash()
{
    m_inner_header.make_inner_hash();
}

///////////////////xrelay_block start /////////
xrelay_block::xrelay_block(uint8_t block_version, evm_common::h256  prev_hash, uint64_t block_height,
                          uint64_t epochID, uint64_t timestamp)
{
    switch (block_version)
    {
    case 0:
        m_version = block_version;
        m_header.set_prev_hash(prev_hash);
        m_header.set_block_height(block_height);
        m_header.set_epochid(epochID);
        m_header.set_timestamp(timestamp);
        break;
    default:
        xwarn("xrelay_block::xrelay_block block_version %d not support!", m_version);
        break;
    }

}

void xrelay_block::set_elections_next(const xrelay_election_group_t &elections)
{   
    m_header.set_elections_next(elections);
}

void xrelay_block::set_transactions(const std::vector<xeth_transaction_t> &transactions)
{
    for (auto & tx: transactions) {
        m_transactions.emplace_back(tx);
    }
}

void xrelay_block::set_receipts(const std::vector<xeth_receipt_t> &receipts)
{
    for (auto & receipt: receipts) {
        m_receipts.emplace_back(receipt);
    }
}

void xrelay_block::add_signature_nodes(std::vector<xrelay_signature_node_t> signature_nodes)
{
    m_header.add_signature_nodes(signature_nodes);
}


void xrelay_block::make_txs_root_hash()
{
    std::vector<bytes> rlp_txs;
    for (auto & tx: m_transactions) {
        rlp_txs.push_back(tx.encodeBytes());
    }
    h256 txsRoot = orderedTrieRoot(rlp_txs);
    xinfo("txsRoot root hash[%s]", txsRoot.hex().c_str());
    m_header.set_txs_root_hash(txsRoot);
}


void xrelay_block::make_receipts_root_hash()
{
    std::vector<bytes> rlp_receipts;
    for (auto & receipt: m_receipts) {
        rlp_receipts.push_back(receipt.encodeBytes());
    }
    h256 receiptsRoot = orderedTrieRoot(rlp_receipts);
    xinfo("receipts root hash[%s]", receiptsRoot.hex().c_str());
    m_header.set_receipts_root_hash(receiptsRoot);
}

 xbytes_t   xrelay_block::encodeBytes(bool withSignature) const 
 {
    xbytes_t _bytes;
    _bytes.push_back(m_version);

    if (m_version == 0) {
        evm_common::RLPStream _s;
        streamRLP(_s, withSignature);
        _bytes.insert(_bytes.begin() + 1, _s.out().begin(), _s.out().end());
    }
    return _bytes;
}


void   xrelay_block::streamRLP(evm_common::RLPStream &rlp_stream, bool withSignature) const
{
    rlp_stream.appendList(block_fileds);
    rlp_stream << m_header.encodeBytes(withSignature);
    rlp_stream.appendList(m_receipts.size());
    for (auto &receipt: m_receipts) {
        rlp_stream << receipt.encodeBytes();
    }
   
    rlp_stream.appendList(m_transactions.size());
    for (auto &tx :m_transactions) {
        rlp_stream << tx.encodeBytes();
    }
}


bool xrelay_block::decodeBytes(xbytes_t const& _d, std::error_code & ec, bool withSignature) {

    if (_d.size() < 2) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block::decodeBytes fail bytes,%zu", _d.size());
        return false;
    }

    m_version = (uint8_t)_d.front();
    if (m_version != 0) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("xrelay_block::decodeBytes fail invalid version,%d", m_version);
        return false;
    }

    xbytes_t _d2(_d.begin() + 1, _d.end());
    RLP _r(_d2);
    return decodeRLP(_r, ec, withSignature);
}


bool xrelay_block::decodeRLP(evm_common::RLP const& _r, std::error_code &ec, bool withSignature)
{
    if (!_r.isList() || _r.itemCount() != block_fileds) {
        ec = common::error::xerrc_t::invalid_rlp_stream;
        xerror("decodeRLP::decodeRLP fail item count,%d", _r.itemCount());
        return false;
    }

    m_header.decodeBytes(_r[0].toBytes(), ec, withSignature);

    evm_common::RLP const& rlp_receiptsList = RLP(_r[1].data());
    unsigned itemCount = rlp_receiptsList.itemCount();
    for (unsigned i = 0; i < itemCount; i++) {
        xeth_receipt_t receipt;
        receipt.decodeBytes(rlp_receiptsList[i].toBytes(), ec);
        m_receipts.emplace_back(receipt);
    }

    evm_common::RLP const& rlp_txList = RLP(_r[2].data());
    itemCount = rlp_txList.itemCount();
    for (unsigned i = 0; i < itemCount; i++) {
        xeth_transaction_t tx;
        eth_error  ecode;
        tx.decodeBytes(rlp_txList[i].toBytes(), ecode);
        m_transactions.emplace_back(tx);
    }
    return true;
}

void    xrelay_block::make_block_hash()
{
    evm_common::RLPStream _s(4); 
    _s << m_header.get_header_version();
    _s << m_header.get_inner_header_hash();
    _s << m_header.get_prev_block_hash();
    m_header.get_elections_sets().streamRLP(_s);

    m_header.m_block_hash = sha3(_s.out());
    xinfo("xrelay_block::make_block_hash  data[%s]", toHex(_s.out()).c_str());
    xinfo("xrelay_block::make_block_hash  hash[%s]", m_header.m_block_hash.hex().c_str());
}

void xrelay_block::make_block_root_hash()
{
    //todo 
    xinfo("xrelay_block::make_block_root_hash last block hash[%s]", m_header.m_prev_hash.hex().c_str());
    h256 block_root_hash{0};
    m_header.set_block_root_hash(block_root_hash);
}


bool    xrelay_block::build_finish()
{
    //todo, add checklist later on
    //calc hash 
    make_txs_root_hash();
    make_receipts_root_hash();
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

xbytes_t xrelay_block::streamRLP_header_to_contract()
{
   return m_header.streamRLP_header_to_contract();
}


std::string xrelay_block::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,
            sizeof(local_param_buf),
            "{height:%lu,epochid:%lu,timestamp:%lu,election size:%u,receipts size:%u}",
            m_header.m_inner_header.get_block_height(),
            m_header.m_inner_header.get_epochID(),
            m_header.m_inner_header.get_timestamp(),
            (uint32_t)m_header.get_elections_sets().elections_vector.size(),
            (uint32_t)m_receipts.size());
    return std::string(local_param_buf);
}

NS_END2