// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include <assert.h>
#include <cinttypes>

#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xdatautil.h"
#include "xdata/xblock.h"
#include "xdata/xnative_contract_address.h"

namespace top { namespace data {

using base::xstring_utl;

std::string xdatautil::serialize_owner_str(const std::string & prefix, uint32_t table_id) {
    return base::xvaccount_t::make_account_address(prefix, (uint16_t)table_id);
}
bool xdatautil::deserialize_owner_str(const std::string & address, std::string & prefix, uint32_t & table_id) {
    uint16_t subaddr{0xFFFF};   // TODO: jimmy, bypass release compiling error
    auto ret = base::xvaccount_t::get_prefix_subaddr_from_account(address, prefix, subaddr);
    table_id = subaddr;
    return ret;
}

bool xdatautil::extract_table_id_from_address(const std::string & address, uint32_t & table_id) {
    std::string prefix;
    return deserialize_owner_str(address, prefix, table_id);
}

bool xdatautil::extract_parts(const std::string& address, std::string& base_addr, uint32_t& table_id) {
    if (address == sys_drand_addr)
        return false;
    return deserialize_owner_str(address, base_addr, table_id);
}

std::string xdatautil::xip_to_hex(const xvip2_t & xip) {
    char data[33] = {0};
    snprintf(data, 33, "%" PRIx64 ":%" PRIx64, xip.high_addr, xip.low_addr);
    return std::string(data);
}

}  // namespace data
}  // namespace top
