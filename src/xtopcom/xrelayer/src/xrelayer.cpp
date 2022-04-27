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
#include "../xtopcom/xdepends/include/trezor-crypto/ed25519-donna/ed25519.h"

#include "xpbase/base/top_utils.h" 

NS_BEG2(top, xrelayer)

using namespace top::evm_common;
using namespace top::evm_common::rlp;

relayer::relayer() {}

//test api
void printHexHash(std::string hash_name, h256 hash_data){
    
    std::cout << "hash name: " << hash_name << " value : " <<  hash_data.hex() << std::endl;   
}

void relayer::relayer_init() {
 

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
    std::string fileName = "block_index_" +  base::xstring_utl::tostring(block_index) + ".bin";
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

uint64_t messageSwapBytes8(uint64_t v) {
    v = ((v & 0x00ff00ff00ff00ff) << 8) | ((v & 0xff00ff00ff00ff00) >> 8);
    v = ((v & 0x0000ffff0000ffff) << 16) | ((v & 0xffff0000ffff0000) >> 16);
    return (v << 32) | (v >> 32);
}


void relayer::relayer_new_header_build()
{
  
    xRelayerBlockInnerHeader& header = m_next_block.header;

    //1. set all data
    if (m_fulloutProof.outcome_proof.block_hash != h256{0})  {
       header.prev_block_hash = m_fulloutProof.outcome_proof.block_hash;
    } else {
       header.prev_block_hash = h256{0};
    }
    printHexHash("prev_block_hash" , header.prev_block_hash);

    //next_block_inner_hash , it's unusefull now
    top::utl::xecprikey_t raw_pri_key_obj;
    bytesConstRef((const unsigned char *)raw_pri_key_obj.data(), 32).copyTo(header.next_block_inner_hash .ref());
    printHexHash("header.next_block_inner_hash" ,header.next_block_inner_hash);

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

    //crate message to sign
    //block hash(inner_lite.hash, res.inner_rest_hash), res.prev_block_hash)

  /*  std::string  message =      uint8(0),                    topBlock.hash,
                    Utils.swapBytes8(topBlock.inner_lite.height),
                    bytes23(0)*/
    m_next_block.next_bps.some = true;
     std::string  message;
    //merkle block proof root hash

    //calc block_merkle_root
    std::vector<std::string>  block_proof_hash_vector;
    int index = 0;
    for(auto & block_proof : m_block_proofs) {
        xRelayerOptionalSignature proof ;        
        std::string publickey((char *)block_proof.publicKey.k.data() , 32);
       //  std::cout << " block_proof.publicKey.k. " << block_proof.publicKey.k  << " , publickey "  << publickey << std::endl;      
        block_proof_hash_vector.emplace_back(publickey);
    }

    base::xmerkle_t<utl::xsha2_256_t, uint256_t> blook_merkle; 
    std::string block_merkle_hash = blook_merkle.calc_root(block_proof_hash_vector);
    bytesConstRef((const unsigned char *)block_merkle_hash.data(), 32).copyTo(header.block_merkle_root.ref());
    printHexHash("(header.block_merkle_root " , header.block_merkle_root);

    //sign inner_lite hash
    xBorshEncoder encoder_inner_lite_hash;
    encoder_inner_lite_hash.EncodeInteger(header.height).EncodeBytesFixArray(header.epoch_id).EncodeBytesFixArray(header.next_epoch_id)
    .EncodeBytesFixArray(header.prev_state_root).EncodeBytesFixArray(header.outcome_root)
    .EncodeInteger(header.timestamp).EncodeBytesFixArray(header.next_bp_hash)
    .EncodeBytesFixArray(header.block_merkle_root);

    std::ostringstream out;
    for (auto c : encoder_inner_lite_hash.GetBuffer()) {
           out << c ;
    }
    const std::string raw_digest = out.str();
    h256 inner_lite_hash =  sha3(raw_digest);

    
    //next_block_inner_hash
    top::utl::xecprikey_t inner_rest_hash_key;
    bytesConstRef((const unsigned char *)inner_rest_hash_key.data(), 32).copyTo(m_next_block.inner_rest_hash.ref());

    h512 hash_256;
    for (int i = 0; i < 32; i++) {
        hash_256[i] = inner_lite_hash[i];
        hash_256[i+32] = m_next_block.inner_rest_hash[i];
    }

    utl::xkeccak256_t first_hash;
    first_hash.reset();
    first_hash.update(hash_256.data(), 64);
    first_hash.update(header.prev_block_hash.data(), 32);    
    

    uint256_t hash_result;
    first_hash.get_hash(hash_result);

    uint64_t bytesHeight =  messageSwapBytes8(header.height);
//sign message     uint8(0), topBlock.hash, Utils.swapBytes8(topBlock.inner_lite.height), bytes23(0)
    unsigned char messageSign[64]= {0};
    memcpy(&messageSign[1], (const unsigned char*)hash_result.data(), 32);
    memcpy(&messageSign[33], (const unsigned char*)&bytesHeight, 8);

    //all block proof to sign this message
    for(auto & block_proof : m_block_proofs) {
        byte signature_result[64]{0};
        ed25519_sign((const unsigned char *)messageSign, 64, block_proof.publicKey.p_k.data(),
                                block_proof.publicKey.k.data(), signature_result);


        h512 h_512;
        bytesConstRef((const unsigned char *)messageSign, 62).copyTo(h_512.ref());
        std::cout << "h_512  " << h_512.hex() << std::endl;

        xRelayerOptionalSignature proof;        
        //check sign 
        int check = ed25519_sign_open(messageSign, 64, block_proof.publicKey.k.data(), signature_result);
        if(check != 0){
            std::cout << " check sign error! " << std::endl;
        }
        proof.some = 1;
        bytesConstRef((const unsigned char *)signature_result, 31).copyTo( proof.signature.r.ref());
        bytesConstRef((const unsigned char *)&signature_result[32], 31).copyTo( proof.signature.s.ref());

        m_next_block.approvals_after_next.push_back(proof);
    }

   for(auto & block_proof : m_block_proofs) {
       // std::string tmp{};
        m_next_block.next_bps.blockProducers.emplace_back(block_proof);
    }
}

void relayer::relayer_new_block_build()
{

    if (m_block_proofs.size() ==0) {
        xerror("proof empty!");
        assert(false);
    }

    relayer_new_header_build();
    //test
    create_block_file();

    m_last_block = m_next_block;
    m_fulloutProof.outcome_proof.block_hash = m_next_block.header.prev_block_hash;
 
}

    



NS_END2
