// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xvledger/xaccountindex.h"
#include "xstate_mpt/xstate_mpt.h"

NS_BEG2(top, statestore)

class xtablestate_ext_t {
 public:
    xtablestate_ext_t(const data::xtablestate_ptr_t & table_state, std::shared_ptr<state_mpt::xstate_mpt_t> const& state_mpt);

    const data::xtablestate_ptr_t &                      get_table_state() const {return m_table_state;}
    std::shared_ptr<state_mpt::xstate_mpt_t> const&    get_state_mpt() const {return m_state_mpt;}
    void get_accountindex(std::string const & unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const;

 private:
    data::xtablestate_ptr_t                     m_table_state{nullptr};
    std::shared_ptr<state_mpt::xstate_mpt_t>  m_state_mpt{nullptr};
};

using xtablestate_ext_ptr_t = std::shared_ptr<xtablestate_ext_t>;


class xtablestate_store_t {
 public:
    xtablestate_store_t(const data::xtablestate_ptr_t & table_state, 
                                std::shared_ptr<state_mpt::xstate_mpt_t> const& state_mpt, 
                                evm_common::xh256_t const & state_root,
                                std::vector<std::pair<data::xunitstate_ptr_t, std::string>> const& unitstates);

    const data::xtablestate_ptr_t &                      get_table_state() const {return m_table_state;}
    std::shared_ptr<state_mpt::xstate_mpt_t> const&    get_state_mpt() const {return m_state_mpt;}
    evm_common::xh256_t const&                                    get_state_root() const {return m_state_root;}
    std::vector<std::pair<data::xunitstate_ptr_t, std::string>> const& get_unitstates() const {return m_unitstates;}

 private:
    data::xtablestate_ptr_t                     m_table_state{nullptr};
    std::shared_ptr<state_mpt::xstate_mpt_t>  m_state_mpt{nullptr};
    evm_common::xh256_t m_state_root;
    std::vector<std::pair<data::xunitstate_ptr_t, std::string>>  m_unitstates;
};

using xtablestate_store_ptr_t = std::shared_ptr<xtablestate_store_t>;


NS_END2
