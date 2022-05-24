// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

class xtop_evm_eth_bridge_contract : public xevm_syscontract_face_t {
public:
    xtop_evm_eth_bridge_contract() = default;
    ~xtop_evm_eth_bridge_contract() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;
    bool init(std::string headerContent, std::string emitter);
    bool sync(std::string headerContent);

private:

    evm_common::bigint calcBaseFee(evm_common::eth::xeth_block_header_t &parentHeader);
    bool validateOwner(std::string owner);
    bool verifyCommon(evm_common::eth::xeth_block_header_t prev_header, evm_common::eth::xeth_block_header_t new_header);
    bool verifyEip1559Header(evm_common::eth::xeth_block_header_t & parentHeader, evm_common::eth::xeth_block_header_t & header);
    bool verifyGaslimit(evm_common::u256 parentGasLimit, evm_common::u256 headerGasLimit);
    bool isLondon(evm_common::eth::xeth_block_header_t & header) const;
    bool isArrowGlacier(evm_common::eth::xeth_block_header_t &header) const;

    bool get_hash(const uint64_t chain_id, const evm_common::bigint height, evm_common::h256 & hash) const;
    bool set_hash(const uint64_t chain_id, const evm_common::bigint height, const evm_common::h256 hash);
    bool get_height(const uint64_t chain_id, evm_common::bigint & height) const;
    bool set_height(const uint64_t chain_id, const evm_common::bigint height);
    bool get_header(const uint64_t chain_id, const evm_common::h256 hash, evm_common::eth::xeth_block_header_t & header, evm_common::bigint & difficulty) const;
    bool set_header(const uint64_t chain_id, evm_common::eth::xeth_block_header_t header, evm_common::bigint difficulty);
    bool rebuild(uint64_t chain_id, evm_common::eth::xeth_block_header_t & current_header, evm_common::eth::xeth_block_header_t & new_header);

    common::xaccount_address_t m_contract_address{evm_eth_bridge_contract_address};
    std::shared_ptr<data::xunit_bstate_t> m_contract_state{nullptr};
    bool m_initialized{false};
};

NS_END4