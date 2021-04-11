#pragma once

#include "xbasic/xns_macro.h"
#include "xbase/xmem.h"
#include "xdata/xblock.h"
#include "xsync/xsync_store.h"
#include "xvnetwork/xaddress.h"
#include "xvledger/xvcnode.h"

NS_BEG2(top, sync)

data::xblock_ptr_t autoptr_to_blockptr(base::xauto_ptr<base::xvblock_t> &autoptr);
bool is_beacon_table(const std::string &address);
bool check_auth(const observer_ptr<base::xvcertauth_t> &certauth, data::xblock_ptr_t &block);
uint32_t vrf_value(const std::string& hash);
vnetwork::xvnode_address_t build_address_from_vnode(const xvip2_t &group_xip2, const std::vector<base::xvnode_t*> &nodes, int32_t slot_id);

NS_END2
