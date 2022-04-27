
#pragma once


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


   void create_block_file();
   void relayer_fullOutProoff();
   void relayer_new_header_build();
    void relayer_new_block_build();
    void relayer_block_proofs_set(std::vector<xRelayerBlockProducer>  &block_proofs);

    void relayer_approvals_next_set(std::vector<xRelayerOptionalSignature> &approve_nexts);
    void relayer_receipts_set( std::vector<xRelayerReceipt> &receipts);

private:

    xFullOutcomeProof  m_fulloutProof{};
    xRelayerBlock m_last_block{};
    xRelayerBlock m_next_block{};
    std::vector<xRelayerBlockProducer>      m_block_proofs;
    std::vector<xRelayerReceipt>            m_receipts;
    std::vector<xRelayerOptionalSignature>  m_approve_next;


};

} // namespace relayer


} // namespace top
