// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "xbasic/xobject_ptr.h"
#include "xbasic/xns_macro.h"
#include "xutility/xhash.h"

NS_BEG2(top, xvm)
using std::string;
using std::vector;
using std::array;
using std::unordered_map;
using utl::xsha3_256_t;

enum class enum_proposal_active_type : uint8_t
{
    enum_proposal_unactive = 0,
    enum_proposal_active
};

enum class enum_proposal_option_active_type : uint8_t
{
    enum_proposal_option_unactive = 0,
    enum_proposal_option_active
};

enum class enum_proposal_limit_voter_type : uint8_t
{
    enum_proposal_no_limit_voter = 0,
    enum_proposal_limit_voter
};

enum class enum_proposal_option_open_type : uint8_t
{
    enum_proposal_option_unopen = 0,
    enum_proposal_option_open
};

enum class enum_proposal_proxy_type : uint8_t
{
    enum_proposal_unsupport_proxy = 0,
    enum_proposal_support_proxy
};

enum class enum_proposal_voted_type : uint8_t
{
    enum_proposal_unvoted = 0,
    enum_proposal_voted
};

struct xvote_staked_t
{
    string      m_option_content;
    string      m_voter_addr;
    uint64_t    m_vote_staked;
};

#define OVERLOADED_MACRO(M, ...) _OVR(M, _COUNT_ARGS(__VA_ARGS__)) (__VA_ARGS__)
#define _OVR(macroName, number_of_args)   _OVR_EXPAND(macroName, number_of_args)
#define _OVR_EXPAND(macroName, number_of_args)    macroName##number_of_args

#define _COUNT_ARGS(...)  _ARG_PATTERN_MATCH(__VA_ARGS__, 9,8,7,6,5,4,3,2,1)
#define _ARG_PATTERN_MATCH(_1,_2,_3,_4,_5,_6,_7,_8,_9, N, ...)   N

#define DIGEST_GEN(...)     OVERLOADED_MACRO(DIGEST_GEN, __VA_ARGS__)


#define DIGEST_GEN2(arg1, arg2) \
array<uint8_t, 64> hash_array;\
uint256_t arg_digest = xsha3_256_t::digest(arg1);\
memcpy(hash_array.data(), arg_digest.data(), arg_digest.size());\
arg_digest = xsha3_256_t::digest(arg2);\
memcpy(hash_array.data() + 32, arg_digest.data(), arg_digest.size());\
arg_digest = xsha3_256_t::digest(hash_array.data(), hash_array.size())

// #define DIGEST_GEN3(arg1, arg2, arg3) \
// array<uint8_t, 96> hash_array;\
// uint256_t arg_digest = xsha3_256_t::digest(arg1);\
// memcpy(hash_array.data(), arg_digest.data(), arg_digest.size());\
// arg_digest = xsha3_256_t::digest(arg2);\
// memcpy(hash_array.data() + sizeof(uint256_t), arg_digest.data(), arg_digest.size());\
// arg_digest = xsha3_256_t::digest(arg3);\
// memcpy(hash_array.data() + 2 * sizeof(uint256_t), arg_digest.data(), arg_digest.size());\
// arg_digest = xsha3_256_t::digest(hash_array.data(), hash_array.size())

#define DIGEST_GEN4(arg1, arg2, arg3, arg4) \
array<uint8_t, 128> hash_array;\
uint256_t arg_digest = xsha3_256_t::digest(arg1);\
memcpy(hash_array.data(), arg_digest.data(), arg_digest.size());\
arg_digest = xsha3_256_t::digest(arg2);\
memcpy(hash_array.data() + 32, arg_digest.data(), arg_digest.size());\
arg_digest = xsha3_256_t::digest(arg3);\
memcpy(hash_array.data() + 64, arg_digest.data(), arg_digest.size());\
arg_digest = xsha3_256_t::digest(arg4);\
memcpy(hash_array.data() + 96, arg_digest.data(), arg_digest.size());\
arg_digest = xsha3_256_t::digest(hash_array.data(), hash_array.size())

//rpc call struct
class xproposal_vote_option_t
{
public:
    uint256_t get_digest()
    {
        // todo check the m_proposal_owern length
        DIGEST_GEN(m_proposal_owern, m_proposal_content);
        return arg_digest;
    }
public:
    string      m_proposal_owern;
    string      m_proposal_content;
    vector<xvote_staked_t>   m_option_vote_list;
};

class xproposal_vote_limit_t
{
public:
    string get_digest()
    {
        // todo check the m_proposal_owern length
        DIGEST_GEN(m_proposal_owern, m_proposal_content);
        return string((char *)arg_digest.data(), arg_digest.size());
    }

    unordered_map<string, string> get_limit_voter_hash()
    {
        unordered_map<string, string> limit_voter_map;
        for(auto& iter : m_voter_list) {
            DIGEST_GEN(m_proposal_owern, m_proposal_content, "vote_limit", iter);
            limit_voter_map.insert(make_pair(string((char *)arg_digest.data(), arg_digest.size()), iter));
        }
        return limit_voter_map;
    }
public:
    string      m_proposal_owern;
    string      m_proposal_content;
    vector<string> m_voter_list;
};

class xproposal_vote_proxy_rpc_t
{
public:
    string get_proposal_digest()
    {
        // todo check the m_proposal_owern length
        DIGEST_GEN(m_proposal_owern, m_proposal_content);
        return string((char *)arg_digest.data(), arg_digest.size());
    }

    string get_proposal_voter_digest()
    {
        // todo check the m_proposal_owern length
        DIGEST_GEN(m_proposal_owern, m_proposal_content, "voter", m_voter_addr);
        return string((char *)arg_digest.data(), arg_digest.size());
    }

public:
    string      m_proposal_owern;
    string      m_proposal_content;
    string      m_voter_addr;
    string      m_proxy_addr;
    uint64_t    m_staked;
};

class xproposal_submit_option_rpc_t
{
public:
    string get_proposal_digest()
    {
        // todo check the m_proposal_owern length
        DIGEST_GEN(m_proposal_owern, m_proposal_content);
        return string((char *)arg_digest.data(), arg_digest.size());
    }
public:
    string      m_proposal_owern;
    string      m_proposal_content;
    string      m_submit_addr;
    string      m_option;
};

//rpc call struct end

class xproposal_option_info_t
{
public:
    int32_t serialize_write(base::xstream_t & stream)
    {
        const int32_t begin_size = stream.size();
        //stream << m_proposal_hash;
        //stream << m_option_hash;
        stream << m_option_content;
        stream << m_option_owern;
        stream << m_total_votes;
        stream << m_is_active;

        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }
    int32_t serialize_read(base::xstream_t & stream)
    {
        const int32_t begin_size = stream.size();
        //stream >> m_proposal_hash;
        //stream >> m_option_hash;
        stream >> m_option_content;
        stream >> m_option_owern;
        stream >> m_total_votes;
        stream >> m_is_active;

        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }
    friend base::xstream_t& operator<< (base::xstream_t& stream, const xproposal_option_info_t &proposal_option_info);
    friend base::xstream_t& operator>> (base::xstream_t& stream, xproposal_option_info_t &proposal_option_info);

public:
    //uint256_t   m_proposal_hash;
    //uint256_t   m_option_hash;
    string      m_option_content;
    string      m_option_owern;
    uint64_t    m_total_votes{0};
    uint8_t     m_is_active{0};//0 no valid 1 valid
};

class xproposal_info_t
{
public:
    int32_t serialize_write(base::xstream_t & stream);
    int32_t serialize_read(base::xstream_t & stream);
    uint256_t get_digest()
    {
        // todo check the m_proposal_owern length
        // todo check the m_proposal_owern length
        DIGEST_GEN(m_proposal_owern, m_proposal_content);
        return arg_digest;
    }
    // bool is_hash_valid()
    // {
    //     return m_proposal_hash == get_digest();
    // }
public:
    //uint256_t   m_proposal_hash;
    string      m_proposal_owern;
    string      m_proposal_content;
    uint32_t    m_end_time{0};
    uint8_t     m_limit_vote{0};//0 no limit 1 limit voter
    uint8_t     m_proposal_open{0};//0 no open 1 open to end user
    uint8_t     m_max_vote_option{0};
    uint8_t     m_is_proxy{0};//0 no proxy 1 support proxy vote
    unordered_map<string, xproposal_option_info_t> m_option_map;
    //vector<xproposal_option_info_t> m_option_list;
};

class xproposal_limit_voter_info_t
{
public:
    int32_t serialize_write(base::xstream_t & stream)
    {
        const int32_t begin_size = stream.size();
        //stream << m_proposal_hash;
        stream << m_voter_addr;

        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }
    int32_t serialize_read(base::xstream_t & stream)
    {
        const int32_t begin_size = stream.size();
        //stream >> m_proposal_hash;
        stream >> m_voter_addr;

        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }
public:
    //uint256_t   m_proposal_hash;
    string      m_voter_addr;
};

class xproposal_vote_info_t
{
public:
    int32_t serialize_write(base::xstream_t & stream)
    {
        const int32_t begin_size = stream.size();
        //stream << m_proposal_hash;
        stream << m_voter_addr;
        stream << m_proxy_addr;
        stream << m_staked;
        stream << m_voted;

        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }
    int32_t serialize_read(base::xstream_t & stream)
    {
        const int32_t begin_size = stream.size();
        //stream >> m_proposal_hash;
        stream >> m_voter_addr;
        stream >> m_proxy_addr;
        stream >> m_staked;
        stream >> m_voted;

        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }
    // string get_digest()
    // {
    //     // todo check the m_proposal_owern length
    //     string digest_str;
    //     digest_str.append(m_proposal_hash.data(), m_proposal_hash.size());
    //     uint256_t voter_addr_digest = xsha3_256_t::digest(m_voter_addr);
    //     digest_str.append(voter_addr_digest.data(), voter_addr_digest.size());
    //     uint256_t vote_digest = xsha3_256_t::digest(digest_str);
    //     return string(vote_digest.data(), vote_digest.size());
    // }
public:
    //uint256_t   m_proposal_hash;
    string      m_voter_addr;
    string      m_proxy_addr;
    uint64_t    m_staked{0};
    uint8_t     m_voted{0};//0 no vote 1 voted
};

NS_END2
