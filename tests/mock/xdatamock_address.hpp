#pragma once

#include <string>
#include "xvledger/xvaccount.h"
#include "xcrypto/xcrypto_util.h"

namespace top {
namespace mock {

class xdatamock_address {
 public:
    static std::string     make_user_address_random() {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        std::string addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);
        return addr;
    }
    static std::string     make_user_address_random(uint16_t subaddr) {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        std::string addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id) + "@" + base::xstring_utl::tostring(subaddr);
        return addr;
    }
};

}
}
