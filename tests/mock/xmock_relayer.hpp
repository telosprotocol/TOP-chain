
#pragma once
#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"

#include <iostream>
#include <limits>
#include <fstream>


namespace top {
namespace mock {


class relayer_mock {

public:
    relayer_mock();
    ~relayer_mock();


   void relayer_init();
   void relayer_approval_add();
   void  relayer_receipts_create(uint64_t height, uint32_t tx_count);

private:

   void relayer_fullOutProoff();

   void genesis_block_init();

private:
    x_LightClientBlock m_genes_block{0};

}


}
}
