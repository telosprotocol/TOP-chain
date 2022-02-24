// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xvm/xvote/xvote_engine.h"

NS_BEG2(top, xvm)

bool xvote_engine::validate_proposal(const xproposal_info_t& proposal_info) const
{
    do {
        // if ( !proposal_info.is_hash_valid() ) {
        //     xinfo_vm("proposal hash not valid");
        //     break;
        // }
        if ( proposal_info.m_proposal_open == static_cast<uint8_t>(enum_proposal_option_open_type::enum_proposal_option_unopen)
            && (proposal_info.m_option_map.size() < 2 
                || proposal_info.m_max_vote_option > proposal_info.m_option_map.size()) )
        {
            xinfo_vm("proposal unopen need at least two option or max_vote error");
            break;
        }

        return true;
    } while (0);
    return false;
}

NS_END2