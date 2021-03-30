// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xunit_service/xcons_face.h"
#include <cinttypes>
#include <string>
#include <vector>
#include "xdata/xblocktool.h"
#include "xunitblock.hpp"
#include "xunit_service/xcons_utl.h"

namespace top {
namespace mock {
using data::xemptyblock_t;
using xunit_service::xcons_service_para_face;
using xunit_service::xcons_utl;

class xempty_block_maker : public xunit_service::xblock_maker_face {
public:
    explicit xempty_block_maker(const std::shared_ptr<xcons_service_para_face>& para):m_para(para) {
    }

    virtual base::xauto_ptr<base::xvblock_t> get_latest_block(const std::string &account) {
        auto block_store = m_para->get_resources()->get_vblockstore();
        return block_store->get_latest_cert_block(account);
    }

    virtual base::xvblock_t *make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t& leader_xip) {
        xassert(0);  // TODO(jimmy) delete
        return nullptr;
    }

    virtual int verify_block(base::xvblock_t *proposal_block) {
        auto                           block_store = m_para->get_resources()->get_vblockstore();
        auto                           last_hash = proposal_block->get_last_block_hash();
        auto                           last_block = block_store->get_latest_cert_block(proposal_block->get_account());
        if (proposal_block->get_height() == last_block->get_height() + 1 && last_block->get_block_hash() == last_hash) {
            return 0;
        }
        // xassert(0);
        xwarn("[xunitservice] block %s height not match leader:%" PRIu64 " vs local %" PRIu64 " node:%s", proposal_block->get_account().c_str(),
                        proposal_block->get_height(), last_block->get_height(), m_para->get_resources()->get_account().c_str());
        return xconsensus::enum_xconsensus_error_bad_height;
    }

private:
   std::shared_ptr<xcons_service_para_face> m_para;
};
} // namespace mock
} // namespace top
