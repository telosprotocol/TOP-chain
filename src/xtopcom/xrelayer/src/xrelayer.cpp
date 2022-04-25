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



/**
 *
 * 
 * /TOP/rank-top/tests/xevm_engine_test/test_cases/erc20/erc20.json  
  "src_address": "T600044dce5c8961e283786cb31ad7fc072347227d7ea2",
            "target_address": "contract_one",
            "data": "0xa9059cbb000000000000000000000000000000000000000000000000000000000000007b0000000000000000000000000000000000000000000000000000000000003039",
            "gas_limit": 50942,
            "value": 0,
            "expected": {
                "status": 0,
                "extra_message": "",
                "gas_used": 50942,
                "logs": [
                    {
                        "address": "contract_one",
                        "topics": [
                            "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                            "0x0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2",
                            "0x000000000000000000000000000000000000000000000000000000000000007b"
                        ],
                        "data": "0x0000000000000000000000000000000000000000000000000000000000003039"
                    }
                ]
            }
        },
 */


void relayer::relayer_receipts_create(uint64_t height, uint32_t receipt_count) {

  struct timeval beg;
  uint64_t rand_time = 0;

    m_receiptVector.clear();
  for (uint32_t i = 0; i < receipt_count; i++) {

    xRelayerReceipt receipt;
    receipt.status = 0x1;
    receipt.gasUsed = h256(50942);
    
    xRelayerReceiptLog log;
    log.contractAddress = h160{"real contract address"};   //tod

    std::string data = "0x0000000000000000000000000000000000000000000000000000000000003039";
    log.data = bytes(data.begin(),data.end());

    std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
    std::string strAddress =  top::HexDecode(address_str); 
    top::evm_common::h2048 bloom;
    char szDigest[32] = {0};
    keccak_256((const unsigned char *)strAddress.data(), strAddress.size(), (unsigned char *)szDigest);
    top::evm_common::h256 hash_h256;
    bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
    bloom.shiftBloom<3>(hash_h256);

    std::vector<std::string> topics = {"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                       "0x0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2",
                                       "0x000000000000000000000000000000000000000000000000000000000000007b"};
    for (auto & topic : topics) {
        std::string strTopic =  top::HexDecode(topic);
        char szDigest[32] = {0};
        keccak_256((const unsigned char *)strTopic.data(), strTopic.size(), (unsigned char *)szDigest);
        top::evm_common::h256 hash_h256;
        bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
        bloom.shiftBloom<3>(hash_h256);

        log.topics.push_back(bytes(topic.begin(), topic.end()));
    }
    std::stringstream outstrbloom;
    outstrbloom << bloom;
    std::string bloom_str = outstrbloom.str();

    receipt.logsBloom = h2048{bloom_str};
    receipt.logs.push_back(log);
    m_receiptVector.push_back(receipt);
  }

//  h256 receiptsRoot = orderedTrieRoot(receipts);

}

void relayer::relayer_fullOutProoff() {

    

}


h256 relayer::relayer_outcome_root_calc()
{
    //RLP.encodeList(postTxStateRLP, cumulativeGasRLP, bloomRLP, logInfoListRLP) ... //交易收据的RLP序列化，对应黄皮书的21公式
    std::vector<bytes> receipt_rlp;
    for(auto receipt : m_receiptVector) {
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


}


void relayer::genesis_block_init() {

     //inner_lite
    xRelayerBlockInnerHeader& header = m_genes_block.header;
    header.height = 0;

    header.epoch_id = h256{"00000000000000000000000000000000"};        //not used now
    header.next_epoch_id = h256{"00000000000000000000000000000000"};        //not used now
    header.prev_state_root = h256{"00000000000000000000000000000000"};        //not used

    //create receipt and comeout root
    relayer_receipts_create(0, 1);
    header.outcome_root = relayer_outcome_root_calc();
    header.timestamp =  GetCurrentTimeMsec();
    header.next_bp_hash = h256{"00000000000000000000000000000000"};;  //not used

    //merkle tree root 
    std::vector<std::string>  proof_hash_vector;
    for(auto & pub_key : proof_public_vector) {
        const std::string pub_key_data = pub_key.get_compress_public_key();
        proof_hash_vector.emplace_back(pub_key_data);
    }

    base::xmerkle_t<utl::xsha2_256_t, uint256_t> merkle;
    std::string root_hash = merkle.calc_root(proof_hash_vector);
    header.block_merkle_root = h256{root_hash};
    
    utl::xecprikey_t  random_data; //using xecprikey_t generate random data
    uint256_t msg_digest((uint8_t*)random_data.data());

    header.inner_rest_hash = h256{msg_digest.data()};

    header.ne


   //total block hash
    xRelayerBlockInnerHeader               header;
    h256                                   inner_rest_hash;             //32 byte
    xRealyerBlockProducers                 next_bps;   //nex block producer info 
    std::vector<xRelayerOptionalSignature> approvals_after_next;   //next approval info 


   


}


NS_END2
