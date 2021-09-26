
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xsystem_contract_runtime/xerror/xerror.h"

#include "xdata/xfulltableblock_account_data.h"
#include "xdata/xfull_tableblock.h"
#include "xvledger/xvcnode.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, contract_runtime)
using namespace top::data;


xfulltableblock_statistic_accounts fulltableblock_statistic_accounts(top::data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service);
void  process_fulltableblock(xblock_ptr_t const& block);


NS_END2