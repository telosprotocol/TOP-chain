#include "xdata/xfulltableblock_account_data.h"
#include "xdata/xfull_tableblock.h"
#include "xvledger/xvcnode.h"
#include "xvledger/xvledger.h"

NS_BEG3(top, vnode, components)
using namespace top::data;

xfulltableblock_statistic_accounts fulltableblock_statistic_accounts(xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service);
NS_END3