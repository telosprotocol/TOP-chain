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

    static std::string     make_consensus_table_address(uint16_t subaddr) {
        base::enum_xchain_zone_index zone = base::enum_chain_zone_consensus_index;
        return base::xvaccount_t::make_table_account_address(zone, subaddr);
    }

    static std::string    make_unit_address(base::enum_xchain_zone_index zone, uint16_t subaddr) {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, zone);
        uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        std::string addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id) + "@" + base::xstring_utl::tostring(subaddr);
        return addr;
    }

    static std::vector<std::string>  make_multi_user_address_in_table(const std::string & table_addr, uint32_t count) {
        base::xvaccount_t vaddr(table_addr);
        std::vector<std::string> uaddrs;
        for (uint32_t i = 0; i < count; i++) {
            std::string unitaddr = make_unit_address((base::enum_xchain_zone_index)vaddr.get_zone_index(), (uint16_t)vaddr.get_ledger_subaddr());
            uaddrs.push_back(unitaddr);
        }
        return uaddrs;
    }
};

}
}
