// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once
#include <map>
#include <string>

#include "xstore/xstore_util.h"
#include "xmetrics/xmetrics.h"

using namespace top::metrics;

std::map<std::string, xmetircs_tag_t> key_start = {
    {"t/", xmetircs_tag_t::db_key_tx},
    {"i/", xmetircs_tag_t::db_key_block_index},
    {"b/", xmetircs_tag_t::db_key_block}
};

std::map<std::string, xmetircs_tag_t> key_end = {
    {"/h", xmetircs_tag_t::db_key_block_object},
    {"/i", xmetircs_tag_t::db_key_block_input},
    {"/ir", xmetircs_tag_t::db_key_block_input_resource},
    {"/o", xmetircs_tag_t::db_key_block_output},
    {"/or", xmetircs_tag_t::db_key_block_output_resource},
    {"/s", xmetircs_tag_t::db_key_block_state},
    {"/d", xmetircs_tag_t::db_key_block_offdata}
};

void xstore_util::metirc_key_value(std::string const& key, std::string const& value, bool add_or_minus) {
#ifdef DB_KV_STATISTIC
    assert(key.size() > 3);

    for (auto const& start_item: key_start) {
        if (key.find(start_item.first) == 0) {

           if (add_or_minus) {
               XMETRICS_GAUGE(start_item.second, value.size());
           } else {
               XMETRICS_GAUGE(start_item.second, -value.size());
           }
           if (start_item.second == xmetircs_tag_t::db_key_block) {
               for (auto const& end_item: key_end) {
                   if (endwith(key, end_item.first)) {
                       if (add_or_minus) {
                           XMETRICS_GAUGE(end_item.second, value.size());
                       } else {
                           XMETRICS_GAUGE(end_item.second, -value.size());
                       }

                       break;
                   }
               }
           }
           break;
        }
    }
#endif
}

bool xstore_util::endwith(std::string const& str, std::string const& suffix) {
    if (str.size() < suffix.size()) return false;
    return str.substr(str.size() - suffix.size(), suffix.size()) == suffix;
}
