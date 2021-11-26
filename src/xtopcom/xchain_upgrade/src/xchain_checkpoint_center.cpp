// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_upgrade/xchain_checkpoint_center.h"

#include "nlohmann/json.hpp"
#include "xbase/xutl.h"
#if defined(XBUILD_CHECKPOINT_TEST)
#    include "xchain_upgrade/xchain_checkpoint_test.h"
#elif defined(XBUILD_CI) || defined(XBUILD_DEV)
#    include "xchain_upgrade/xchain_checkpoint_default.h"
#elif defined(XBUILD_GALILEO)
#    include "xchain_upgrade/xchain_checkpoint_galileo.h"
#else
#    include "xchain_upgrade/xchain_checkpoint_new_horizons.h"
#endif

using json = nlohmann::json;

#define BLOCK_HEIGHT_KEY "heigh"
#define BLOCK_HASH_KEY "hash"

namespace top {
namespace chain_checkpoint {

auto checkpoint_json_parse = json::parse(checkpoint_json);

void xtop_chain_checkpoint::init() {
    for (auto it = checkpoint_json_parse.cbegin(); it != checkpoint_json_parse.cend(); it++) {
        std::map<std::string, xcheckpoint_info_t> one_checkpoint;
        auto timer_height = base::xstring_utl::touint64(static_cast<std::string>(it.key()));
        auto const & info_map = it.value();
        for (auto it_in = info_map.cbegin(); it_in != info_map.cend(); it_in++) {
            xcheckpoint_info_t one_account_checkpoint;
            auto account = static_cast<std::string>(it.key());
            auto height_str = static_cast<std::string>(it->at(BLOCK_HEIGHT_KEY).get<std::string>());
            auto hash = static_cast<std::string>(it->at(BLOCK_HASH_KEY).get<std::string>());
            one_account_checkpoint.height = base::xstring_utl::touint64(height_str);
            one_account_checkpoint.hash = hash;
            one_checkpoint.insert(std::make_pair(account, one_account_checkpoint));
        }
        m_checkpoints.emplace_back(std::make_pair(timer_height, one_checkpoint));
    }
    checkpoint_json_parse.clear();
}

xcheckpoints_t const & xtop_chain_checkpoint::checkpoints() {
    return m_checkpoints;
}

}  // namespace chain_data
}  // namespace top
