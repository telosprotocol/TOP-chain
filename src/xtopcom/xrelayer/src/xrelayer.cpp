#include "xrelayer/xrelayer.hpp"
#include "xbase/xns_macro.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xutl.h"
#include "trezor-crypto/sha3.h"
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>
#include "xevm_common/xtriehash.h"
#include "xevm_common/rlp.h"
#include "xvledger/xmerkle.hpp"
#include "xutility/xhash.h"

NS_BEG2(top, xrelayer)

using namespace top::evm_common;
using namespace top::evm_common::rlp;

relayer::relayer() {}

   

void relayer::relayer_init() {
 
    //create proof vecotr
   for(int i =0; i< 20; i++){
        utl::xecprikey_t raw_pri_key_obj;
       proof_public_vector.emplace_back(raw_pri_key_obj);
   }

}

void relayer::relayer_approval_add() {


}



void relayer::relayer_fullOutProoff() {

    
}


h256 relayer::relayer_outcome_root_calc()
{
    //RLP.encodeList(postTxStateRLP, cumulativeGasRLP, bloomRLP, logInfoListRLP) ... 
    std::vector<bytes> receipt_rlp;
    for(auto receipt : m_receipts) {
        bytes encoded = bytes();
        append(encoded, RLP::encode(receipt.status));
        append(encoded, RLP::encode(bytes(receipt.gasUsed.begin(), receipt.gasUsed.end())));
        append(encoded, RLP::encode(bytes(receipt.logsBloom.begin(), receipt.logsBloom.end())));
        for (auto log : receipt.logs ) {
            append(encoded, RLP::encode(bytes(log.contractAddress.begin(), log.contractAddress.end())));
            append(encoded, RLP::encode(bytes(log.data.begin(), log.data.end())));
            for (auto  topic : log.topics) {
                append(encoded, RLP::encode(bytes(topic.begin(), topic.end())));
            }
        }
        receipt_rlp.push_back(encoded);
    }

    h256 receiptsRoot = orderedTrieRoot(receipt_rlp);
    std::cout << "receiptsRoot  Hash: " <<  receiptsRoot.hex()  << std::endl;
    return receiptsRoot;
}

void relayer::create_block_file()
{
    static int block_index;
    block_index++;
    int writeLen = 0, fileLen;
    xBorshEncoder encoder;
    encoder.EncodeInteger(true);
    std::string fileName = "block_index_" +  base::xstring_utl::tostring(block_index);
    std::ofstream fin(fileName, std::ios::binary);

    xBorshEncoder encoder;

    //cala calculate
    encoder.EncodeStringFixArray(m_next_block.prev_block_hash).EncodeStringFixArray(m_next_block.next_block_inner_hash)
    .EncodeInteger(m_next_block.height).EncodeStringFixArray(m_next_block.epoch_id).EncodeStringFixArray(m_next_block.next_epoch_id)
    .EncodeStringFixArray(m_next_block.prev_state_root).EncodeStringFixArray(m_next_block.outcome_root)
    .EncodeInteger(m_next_block.timestamp).EncodeStringFixArray(m_next_block.next_bp_hash).EncodeStringFixArray(m_next_block.block_merkle_root);
    

    //sign
     xBorshEncoder encoder_sign;
     encoder_sign..EncodeInteger(m_next_block.height).EncodeStringFixArray(m_next_block.epoch_id).EncodeStringFixArray(m_next_block.next_epoch_id)
    .EncodeStringFixArray(m_next_block.prev_state_root).EncodeStringFixArray(m_next_block.outcome_root)
    .EncodeInteger(m_next_block.timestamp).EncodeStringFixArray(m_next_block.next_bp_hash).EncodeStringFixArray(m_next_block.block_merkle_root);
    

        std::ostringstream out;
    out << "[ ";
    for (h256 const & i : _bs)
        out << i.abridged() << ", ";
    out << "]";
    return out.str();

    for (auto c : encoder_sign.GetBuffer()) {
           out << c ;
    }

   
     for (auto & proof: m_block_proofs) {
    {
         utl::xecprikey_t raw_pri_key_obj;
        utl::xecpubkey_t raw_pub_key_obj = raw_pri_key_obj.get_public_key();
        const std::string uncompressed_pub_key_data((const char*)raw_pub_key_obj.data(),raw_pub_key_obj.size());
        const std::string compressed_pub_key_data = raw_pri_key_obj.get_compress_public_key();
        const std::string account_addr_from_raw_pri_key = raw_pri_key_obj.to_account_address(addr_type, ledger_id);
                
        utl::xecdsasig_t signature_obj = raw_pri_key_obj.sign(msg_digest);
    }
    



    m_next_block.inner_rest_hash =  sha3(reset_str);


     for (auto & proof: m_block_proofs) {
        m_next_block.xRealyerBlockProducers.blockProducers.emplace_back(proof);
        block_proofs_hash_vector.emplace_back(tostring(proof.publicKey.K));
    }
    
    
    encoder.EncodeStringFixArray(m_next_block.inner_rest_hash).EncodeInteger(m_next_block.next_bps.some);
    if (m_next_block.next_bps.some == true) {
         //add len before blockProducers
        uint32_t blockProducersLen = m_next_block.next_bps.blockProducers.size();
        encoder.EncodeInteger(blockProducersLen);
        std::cout << " blockProducersLen   " << blockProducersLen << std::endl;
        for (auto blockpro : m_next_block.next_bps.blockProducers) {
             encoder.EncodeInteger(blockpro.publicKey.version).EncodeInteger(blockpro.publicKey.keyType)
             .EncodeInteger(blockpro.publicKey.bt8).EncodeStringFixArray(blockpro.publicKey.k)
             .EncodeInteger(blockpro.stake);
        }
    }
    
   //add len before blockProducers
    uint32_t approvals_after_nextLen = m_next_block.approvals_after_next.size();
    encoder.EncodeInteger(approvals_after_nextLen);
    std::cout << " approvals_after_nextLen   " << approvals_after_nextLen << std::endl;



    for (auto opt_signatrue :m_next_block.approvals_after_next)    {
        encoder.EncodeInteger(opt_signatrue.some).EncodeInteger(opt_signatrue.signature.signatureType)
        .EncodeStringFixArray(opt_signatrue.signature.r).EncodeStringFixArray(opt_signatrue.signature.s);
    }

    int writeLen = 0;

    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

}


void relayer::relayer_all_proofs_set(std::vector<xRelayerBlockProducer> proofs)
{
    for (auto proof : proofs) {
        m_proofs.emplace_back(proof);
    }
}

void relayer::relayer_block_proofs_set(std::vector<xRelayerBlockProducer> block_proofs)
{
    for (auto proof : block_proofs) {
        m_block_proofs.emplace_back(block_proofs);
    }
}

void relayer::relayer_receipts_set(std::vector<xRelayerReceipt> receipts)
{
    for (auto receipt : receipts) {
        m_receipts.emplace_back(receipt);
    }
}


void relayer::relayer_approvals_next_set(std::vector<xRelayerOptionalSignature> approve_nexts)
{
    for (auto approve : approve_nexts) {
        m_approve_next.emplace_back(approve);
    }
}


void relayer::relayer_new_block_build()
{

    if (m_proofs.size() == 0 || !m_block_proofs.size() ==0) {
        xerror("proof empty!");
        assert(false);
    }

    //inner_lite
    xRelayerBlockInnerHeader& header = m_next_block.header;


    header.height = m_last_block.header.height +1;
    header.epoch_id = h256{"00000000000000000000000000000000"};        //not used now
    header.next_epoch_id = h256{"00000000000000000000000000000000"};        //not used now
    header.prev_state_root = h256{"00000000000000000000000000000000"};        //not used

    if (m_receipts.size() > 0) {
       header.outcome_root = relayer_outcome_root_calc();
    } else {
       header.outcome_root = h256{0};
    }
    header.timestamp =  GetCurrentTimeMsec();
    header.next_bp_hash = h256{0};;  //not used*/

    //merkle block proof root hash
    std::vector<std::string>  block_proof_hash_vector;
    for(auto & block_proof : m_block_proofs) {
        block_proof_hash_vector.emplace_back(toString(block_proof));
    }

    base::xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string root_hash = merkle.calc_root(proof_hash_vector);
    header.block_merkle_root = h256{root_hash};

    m_next_block.next_bps.some = 1;
    //merkle tree root 
    std::vector<std::string>  block_proofs_hash_vector;
    for (auto & proof: m_block_proofs) {
        m_next_block.next_bps.blockProducers.emplace_back(proof);
        block_proofs_hash_vector.emplace_back(tostring(proof.publicKey.K));
    }
 
    base::xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string block_proofs_root_hash = merkle.calc_root(block_proofs_hash_vector);
    header.block_merkle_root = h256{block_proofs_root_hash};
    


    //create inner_rest_hash
  /*  std::string  reset_str;
    const uint64_t randn_num = top::base::xtime_utl::get_fast_random64();
    uint64_t height = m_last_block.height;
    base::xstream_t _stream(base::xcontext_t::instance());
    const int32_t begin_size = _stream.size();
    _stream << randn_num;
    _stream << height;
    _stream << toString(header.outcome_root);
    //and so on
    const int32_t end_size = _stream.size();
    reset_str.assign((const char*)_stream.data(), _stream.size());*/





    if (m_last_block.header.prev_block_hash != h256{0})
    {
       header.prev_block_hash = m_last_block.header.prev_block_hash;
    } else {
       header.prev_block_hash =h256{0};
    }
    

    //merkle tree root 
    std::vector<std::string>  block_proofs_hash_vector;
    for (auto & proof: m_block_proofs) {
        m_next_block.xRealyerBlockProducers.blockProducers.emplace_back(proof);
        block_proofs_hash_vector.emplace_back(tostring(proof.publicKey.K));
    }
 
    base::xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string block_proofs_root_hash = merkle.calc_root(block_proofs_hash_vector);
    header.block_merkle_root = h256{block_proofs_root_hash};
    

    for (auto & approval: m_approve_next)   {
         header.approvals_after_next.emplace_back(approval);
    }
    
  }



}



NS_END2
