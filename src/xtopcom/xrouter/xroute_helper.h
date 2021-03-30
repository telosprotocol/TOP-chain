#include "xcommon/xnode_type.h"
#include "xdata/xdata_common.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_addr_type.h"
NS_BEG2(top, router)
using common::xnode_type_t;
using data::xcontract_addr_type;

REGISTER_ADDR_TYPE(sys_contract_zec_elect_consensus_addr, common::xnode_type_t::zec);
REGISTER_ADDR_TYPE(sys_contract_zec_group_assoc_addr, common::xnode_type_t::zec);

NS_END2
