// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xtxexecutor/xtable_maker.h"
NS_BEG2(top, txexecutor)

class xtablemaker_mgr {
 public:
    explicit xtablemaker_mgr(const xblockmaker_resources_ptr_t & resources)
    : m_resources(resources) {}

 public:
    xtable_maker_ptr_t get_tablemaker(const std::string & account);

 private:
    xblockmaker_resources_ptr_t m_resources;
    std::map<std::string, xtable_maker_ptr_t> m_table_makers;
};


NS_END2
