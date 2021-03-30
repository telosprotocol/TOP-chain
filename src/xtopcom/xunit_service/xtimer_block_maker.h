// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xemptyblock.h"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_utl.h"

NS_BEG2(top, xunit_service)

class xtimer_block_maker_t : public xunit_service::xblock_maker_face {
public:
    explicit xtimer_block_maker_t(const std::shared_ptr<xcons_service_para_face> para);

    base::xauto_ptr<base::xvblock_t> get_latest_block(const std::string &account) override;
    base::xvblock_t *                make_block(const std::string &account, uint64_t clock, uint64_t viewid, uint16_t threshold, const xvip2_t &leader_xip) override;
    base::xvblock_t *                make_block(const std::string &account, const xblock_maker_para_t &para, const xvip2_t &leader_xip) override;
    int                              verify_block(base::xvblock_t *proposal_block) override;

private:
    std::shared_ptr<xcons_service_para_face> m_param;
};

NS_END2
