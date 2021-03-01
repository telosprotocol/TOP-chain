// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xvledger.h"
#include "xcommon/xip.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xdata/xgenesis_data.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

void
print_help(std::string const & executable_name) {
    std::cout << executable_name << " <count> [address type] [zone index] [network id]" << std::endl;
    std::cout << " " << "address type: 0, 1 are of user type addresses.  others are system addresses." << std::endl;
    std::cout << "  " << "system types includes:" << std::endl;
    std::cout << "   " << "2: system contract type" << std::endl;
    std::cout << "   " << "3: custom contract type" << std::endl;
    std::cout << "   " << "a: block address type" << std::endl;
    std::cout << " " << "network id: 0 represents mainnet.  If you don't set it, defaults to mainnet(0)" << std::endl;
    std::cout << " " << "zone index: we have at most 16 zone index." << std::endl;
    std::cout << "  " << "0: consensus zone index" << std::endl;
    std::cout << "  " << "1: rec zone index" << std::endl;
    std::cout << "  " << "2: zec zone index" << std::endl;
    std::cout << "  " << "14: archive zone index" << std::endl;
    std::cout << "  " << "15: edge zone index" << std::endl;
}

using namespace top::data;

int main(int argc, char * argv[]) {
    std::ios_base::sync_with_stdio(false);

    if (argc <= 1 || argc > 5) {
        print_help(argv[0]);
        exit(1);
    }

    int32_t count{0};
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::base::enum_vaccount_addr_type account_address_type{top::base::enum_vaccount_addr_type_secp256k1_user_account};
    top::base::enum_xchain_zone_index zone_index{top::base::enum_chain_zone_consensus_index};
    switch (argc) {
        default:
        case 5:
            network_id = top::common::xnetwork_id_t{ static_cast<top::common::xnetwork_id_t::value_type>(std::stoi(argv[4]))};
        case 4:
            zone_index = static_cast<top::base::enum_xchain_zone_index>(std::stoi(argv[3]));
        case 3:
            account_address_type = static_cast<top::base::enum_vaccount_addr_type>(argv[2][0]);
        case 2:
            count = std::stoi(argv[1]);
    }

    uint16_t ledger_id = top::base::xvaccount_t::make_ledger_id(static_cast<top::base::enum_xchain_id>(network_id.value()), zone_index);
    for (int32_t i = 0; i < count; i++) {
        if (account_address_type == top::base::enum_vaccount_addr_type_secp256k1_user_account) {
            top::utl::xecprikey_t private_key;
            auto public_key = private_key.get_public_key();
            std::string account_address = private_key.to_account_address(account_address_type, ledger_id);
            auto base64_prk = top::base::xstring_utl::base64_encode((unsigned char*)private_key.data(), 32);
            auto base64_pub = top::base::xstring_utl::base64_encode((unsigned char*)public_key.data(), public_key.size());
            std::cout << account_address << "\tprivate key: " << base64_prk << "\tpublic key: " << base64_pub << std::endl;
        } else {
            auto hash = top::utl::xsha2_512_t::digest(std::to_string(std::time(nullptr) + i));
            auto prefix_account = top::utl::xcrypto_util::make_address_by_assigned_key(hash.data(), account_address_type, ledger_id);
            std::cout << prefix_account << std::endl;
        }
    }

    return 0;
}
