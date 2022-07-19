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
    xtop_evm_eth_bridge_contract();
    ~xtop_evm_eth_bridge_contract() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;

private:
    bool init(const xbytes_t & rlp_bytes);
    bool sync(const xbytes_t & rlp_bytes);
    bool is_confirmed(const evm_common::u256 height, const xbytes_t & hash_bytes);
    void reset();
    std::set<std::string> load_whitelist();

    bool verify_common(const evm_common::eth::xeth_block_header_t & prev_header, const evm_common::eth::xeth_block_header_t & new_header) const;

    bool get_height(evm_common::bigint & height) const;
    bool set_height(const evm_common::bigint height);
    bool get_hash(const evm_common::bigint height, evm_common::h256 & hash) const;
    bool set_hash(const evm_common::bigint height, const evm_common::h256 hash);
    bool remove_hash(const evm_common::bigint height);
    bool get_header(const evm_common::h256 hash, evm_common::eth::xeth_block_header_t & header, evm_common::bigint & difficulty) const;
    bool set_header(evm_common::eth::xeth_block_header_t & header, evm_common::bigint difficulty);
    bool remove_header(const evm_common::h256 hash);
    bool rebuild(evm_common::eth::xeth_block_header_t & current_header, evm_common::eth::xeth_block_header_t & new_header);
    void release(const evm_common::bigint number);

    const common::xaccount_address_t m_contract_address{evm_eth_bridge_contract_address};
    std::shared_ptr<data::xunit_bstate_t> m_contract_state{nullptr};
    std::set<std::string> m_whitelist;
};
using xevm_eth_bridge_contract_t = xtop_evm_eth_bridge_contract;

NS_END4
