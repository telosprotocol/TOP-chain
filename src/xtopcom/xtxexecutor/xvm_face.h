// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xstatectx/xstatectx_face.h"
#include "xevm_common/xevm_transaction_result.h"

NS_BEG2(top, txexecutor)

using data::xcons_transaction_ptr_t;

enum enum_execute_result_type {
    enum_exec_success                   = 0,
    enum_exec_error_nonce_mismatch      = 1,
    enum_exec_error_receiptid_mismatch  = 2,
    enum_exec_error_load_state          = 3,
    enum_exec_error_vm_execute          = 4,
    enum_exec_error_evm_execute         = 5,
    enum_exec_error_property_set        = 6,
    enum_exec_error_state_dirty         = 7,
    enum_exec_error_estimate_gas        = 8,
    enum_exec_error_out_of_gas          = 9,

    enum_exec_error_invalid             = -1,
};

class xvm_para_t {
 public:
    xvm_para_t(uint64_t clock, const std::string & random_seed, uint64_t tgas_lock, uint64_t gas_limit)  // for test
    : m_clock(clock), m_random_seed(random_seed), m_lock_tgas_token(tgas_lock), m_gas_limit(gas_limit) {
    }
    xvm_para_t(uint64_t clock, const std::string & random_seed, uint64_t tgas_lock, uint64_t gas_limit, uint64_t block_height, common::xaccount_address_t const& coinbase)
    : m_clock(clock), m_random_seed(random_seed), m_lock_tgas_token(tgas_lock), m_gas_limit(gas_limit), m_block_height(block_height), m_block_coinbase(coinbase) {
    }

 public:
    uint64_t                get_clock() const {return m_clock;}
    uint64_t                get_timestamp() const {return (uint64_t)(m_clock * 10) + base::TOP_BEGIN_GMTIME;}
    const std::string &     get_random_seed() const {return m_random_seed;}
    uint64_t                get_lock_tgas_token() const {return m_lock_tgas_token;}
    uint64_t                get_gas_limit() const {return m_gas_limit;}
    uint64_t                get_block_height() const {return m_block_height;}
    common::xaccount_address_t const&   get_block_coinbase() const {return m_block_coinbase;}

 private:
    uint64_t        m_clock{0};
    std::string     m_random_seed;
    uint64_t        m_lock_tgas_token{0};
    uint64_t        m_gas_limit{0};
    uint64_t        m_block_height{0};
    common::xaccount_address_t  m_block_coinbase;
};

struct xvm_gasfee_detail_t {
    uint64_t m_state_burn_balance{0};
    uint64_t m_state_lock_balance{0};
    uint64_t m_state_unlock_balance{0};
    uint64_t m_state_used_tgas{0};
    uint64_t m_state_last_time{0};
    uint64_t m_tx_used_tgas{0};
    uint64_t m_tx_used_deposit{0};
    evm_common::u256 m_state_burn_eth_balance{0};

    std::string str() {
        std::stringstream ss;
        ss << "m_state_burn_balance: ";
        ss << m_state_burn_balance;
        ss << ", m_state_burn_eth_balance: ";
        ss << m_state_burn_eth_balance.str();
        ss << ", m_state_lock_balance: ";
        ss << m_state_lock_balance;
        ss << ", m_state_unlock_balance: ";
        ss << m_state_unlock_balance;
        ss << ", m_state_used_tgas: ";
        ss << m_state_used_tgas;
        ss << ", m_state_last_time: ";
        ss << m_state_last_time;
        ss << ", m_tx_used_tgas: ";
        ss << m_tx_used_tgas;
        ss << ", m_tx_used_deposit: ";
        ss << m_tx_used_deposit;
        return ss.str();
    }
};

// struct xtxexecutor_ctx_para_t {
//     xtxexecutor_ctx_para_t(uint64_t clock, const std::string & random_seed, uint64_t tgas_lock)
//     : m_clock(clock), m_random_seed(random_seed), m_total_lock_tgas_token(tgas_lock) {
//     }
//     uint64_t        m_clock{0};
//     std::string     m_random_seed;
//     uint64_t        m_total_lock_tgas_token{0};
//     uint64_t        get_timestamp() const {return (uint64_t)(m_clock * 10) + base::TOP_BEGIN_GMTIME;}
// };

class xvm_input_t {
 public:
    xvm_input_t(const statectx::xstatectx_face_ptr_t & statectx, const xvm_para_t & para, const xcons_transaction_ptr_t & tx)
    : m_statectx(statectx), m_para(para), m_tx(tx) {}

 public:
    const xcons_transaction_ptr_t &     get_tx() const {return m_tx;}
    const xvm_para_t &                  get_para() const {return m_para;}
    const statectx::xstatectx_face_ptr_t &  get_statectx() const {return m_statectx;} 

 private:
    statectx::xstatectx_face_ptr_t  m_statectx;
    xvm_para_t                      m_para;
    xcons_transaction_ptr_t         m_tx;
};

struct xvm_output_t {
    evm_common::xevm_transaction_result_t tx_result;
    std::error_code ec;
    int64_t tgas_balance_change{0};
    std::vector<xcons_transaction_ptr_t> contract_create_txs;
    xvm_gasfee_detail_t gasfee_detail;
};

class xvm_face_t {
public:
    virtual enum_execute_result_type execute(const xvm_input_t & input, xvm_output_t & output) = 0;
};

class xtop_vm_face {
public:
    xtop_vm_face() = default;
    xtop_vm_face(xtop_vm_face const &) = delete;
    xtop_vm_face & operator=(xtop_vm_face const &) = delete;
    xtop_vm_face(xtop_vm_face &&) = default;
    xtop_vm_face & operator=(xtop_vm_face &&) = default;
    virtual ~xtop_vm_face() = default;

    virtual evm_common::xevm_transaction_result_t execute(xvm_input_t const & input, std::error_code & ec) = 0;
};
using xvm_face2_t = xtop_vm_face;

NS_END2
