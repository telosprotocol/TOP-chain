// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xunit_service/xcons_face.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

NS_BEG2(top, blockmaker)

using data::xblock_consensus_para_t;
using data::xblock_t;

class xrelay_maker_t : public xblock_maker_t {
public:
    explicit xrelay_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xrelay_maker_t();

public:
    xblock_ptr_t make_proposal(xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result);
    int32_t verify_proposal(base::xvblock_t * proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para);

protected:
    bool verify_proposal_with_local(base::xvblock_t * proposal_block, base::xvblock_t * local_block) const;
    xblock_ptr_t make_relay_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code);
    bool can_make_next_relay_block() const;

private:
    xblock_builder_face_ptr_t m_relay_block_builder;
    xblock_builder_para_ptr_t m_default_builder_para;
    mutable std::mutex m_lock;
};

using xrelay_maker_ptr_t = xobject_ptr_t<xrelay_maker_t>;

NS_END2
