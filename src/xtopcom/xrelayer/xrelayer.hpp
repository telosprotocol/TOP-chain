
#pragma once

#include "xcrypto/xckey.h"
#include "xrelayer_header.hpp"
#include "xrelayer_proof.hpp"

#include <iostream>
#include <limits>
#include <fstream>


namespace top{
    
namespace xrelayer {
    

class relayer {

public:
    relayer();
    ~relayer(){};


   void relayer_init();
   void relayer_approval_add();
   void relayer_receipts_create(uint64_t height, uint32_t receipt_count);
   h256 relayer_outcome_root_calc();

private:
   void create_block_file();
   void relayer_fullOutProoff();

   void genesis_block_init();

    void relayer_all_proofs_set(std::vector<h256> proofs);
    void relayer_block_proofs_set(std::vector<xRelayerBlockProducer> block_proofs);
    void relayer_proofs_set(std::vector<h256> proofs);
    void relayer_receipts_set( std::vector<xRelayerReceipt> receipts);
    void relayer_approvals_next_set(std::vector<xRelayerOptionalSignature> approve_nexts);


private:

    xRelayerBlock m_last_block{0};
    xRelayerBlock m_next_block;
    std::vector<xRelayerBlockProducer>  m_proofs;
    std::vecotr<xRelayerBlockProducer>  m_block_proofs;
    std::vector<xRelayerReceipt> m_receipts;
    std::vector<xRelayerOptionalSignature> m_approve_next;



   //test data
   utl::xecprikey_t epoch_key_obj_now;
   utl::xecprikey_t epoch_key_obj_next;

   std::vector<utl::xecprikey_t>  proof_public_vector;
   std::vector<utl::xecprikey_t>  proof_pri_vector;
};

} // namespace relayer


} // namespace top
