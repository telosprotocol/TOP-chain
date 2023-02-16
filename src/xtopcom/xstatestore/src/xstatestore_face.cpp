// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatestore/xstatestore_face.h"

#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xverifier/xverifier_errors.h"

#include <cassert>

NS_BEG2(top, statestore)

static data::system_contract::xreg_node_info get_reg_info(common::xaccount_address_t const & node_addr) {
    std::string value_str;
    int const ret =
        xstatestore_hub_t::instance()->map_get(rec_registration_contract_address, data::system_contract::XPORPERTY_CONTRACT_REG_KEY, node_addr.to_string(), value_str);
    if (ret != xsuccess || value_str.empty()) {
        xwarn("[get_reg_info] get node register info fail, node_addr: %s", node_addr.to_string().c_str());
        return data::system_contract::xreg_node_info{};
    }

    data::system_contract::xreg_node_info node_info;
    base::xstream_t stream(base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(value_str.c_str())), static_cast<uint32_t>(value_str.size()));

    node_info.serialize_from(stream);
    return node_info;
}

int32_t verify_standby_transaction(data::xtransaction_t const * trx) {
    assert(trx->get_target_addr() == rec_standby_pool_contract_address.to_string());

    auto const & pub_key = get_reg_info(common::xaccount_address_t{trx->get_source_addr()}).consensus_public_key;
    xdbg("verify_standby_transaction: tx:%s, pub_key(base64):%s", trx->dump().c_str(), pub_key.to_string().c_str());

    auto const check_success = !pub_key.empty() && trx->pub_key_sign_check(pub_key);
    xdbg("verify_standby_transaction: %s .tx:%s, pub_key(base64):%s", check_success ? "success" : "fail", trx->dump().c_str(), pub_key.to_string().c_str());

    return check_success ? xverifier::xverifier_error::xverifier_error_tx_signature_invalid : xverifier::xverifier_error::xverifier_success;
}

NS_END2
