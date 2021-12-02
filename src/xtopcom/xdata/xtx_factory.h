// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtransaction.h"
namespace top { namespace data {

class xtx_factory {
public:
    static xtransaction_ptr_t create_tx(const enum_xtransaction_version tx_version = xtransaction_version_2);
    static xtransaction_ptr_t create_genesis_tx_with_balance(const std::string & account, int64_t top_balance);
    static xtransaction_ptr_t create_genesis_tx_with_sys_contract(const std::string & account);
    static xtransaction_ptr_t create_contract_subtx_transfer(const std::string & sender, 
                                                             const std::string & receiver,
                                                             uint64_t latest_sendtx_nonce,
                                                             const uint256_t & latest_sendtx_hash,
                                                             uint64_t amount,
                                                             uint64_t timestamp);
    static xtransaction_ptr_t create_contract_subtx_call_contract(const std::string & sender, 
                                                             const std::string & receiver,
                                                             uint64_t latest_sendtx_nonce,
                                                             const uint256_t & latest_sendtx_hash,
                                                             const std::string& func_name, 
                                                             const std::string& func_param,
                                                             uint64_t timestamp);
    static xtransaction_ptr_t create_sys_contract_call_self_tx(const std::string & account, 
                                                             const uint64_t latest_sendtx_nonce,
                                                             const uint256_t & latest_sendtx_hash,
                                                             const std::string& func_name, 
                                                             const std::string& func_param,
                                                             const uint64_t timestamp,
                                                             const uint16_t expire_duration);
    static xtransaction_ptr_t create_nodejoin_tx(const std::string & sender, 
                                                 const uint64_t & latest_sendtx_nonce,
                                                 const uint64_t & latest_sendtx_hash,
                                                 const std::string & func_param,
                                                 const uint32_t deposit,
                                                 const std::string & sign_key);

};

}  // namespace data
}  // namespace top
