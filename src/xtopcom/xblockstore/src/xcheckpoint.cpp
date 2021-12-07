// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockstore/src/xcheckpoint.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbase/xutl.h"
#if defined(CHECKPOINT_TEST)
#    include "xblockstore/xcheckpoint_data/xcheckpoint_test.h"
#elif defined(XBUILD_CI) || defined(XBUILD_DEV)
#    include "xblockstore/xcheckpoint_data/xcheckpoint_default.h"
#elif defined(XBUILD_GALILEO)
#    include "xblockstore/xcheckpoint_data/xcheckpoint_galileo.h"
#else
#    include "xblockstore/xcheckpoint_data/xcheckpoint_new_horizons.h"
#endif

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

#define BLOCK_HEIGHT_KEY "height"
#define BLOCK_HASH_KEY "hash"

namespace top {
namespace store {

auto load = []() -> xcheckpoints_map_t {
    xcheckpoints_map_t m;
    auto j = json::parse(checkpoint_json);
    for (auto it = j.cbegin(); it != j.cend(); it++) {
        // ignore clock here because it can also be verified by hash
        auto const & info_map = it.value();
        for (auto it_in = info_map.cbegin(); it_in != info_map.cend(); it_in++) {
            auto account = static_cast<std::string>(it_in.key());
            auto height = base::xstring_utl::touint64(it_in->at(BLOCK_HEIGHT_KEY).get<std::string>());
            auto hash = base::xstring_utl::from_hex(it_in->at(BLOCK_HASH_KEY).get<std::string>());
            m[account].emplace(std::make_pair(height, hash));
        }
    }
    j.clear();
    return m;
};

xcheckpoints_map_t xtop_chain_checkpoint::m_checkpoints_map = load();

xcheckpoints_t const & xtop_chain_checkpoint::checkpoints(std::string const & account) {
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        static xcheckpoints_t empty;
        return empty;
    }
    return it->second;
}

}  // namespace chain_data
}  // namespace top

