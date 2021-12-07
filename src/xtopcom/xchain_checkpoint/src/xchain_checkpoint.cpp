// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_checkpoint/xchain_checkpoint.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbase/xutl.h"
#if defined(CHECKPOINT_TEST)
#    include "xchain_checkpoint/xchain_checkpoint_test.h"
#elif defined(XBUILD_CI) || defined(XBUILD_DEV)
#    include "xchain_checkpoint/xchain_checkpoint_default.h"
#elif defined(XBUILD_GALILEO)
#    include "xchain_checkpoint/xchain_checkpoint_galileo.h"
#else
#    include "xchain_checkpoint/xchain_checkpoint_new_horizons.h"
#endif


template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

#define BLOCK_HEIGHT_KEY "height"
#define BLOCK_HASH_KEY "hash"

namespace top {
namespace chain_checkpoint {

auto init = []() -> xcheckpoints_map_t {
    xcheckpoints_map_t m;
    auto j = json::parse(checkpoint_json);
    for (auto it = j.cbegin(); it != j.cend(); it++) {
        auto timer_height = base::xstring_utl::touint64(static_cast<std::string>(it.key()));
        auto const & info_map = it.value();
        for (auto it_in = info_map.cbegin(); it_in != info_map.cend(); it_in++) {
            auto account = static_cast<std::string>(it_in.key());
            auto height_str = static_cast<std::string>(it_in->at(BLOCK_HEIGHT_KEY).get<std::string>());
            auto hash = static_cast<std::string>(it_in->at(BLOCK_HASH_KEY).get<std::string>());
            xcheckpoint_info_t checkpoint_info{base::xstring_utl::touint64(height_str), hash};
            m[account].emplace(std::make_pair(timer_height, checkpoint_info));
        }
    }
    j.clear();
    return m;
};

xcheckpoints_map_t xtop_chain_checkpoint::m_checkpoints_map = init();

xcheckpoints_t const xtop_chain_checkpoint::checkpoints(std::string account) {
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        return {};
    }
    return it->second;
}

}  // namespace chain_data
}  // namespace top
