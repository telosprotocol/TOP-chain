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
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xevm_common/xtriecommon.h"


NS_BEG2(top, xrelayer)

using namespace top::evm_common;
using namespace top::evm_common::rlp;

relayer::relayer() {}

   

void relayer::relayer_init() {
 
    //create proof vecotr
 /*  for(int i =0; i< 20; i++){
        utl::xecprikey_t raw_pri_key_obj;
       proof_public_vector.emplace_back(raw_pri_key_obj);
   }*/

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

    //cala calculate
    encoder.EncodeBytesFixArray(m_next_block.header.prev_block_hash).EncodeBytesFixArray(m_next_block.header.next_block_inner_hash)
    .EncodeInteger(m_next_block.header.height).EncodeBytesFixArray(m_next_block.header.epoch_id).EncodeBytesFixArray(m_next_block.header.next_epoch_id)
    .EncodeBytesFixArray(m_next_block.header.prev_state_root).EncodeBytesFixArray(m_next_block.header.outcome_root)
    .EncodeInteger(m_next_block.header.timestamp).EncodeBytesFixArray(m_next_block.header.next_bp_hash).EncodeBytesFixArray(m_next_block.header.block_merkle_root);
    

  
    
    encoder.EncodeBytesFixArray(m_next_block.inner_rest_hash).EncodeInteger(m_next_block.next_bps.some);
    if (m_next_block.next_bps.some == true) {
         //add len before blockProducers
        uint32_t blockProducersLen = m_next_block.next_bps.blockProducers.size();
        encoder.EncodeInteger(blockProducersLen);
        std::cout << " blockProducersLen   " << blockProducersLen << std::endl;
        for (auto blockpro : m_next_block.next_bps.blockProducers) {
             encoder.EncodeInteger(blockpro.publicKey.version).EncodeInteger(blockpro.publicKey.keyType)
             .EncodeInteger(blockpro.publicKey.bt8).EncodeBytesFixArray(blockpro.publicKey.k)
             .EncodeInteger(blockpro.stake);
        }
    }
    
   //add len before blockProducers
    uint32_t approvals_after_nextLen = m_next_block.approvals_after_next.size();
    encoder.EncodeInteger(approvals_after_nextLen);
    std::cout << " approvals_after_nextLen   " << approvals_after_nextLen << std::endl;


    for (auto opt_signatrue :m_next_block.approvals_after_next)    {
        encoder.EncodeInteger(opt_signatrue.some).EncodeInteger(opt_signatrue.signature.signatureType)
        .EncodeBytesFixArray(opt_signatrue.signature.r).EncodeBytesFixArray(opt_signatrue.signature.s);
    }


    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

}



void relayer::relayer_block_proofs_set(std::vector<xRelayerBlockProducer> &block_proofs)
{
    for (auto proof : block_proofs) {
        m_block_proofs.emplace_back(proof);
    }
}

void relayer::relayer_receipts_set(std::vector<xRelayerReceipt> &receipts)
{
    for (auto receipt : receipts) {
        m_receipts.emplace_back(receipt);
    }
}


void relayer::relayer_approvals_next_set(std::vector<xRelayerOptionalSignature> &approve_nexts)
{
    for (auto approve : approve_nexts) {
        m_approve_next.emplace_back(approve);
    }
}


void relayer::relayer_new_header_build()
{
  
    xRelayerBlockInnerHeader& header = m_next_block.header;

    if (m_fulloutProof.outcome_proof.block_hash != h256{0})  {
       header.prev_block_hash = m_fulloutProof.outcome_proof.block_hash;
    } else {
       header.prev_block_hash = h256{0};
    }

    //next_block_inner_hash
    top::utl::xecprikey_t raw_pri_key_obj;
    bytesConstRef((const unsigned char *)raw_pri_key_obj.data(), 32).copyTo(header.next_block_inner_hash .ref());

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


     res.height = data.decodeU64();
        res.epoch_id = data.decodeBytes32();
        res.next_epoch_id = data.decodeBytes32();
        res.prev_state_root = data.decodeBytes32();
        res.outcome_root = data.decodeBytes32();
        res.timestamp = data.decodeU64();
        res.next_bp_hash = data.decodeBytes32();
        res.block_merkle_root = data.decodeBytes32();

    //sign inner_lite hash
     xBorshEncoder encoder_inner_lite_hash;
     encoer.EncodeBytesFixArray(header.epoch_id).EncodeBytesFixArray(header.next_epoch_id)
     .EncodeBytesFixArray(header.prev_state_root).EncodeBytesFixArray(header.outcome_root)
    .EncodeInteger(header.timestamp).EncodeBytesFixArray(header.next_bp_hash)
    EncodeBytesFixArray(header.outcome_root);

    std::ostringstream out;
    for (auto c : encoder_inner_lite_hash.GetBuffer()) {
           out << c ;
    }
    const std::string raw_digest = out.str();
    h256 inner_lite_hash =  sha3(raw_digest);

    
    //next_block_inner_hash
    top::utl::xecprikey_t raw_pri_key_obj;
    bytesConstRef((const unsigned char *)raw_pri_key_obj.data(), 32).copyTo(header.inner_rest_hash.ref());

    

   //block hash(inner_lite.hash, res.inner_rest_hash), res.prev_block_hash)
  /*  std::string  message =      uint8(0),                    topBlock.hash,
                    Utils.swapBytes8(topBlock.inner_lite.height),
                    bytes23(0)*/
    m_next_block.next_bps.some = 1;
    //merkle block proof root hash
    std::vector<std::string>  block_proof_hash_vector;
    int index =0;
    for(auto & block_proof : m_block_proofs) {
       // std::string tmp{};
        byte signature_result[64]{0};
         ed25519_sign(message, message.length(), block_proof.publicKey.p_k.data(),
                     block_proof.publicKey.k.data(), signature_result);

        xRelayerOptionalSignature proof ;        
        proof.signature.r = bytes(signature_result[0], signature_result[31]);
        proof.signature.s = bytes(signature_result[32], signature_result[63]);

        block_proof_hash_vector.emplace_back(block_proof.publicKey.k.hex());
        m_next_block.approvals_after_next.emplace_back(proof);
        index++;
    }
    
    base::xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string root_hash = merkle.calc_root(block_proof_hash_vector);
    header.block_merkle_root = h256{root_hash};
    

   for(auto & block_proof : m_block_proofs) {
       // std::string tmp{};
        block_proof_hash_vector.emplace_back(block_proof.publicKey.k.hex());
        m_next_block.next_bps.blockProducers.emplace_back(block_proof);
    }

}

void relayer::relayer_new_block_build()
{

    if (m_block_proofs.size() ==0) {
        xerror("proof empty!");
        assert(false);
    }

    //test
    create_block_file();
}

    



NS_END2
