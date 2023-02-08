#pragma once

#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "tests/mock/xcertauth_util.hpp"

namespace top {
namespace mock {

class xtableblock_util {
 public:
    static base::xvblock_t* create_tableblock(std::vector<base::xvblock_t*> &units, base::xvblock_t* prev_tableblock) {
        // assert(prev_tableblock != nullptr);
        // if (units.size() == 0) {
        //     base::xvblock_t* next_block = xblocktool_t::create_next_emptyblock(prev_tableblock);
        //     assert(next_block != nullptr);
        //     xcertauth_util::instance().do_multi_sign(next_block);

        //     return next_block;
        // }
        // xblock_consensus_para_t cs_para{xcertauth_util::instance().get_leader_xip(), prev_tableblock};
        // xtable_block_para_t table_para;
        // for (auto& unit : units) {
        //     table_para.add_unit(unit, 1);
        // }

        // base::xvblock_t* proposal_block = xblocktool_t::create_next_tableblock(table_para, cs_para, prev_tableblock);

        // xcertauth_util::instance().do_multi_sign(proposal_block);
        // assert(!proposal_block->get_cert()->get_verify_signature().empty());
        // return proposal_block;
        assert(false);
        return nullptr;
    }

    static base::xvblock_t* create_tableblock_no_sign(std::vector<base::xvblock_t*> &units, base::xvblock_t* prev_tableblock, const xvip2_t &leader_xip) {
        // assert(prev_tableblock != nullptr);
        // if (units.size() == 0) {
        //     base::xvblock_t* next_block = xblocktool_t::create_next_emptyblock(prev_tableblock);
        //     assert(next_block != nullptr);

        //     return next_block;
        // }
        // xblock_consensus_para_t cs_para{leader_xip, prev_tableblock};
        // xtable_block_para_t table_para;
        // for (auto& unit : units) {
        //     table_para.add_unit(unit, 1);
        // }

        // base::xvblock_t* proposal_block = xblocktool_t::create_next_tableblock(table_para, cs_para, prev_tableblock);
        // return proposal_block;
        assert(false);
        return nullptr;        
    }
};

}
}
