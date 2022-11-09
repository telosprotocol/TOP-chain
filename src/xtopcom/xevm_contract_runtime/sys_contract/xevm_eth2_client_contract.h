// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xcrosschain/xeth2.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_crosschain_contract_face.h"
#include "xvledger/xmerkle.hpp"

NS_BEG4(top, contract_runtime, evm, sys_contract)

enum class xeth2_client_net_t : uint8_t {
    eth2_net_mainnet = 0,
    eth2_net_kiln,
    eth2_net_ropsten,
    eth2_net_goerli,
};

class xtop_evm_eth2_client_contract : public xtop_evm_syscontract_face {
public:
    xtop_evm_eth2_client_contract();
    xtop_evm_eth2_client_contract(xeth2_client_net_t version);
    ~xtop_evm_eth2_client_contract() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;

    bool init(state_ptr const & state, evm_common::eth2::xinit_input_t const & init_input);
    bool initialized(state_ptr const & state);
    uint64_t last_block_number(state_ptr const & state);
    h256 block_hash_safe(state_ptr const & state, uint64_t const block_number);
    bool is_known_execution_header(state_ptr const & state, h256 const & hash);
    h256 finalized_beacon_block_root(state_ptr const & state);
    uint64_t finalized_beacon_block_slot(state_ptr const & state);
    evm_common::eth2::xextended_beacon_block_header_t finalized_beacon_block_header(state_ptr const & state);
    evm_common::eth2::xlight_client_state_t get_light_client_state(state_ptr const & state);
    bool submit_beacon_chain_light_client_update(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update);
    bool submit_execution_header(state_ptr const & state, evm_common::xeth_header_t const & block_header);
    bool reset(state_ptr state);
    bool disable_reset(state_ptr state);

private:
    // impl
    std::set<std::string> load_whitelist();
    bool validate_light_client_update(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update);
    bool verify_finality_branch(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update, uint64_t const finalized_period);
    bool verify_bls_signatures(state_ptr const & state,
                               evm_common::eth2::xlight_client_update_t const & update,
                               std::string const & sync_committee_bits,
                               uint64_t const finalized_period);
    bool update_finalized_header(state_ptr const & state, evm_common::eth2::xextended_beacon_block_header_t const & finalized_header);
    bool commit_light_client_update(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update);
    void release_finalized_execution_blocks(state_ptr const & state, uint64_t number);

    // properties
    h256 get_finalized_execution_blocks(state_ptr const & state, uint64_t const height);
    bool set_finalized_execution_blocks(state_ptr const & state, uint64_t const height, h256 const & hash);
    bool del_finalized_execution_blocks(state_ptr const & state, uint64_t const height);
    evm_common::eth2::xexecution_header_info_t get_unfinalized_headers(state_ptr const & state, h256 const & hash);
    bool set_unfinalized_headers(state_ptr const & state, h256 const & hash, evm_common::eth2::xexecution_header_info_t const & info);
    bool del_unfinalized_headers(state_ptr const & state, h256 const & hash);
    evm_common::eth2::xextended_beacon_block_header_t get_finalized_beacon_header(state_ptr const & state);
    bool set_finalized_beacon_header(state_ptr const & state, evm_common::eth2::xextended_beacon_block_header_t const & beacon);
    evm_common::eth2::xexecution_header_info_t get_finalized_execution_header(state_ptr const & state);
    bool set_finalized_execution_header(state_ptr const & state, evm_common::eth2::xexecution_header_info_t const & info);
    evm_common::eth2::xsync_committee_t get_current_sync_committee(state_ptr const & state);
    bool set_current_sync_committee(state_ptr const & state, evm_common::eth2::xsync_committee_t const & committee);
    evm_common::eth2::xsync_committee_t get_next_sync_committee(state_ptr const & state);
    bool set_next_sync_committee(state_ptr const & state, evm_common::eth2::xsync_committee_t const & committee);
    int get_flag(state_ptr state) const;
    bool set_flag(state_ptr state);

    xeth2_client_net_t m_network;
    std::set<std::string> m_whitelist;
};
using xevm_eth2_client_contract_t = xtop_evm_eth2_client_contract;

NS_END4
