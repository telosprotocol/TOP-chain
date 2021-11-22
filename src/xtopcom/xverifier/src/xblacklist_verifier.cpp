// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xverifier/xblacklist_verifier.h"

NS_BEG2(top, xverifier)

bool xtop_blacklist_utl::is_black_address(std::string const& addr) {
    auto const& black_addrs = black_config();
    return std::find(black_addrs.begin(), black_addrs.end(), addr) != std::end(black_addrs);
}


std::vector<std::string>  xtop_blacklist_utl::black_config() {
    std::string offchain_config;
    std::string onchain_config;
    config::config_register.get("local_blacklist", offchain_config);
    config::config_register.get("blacklist", onchain_config);

    xdbg("[xtop_blacklist_utl::black_config] offchain: %s", offchain_config.c_str());
    xdbg("[xtop_blacklist_utl::black_config] onchain: %s", onchain_config.c_str());

    std::vector<std::string> res;
    std::vector<std::string> vec;
    if (!offchain_config.empty()) {
        vec.clear();
        base::xstring_utl::split_string(offchain_config, ',', vec);
        for (auto const& v: vec)  res.push_back(v);
    }

    if (!onchain_config.empty()) {
        vec.clear();
        base::xstring_utl::split_string(onchain_config, ',', vec);

        for (auto const& v: vec) res.push_back(v);
    }

#ifdef DEBUG
    std::string black_config_str{""};
    for (auto const& v: res) black_config_str += v + ",";
    xdbg("[xtop_blacklist_utl::black_config] black_config_str: %s", black_config_str.c_str());
#endif
    return res;
}


NS_END2

