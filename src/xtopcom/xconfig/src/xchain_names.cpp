// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconfig/xconfig_register.h"
#include "xconfig/xchain_names.h"

NS_BEG2(top, config)


// used by para_proxy genesis node only?
void set_chain_name_id(std::string const & chain_name, uint32_t chain_id) {
    assert(chain_id != base::enum_main_chain_id && chain_id != base::enum_test_chain_id);
    assert(chain_name != chain_name_mainnet && chain_name != chain_name_testnet);
    assert(chain_name_id_dict.find(chain_name) == chain_name_id_dict.end());

    chain_name_id_dict.insert({chain_name, chain_id});
}

base::enum_xchain_id to_chainid(std::string const & chain_name) {
    // assert(chain_name == chain_name_mainnet || chain_name == chain_name_testnet);
    if (chain_name == chain_name_mainnet) {
        return base::enum_main_chain_id;
    } else if (chain_name == chain_name_testnet) {
        return base::enum_test_chain_id;
    } else if (chain_name_id_dict.find(chain_name)!=chain_name_id_dict.end()){
        return static_cast<base::enum_xchain_id>(chain_name_id_dict.at(chain_name));
    } else {
        return base::enum_service_chain_id_start_reserved;
    }
}

NS_END2
