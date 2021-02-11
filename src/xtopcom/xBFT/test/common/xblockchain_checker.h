// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xtestnode.hpp"

NS_BEG2(top, test)

/**
 * check blockchains state:
 * 1, height & hash pointer continuity
 * 2, same blocks between all blockchain
 * 
 */
class xblockchain_checker_t {
public:
    xblockchain_checker_t(std::vector<xtestnode_t *> const &nodes);
    virtual ~xblockchain_checker_t();

    void do_check();

protected:
    uint32_t update_cache();
    bool     vertical_check(uint32_t max) const;

protected:
    std::vector<xtestnode_t *> m_nodes{};

    std::unordered_map<std::string, std::vector<base::xvblock_t *>> m_cache;
};

NS_END2