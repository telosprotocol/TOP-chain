// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvm/xvm_define.h"
#include "xvm/xvote/xproposal.h"
NS_BEG2(top, xvm)
class xvote_engine
{
    public:
        bool validate_proposal(const xproposal_info_t& proposal_info) const;
};
NS_END2