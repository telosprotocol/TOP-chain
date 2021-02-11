// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xvledger.h"
#include "xbasic/xns_macro.h"

NS_BEG2(top, application)

class xapplication_util {
 public:
    static base::xauto_ptr<base::xvblock_t> get_latest_lightunit(base::xvblockstore_t* blockstore, const std::string & address) {
        base::xauto_ptr<base::xvblock_t> vblock = blockstore->get_latest_committed_block(address);
        if (vblock != nullptr && vblock->get_block_class() == base::enum_xvblock_class_full) {
            xassert(vblock->get_height() > 1);
            return blockstore->load_block_object(address, vblock->get_height() - 1);
        }
        return vblock;
    }
    static base::xauto_ptr<base::xvblock_t> get_prev_lightunit(base::xvblockstore_t* blockstore, const std::string & address, uint64_t height) {
        base::xauto_ptr<base::xvblock_t> vblock = blockstore->load_block_object(address, height);
        if (vblock != nullptr && vblock->get_block_class() == base::enum_xvblock_class_full) {
            xassert(height > 1);
            return blockstore->load_block_object(address, height - 1);
        }
        return vblock;
    }
};

NS_END2
