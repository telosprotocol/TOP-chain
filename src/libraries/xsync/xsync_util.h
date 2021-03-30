#pragma once

#include "xbasic/xns_macro.h"
#include "xbase/xmem.h"
#include "xdata/xblock.h"
#include "xsync/xsync_store.h"

NS_BEG2(top, sync)

data::xblock_ptr_t autoptr_to_blockptr(base::xauto_ptr<base::xvblock_t> &autoptr);
bool is_beacon_table(const std::string &address);
bool check_auth(const observer_ptr<base::xvcertauth_t> &certauth, data::xblock_ptr_t &block);

NS_END2
