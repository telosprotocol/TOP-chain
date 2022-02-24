// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvm/xvote/xproposal.h"

NS_BEG2(top, xvm)

base::xstream_t& operator<< (base::xstream_t& stream, const xproposal_option_info_t &proposal_option_info)
{
    stream << proposal_option_info.m_option_content;
    stream << proposal_option_info.m_option_owern;
    stream << proposal_option_info.m_total_votes;
    stream << proposal_option_info.m_is_active;
    return stream;
}

base::xstream_t& operator>> (base::xstream_t& stream, xproposal_option_info_t &proposal_option_info)
{
    stream >> proposal_option_info.m_option_content;
    stream >> proposal_option_info.m_option_owern;
    stream >> proposal_option_info.m_total_votes;
    stream >> proposal_option_info.m_is_active;
    return stream;
}

int32_t xproposal_info_t::serialize_write(base::xstream_t & stream)
{
    const int32_t begin_size = stream.size();
    // stream << m_proposal_hash;
    stream << m_proposal_owern;
    stream << m_proposal_content;
    stream << m_end_time;
    stream << m_limit_vote;
    stream << m_proposal_open;
    stream << m_max_vote_option;
    stream << m_is_proxy;
    SERIALIZE_TO_STREAM_MAP(stream, m_option_map);
    const int32_t end_size = stream.size();
    return (end_size - begin_size);
}
int32_t xproposal_info_t::serialize_read(base::xstream_t & stream)
{
    const int32_t begin_size = stream.size(); 
    // stream >> m_proposal_hash;
    stream >> m_proposal_owern;
    stream >> m_proposal_content;
    stream >> m_end_time;
    stream >> m_limit_vote;
    stream >> m_proposal_open;
    stream >> m_max_vote_option;
    stream >> m_is_proxy;
    SERIALIZE_FROM_STREAM_MAP(stream, m_option_map, string, xproposal_option_info_t);
    //SERIALIZE_FROM_STREAM_VECTOR_FACE(stream, m_option_list, xproposal_option_info_t);
    const int32_t end_size = stream.size();
    return (begin_size - end_size);
}

NS_END2