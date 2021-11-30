// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmbus/xevent_store.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, mbus)

data::xblock_ptr_t extract_block_from(xevent_store_block_to_db_ptr_t const & ev, const int etag) noexcept {
    xdbg("extract_block_from connect block event,account=%s,height=%ld", ev->owner.c_str(), ev->blk_height);
    auto * blkstore = base::xvchain_t::instance().get_xblockstore();
    xobject_ptr_t<base::xvblock_t> blk = blkstore->load_block_object(base::xvaccount_t{ ev->owner }, ev->blk_height, ev->blk_hash, false, etag);
    xassert(blk != nullptr);
    return dynamic_xobject_ptr_cast<data::xblock_t>(blk);
}

data::xblock_ptr_t extract_block_from(xevent_store_block_committed_ptr_t const & ev, const int etag) noexcept {
    xdbg("extract_block_from commit block event,account=%s,height=%ld", ev->owner.c_str(), ev->blk_height);
    auto * blkstore = base::xvchain_t::instance().get_xblockstore();
    xobject_ptr_t<base::xvblock_t> blk = blkstore->load_block_object(base::xvaccount_t{ ev->owner }, ev->blk_height, ev->blk_hash, false, etag);
    if(nullptr == blk) {
        xerror("extract_block_from: nullptr block,account=%s,height=%ld", ev->owner.c_str(), ev->blk_height);
        xassert(blk != nullptr);
        return nullptr;
    }
    return dynamic_xobject_ptr_cast<data::xblock_t>(blk);
}

NS_END2
