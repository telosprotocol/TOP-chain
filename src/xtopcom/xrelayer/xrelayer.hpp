
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

private:
  xRelayerBlock m_genes_block;
  std::vector<xRelayerReceipt> m_receiptVector{};
   //test data
   utl::xecprikey_t epoch_key_obj_now;
   utl::xecprikey_t epoch_key_obj_next;

   std::vector<utl::xecprikey_t>  proof_public_vector;
   std::vector<utl::xecprikey_t>  proof_pri_vector;
};

} // namespace relayer


} // namespace top
