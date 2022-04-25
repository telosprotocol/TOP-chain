
// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#pragma once
#include <vector>
#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"



namespace top {
   
namespace xrelayer {
        
using namespace top::evm_common;

typedef  struct _xRelayerReceiptLog {
    h160               contractAddress; 
    bytes              data;
    std::vector<bytes> topics;
}xRelayerReceiptLog;

typedef  struct  _xRelayerReceipt{
    uint8_t status;
    h256    gasUsed;
    h2048   logsBloom;
    std::vector<xRelayerReceiptLog> logs;
}xRelayerReceipt;


typedef  struct _xRelayerPublicKey {
    uint8_t     version;    //if version =0 ,isChunkOnly not borsh,  version = 1 isChunkOnly  borsh,but no used
    uint32_t    keyType;    //no used,must 0
    uint8_t     bt8;        //no used
    h256        k;                 //key
}xRelayerPublicKey;

typedef   struct _xRelayerBlockProducer {
    xRelayerPublicKey publicKey;      //voter key struct
    u128              stake;                 //voter stake
}xRelayerBlockProducer;

typedef   struct _xRealyerBlockProducers {
    bool some;                                    //true is exist blockProducers, false is empty
    std::vector<xRelayerBlockProducer> blockProducers;  //voter for block 
}xRealyerBlockProducers;


typedef   struct _xRelayerSignature {
    uint8_t signatureType;      //no used
    h256     r;                 
    h256     s;
}xRelayerSignature;

typedef  struct _xRelayerOptionalSignature {
    bool some;                  //check signature if exist
    xRelayerSignature signature;      
}xRelayerOptionalSignature;


typedef struct     _xRelayerBlockInnerHeader {
    h256    prev_block_hash;         //32 byte, 
    h256    next_block_inner_hash;   //32 byte

    uint64_t height;                    // Height of this block since the genesis block (height 0).
    h256     epoch_id;                // byt32 Epoch start hash of this block's epoch. Used for retrieving validator information
    h256     next_epoch_id;           // next validator info
    //not used
    h256     prev_state_root;         // Root hash of the state at the previous block.   ???
    h256     outcome_root;            // Root of the outcomes of transactions and receipts.
    uint64_t timestamp;               // Timestamp at which the block was built.
    //not used
    h256     next_bp_hash;            // Hash of the next epoch block producers set    
    h256     block_merkle_root;       //every block merlke 
}xRelayerBlockInnerHeader;


typedef struct _xRelayerBlock {
    xRelayerBlockInnerHeader               header;
    h256                                   inner_rest_hash;             //32 byte
    xRealyerBlockProducers                 next_bps;   //nex block producer info 
    std::vector<xRelayerOptionalSignature> approvals_after_next;   //next approval info 
}xRelayerBlock;



}
}



