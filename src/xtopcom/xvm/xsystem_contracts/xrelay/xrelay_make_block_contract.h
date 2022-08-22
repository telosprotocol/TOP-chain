// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xrelay_block.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, relay)

// relay wrap phase
#define RELAY_WRAP_PHASE_0 "0"
#define RELAY_WRAP_PHASE_1 "1"
#define RELAY_WRAP_PHASE_2 "2"
#define RELAY_WRAP_PHASE_INIT "2"

// properties of relay wrap block
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_NEXT_POLY_BLOCK_LOGIC_TIME = "@0";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME = "@1";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_LAST_HEIGHT = "@2";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_LAST_HASH = "@3";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_LAST_EPOCH_ID = "@4";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_CROSS_TXS = "@5";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST = "@6";
XINLINE_CONSTEXPR const char * XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST = "@7";

class xtop_relay_make_block_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_relay_make_block_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_relay_make_block_contract);

    explicit xtop_relay_make_block_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override {
        return new xtop_relay_make_block_contract(network_id());
    }

    void setup();
    static const std::string block_hash_chainid_to_string(const evm_common::h256 & block_hash, const evm_common::u256 & chain_bits);
    static void block_hash_chainid_from_string(const std::string & str, evm_common::h256 & block_hash, evm_common::u256 & chain_bits);

    BEGIN_CONTRACT_WITH_PARAM(xtop_relay_make_block_contract)
    CONTRACT_FUNCTION_PARAM(xtop_relay_make_block_contract, on_receive_cross_txs);
    CONTRACT_FUNCTION_PARAM(xtop_relay_make_block_contract, on_make_block);
    END_CONTRACT_WITH_PARAM

private:
    void on_receive_cross_txs(std::string const & cross_txs_data);
    void on_make_block(std::string const & make_block_info);
    void proc_created_relay_block(data::xrelay_block & relay_block, uint64_t clock, const evm_common::u256 & chain_bits);
    bool update_wrap_phase(uint64_t last_height);
    bool build_elect_relay_block(const evm_common::h256 & prev_hash, uint64_t block_height, uint64_t clock, const std::string & data);
    bool build_poly_relay_block(const evm_common::h256 & prev_hash, uint64_t block_height, uint64_t clock);
    bool build_tx_relay_block(const evm_common::h256 & prev_hash, uint64_t block_height, uint64_t clock);
    void pop_tx_block_hashs(const string & list_key, bool for_poly_block, std::vector<evm_common::h256> & tx_block_hash_vec, evm_common::u256 & chain_bits);
    void update_next_block_clock_for_a_type(const string & key, uint64_t clock);
};
using xrelay_make_block_contract_t = xtop_relay_make_block_contract;

NS_END4
