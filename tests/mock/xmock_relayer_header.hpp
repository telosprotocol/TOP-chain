
#pragma once
#include <vector>
#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"



typedef  struct PublicKey {
    uint8_t     version;    //if version =0 ,isChunkOnly not borsh,  version = 1 isChunkOnly  borsh,but no used
    uint32_t    keyType;    //no used,must 0
    uint8_t     bt8;        //no used
    h256 k;                 //key
}x_PublicKey;

typedef   struct BlockProducer {
    x_PublicKey publicKey;      //voter key struct
    u128 stake;                 //voter stake
}x_BlockProducer;

typedef   struct OptionalBlockProducers {
    bool some;                                    //true is exist blockProducers, false is empty
    std::vector<x_BlockProducer> blockProducers;  //voter for block 
}x_OptionalBlockProducers;


typedef   struct Signature {
    uint8_t signatureType;      //no used
    h256     r;                 
    h256     s;
}x_Signature;

typedef  struct OptionalSignature {
    bool some;                  //check signature if exist
    x_Signature signature;      
}x_OptionalSignature;


typedef struct LightClientBlockInnerHeader {
    h256    prev_block_hash;         //32 byte, 
    h256    next_block_inner_hash;   //32 byte

    uint64_t height;                    // Height of this block since the genesis block (height 0).
    h256     epoch_id;                // byt32 Epoch start hash of this block's epoch. Used for retrieving validator information
    h256     next_epoch_id;           // next validator info
    h256     prev_state_root;         // Root hash of the state at the previous block.
    h256     outcome_root;            // Root of the outcomes of transactions and receipts.
    uint64_t timestamp;               // Timestamp at which the block was built.
    h256     next_bp_hash;            // Hash of the next epoch block producers set    
    h256     block_merkle_root;       //every block merlke 
}x_LightClientBlockInnerHeader;


typedef struct LightClientBlock {

    x_LightClientBlockInnerHeader  header;
    h256                      inner_rest_hash;             //32 byte
    x_OptionalBlockProducers  next_bps;   //nex block producer info 
    std::vector<x_OptionalSignature> approvals_after_next;   //next approval info 
}x_LightClientBlock;



