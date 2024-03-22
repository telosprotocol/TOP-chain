// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xcommon/rlp.h"
#include "xevm_common/xcrosschain/xeth2.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_crosschain_contract_face.h"
#include "xvledger/xmerkle.hpp"

NS_BEG4(top, contract_runtime, evm, sys_contract)

class xtop_evm_eth2_client_contract : public xtop_evm_syscontract_face {
public:
    xtop_evm_eth2_client_contract();
    xtop_evm_eth2_client_contract(evm_common::eth2::xnetwork_id_t version);
    ~xtop_evm_eth2_client_contract() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;

    bool init(state_ptr const & state, evm_common::eth2::xinit_input_t const & init_input, common::xeth_address_t const & sender);
    bool initialized(state_ptr const & state) const;
    uint64_t last_block_number(state_ptr const & state) const;
    h256 block_hash_safe(state_ptr const & state, uint64_t block_number) const;
    bool is_confirmed(state_ptr state, uint64_t number, h256 const & hash_bytes) const;
    bool is_known_execution_header(state_ptr const & state, uint64_t height) const;
    h256 finalized_beacon_block_root(state_ptr const & state) const;
    uint64_t finalized_beacon_block_slot(state_ptr const & state) const;
    evm_common::eth2::xextended_beacon_block_header_t finalized_beacon_block_header(state_ptr const & state) const;
    evm_common::eth2::xlight_client_state_t get_light_client_state(state_ptr const & state) const;
    bool submit_beacon_chain_light_client_update(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update);
    bool submit_execution_header(state_ptr const & state, evm_common::xeth_header_t const & block_header, common::xeth_address_t const & sender);
    bool reset(state_ptr state);
    bool disable_reset(state_ptr state);
    evm_common::eth2::xclient_mode_t get_client_mode(state_ptr state) const;
    uint64_t get_unfinalized_tail_block_number(state_ptr state) const;

private:
    // impl
    bool is_light_client_update_allowed(state_ptr state) const;
    std::set<std::string> load_whitelist();
    bool validate_light_client_update(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update);
    bool verify_finality_branch(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update, uint64_t const finalized_period) const;
    bool verify_bls_signatures(state_ptr const & state,
                               evm_common::eth2::xlight_client_update_t const & update,
                               xbitset_t<evm_common::eth2::SYNC_COMMITTEE_BITS_SIZE> const & sync_committee_bits,
                               uint64_t const finalized_period);
    // bool update_finalized_header(state_ptr const & state, evm_common::eth2::xextended_beacon_block_header_t const & finalized_header);
    bool commit_light_client_update(state_ptr const & state, evm_common::eth2::xlight_client_update_t const & update);
    void gc_finalized_execution_blocks(state_ptr const & state, uint64_t number);

    // properties
    h256 get_finalized_execution_blocks(state_ptr const & state, uint64_t height) const;
    bool add_finalized_execution_blocks(state_ptr const & state, uint64_t height, h256 const & hash);
    bool del_finalized_execution_blocks(state_ptr const & state, uint64_t height);
    static evm_common::eth2::xextended_beacon_block_header_t get_finalized_beacon_header(state_ptr const & state);
    bool set_finalized_beacon_header(state_ptr const & state, evm_common::eth2::xextended_beacon_block_header_t const & beacon);
    evm_common::eth2::xexecution_header_info_t get_finalized_execution_header(state_ptr const & state) const;
    bool set_finalized_execution_header(state_ptr const & state, evm_common::eth2::xexecution_header_info_t const & info);
    bool set_finalized_execution_header_bytes(state_ptr const & state, std::string const & bytes);
    evm_common::eth2::xsync_committee_t get_current_sync_committee(state_ptr const & state) const;
    bool set_current_sync_committee(state_ptr const & state, evm_common::eth2::xsync_committee_t const & committee);
    evm_common::eth2::xsync_committee_t get_next_sync_committee(state_ptr const & state) const;
    bool set_next_sync_committee(state_ptr const & state, evm_common::eth2::xsync_committee_t const & committee);
    int get_flag(state_ptr state) const;
    bool set_flag(state_ptr state);
    bool client_mode(state_ptr state, evm_common::eth2::xclient_mode_t mode);
    evm_common::eth2::xexecution_header_info_t get_unfinalized_tail_execution_header_info(state_ptr const & state) const;
    evm_common::eth2::xexecution_header_info_t get_unfinalized_head_execution_header_info(state_ptr const & state) const;
    uint64_t get_diff_between_unfinalized_head_and_tail(state_ptr state) const;
    bool reset_unfinalized_tail_execution_header(state_ptr const & state);
    bool reset_unfinalized_head_execution_header(state_ptr const & state);
    bool set_unfinalized_tail_execution_header_info(state_ptr const & state, evm_common::eth2::xexecution_header_info_t const & info);
    bool set_unfinalized_head_execution_header_info(state_ptr const & state, evm_common::eth2::xexecution_header_info_t const & info);

    bool validate_beacon_block_header_update(evm_common::eth2::xnetwork_config_t const & config, evm_common::eth2::xheader_update_t const & header_update) const;

    int32_t create_client_mode_property_if_necessary(state_ptr state);
    int32_t create_unfinalized_head_execution_header_property_if_necessary(state_ptr state);
    int32_t create_unfinalized_tail_execution_header_property_if_necessary(state_ptr state);

    evm_common::eth2::xnetwork_id_t m_network;
    std::set<std::string> m_whitelist;
};
using xevm_eth2_client_contract_t = xtop_evm_eth2_client_contract;

NS_END4
