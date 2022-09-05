#include "common_func.h"
#include "../common.h"
#include "../../mock/xmock_network_config.hpp"
#include "../../mock/xmock_network.hpp"
#include "xdata/xdatautil.h"
#include "xdata/xblocktool.h"
#include "xdata/xnative_contract_address.h"
#include "xsync/xsync_util.h"


using namespace top;
using namespace top::sync;
using namespace top::mock;
using namespace top::data;

 std::vector<top::data::xblock_ptr_t>  create_emptyblock_with_address(const std::string& address, uint32_t block_count)
 {
     std::vector<top::data::xblock_ptr_t> vector_blocks;
     base::xvblock_t* genesis_block = top::data::xblocktool_t::create_genesis_empty_table(address);
     base::xvblock_t* prev_block = genesis_block;
     for (uint64_t i = 0; i < 10; i++) {
         prev_block = top::data::xblocktool_t::create_next_emptyblock(prev_block);
         prev_block->add_ref();
         base::xauto_ptr<base::xvblock_t> autoptr = prev_block;
         xblock_ptr_t block_ptr = autoptr_to_blockptr(autoptr);
         vector_blocks.push_back(block_ptr);
     }
     return vector_blocks;
 }
