// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xtxexecutor/xtablemaker_mgr.h"

NS_BEG2(top, txexecutor)

xtable_maker_ptr_t xtablemaker_mgr::get_tablemaker(const std::string & account) {
    auto iter = m_table_makers.find(account);
    if (iter != m_table_makers.end()) {
        return iter->second;
    }
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(account, m_resources);
    m_table_makers[account] = tablemaker;
    return tablemaker;
}


NS_END2
