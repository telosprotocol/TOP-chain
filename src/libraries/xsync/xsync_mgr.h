// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>
#include "xvnetwork/xaddress.h"
#include "xsync/xrequest.h"
#include "xsync/xchain_info.h"

NS_BEG2(top, sync)

class xsync_mgr_t {
public:
    xsync_mgr_t(std::string vnode_id, const std::string &address);
    xentire_block_request_ptr_t create_request(uint64_t start_height, uint32_t count);

protected:
    std::string m_vnode_id;
    std::string m_address;
};

NS_END2
