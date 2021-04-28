// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xbasic/xserializable_based_on.h"
#include "xvledger/xdataobj_base.hpp"
#include "xdata/xcons_transaction.h"

NS_BEG2(top, blockmaker)
using data::xcons_transaction_ptr_t;
using data::xcons_transaction_t;

class xunit_proposal_input_t : public xserializable_based_on<void> {
 public:
    xunit_proposal_input_t() = default;
    xunit_proposal_input_t(const std::string & account, uint64_t last_block_height, const std::string & last_block_hash,
        const std::vector<xcons_transaction_ptr_t> & input_txs)
    : m_account(account), m_last_block_height(last_block_height), m_last_block_hash(last_block_hash), m_input_txs(input_txs) {}


    void                set_basic_info(const std::string & account, uint64_t last_block_height, const std::string & last_block_hash) {
        m_account = account;
        m_last_block_height = last_block_height;
        m_last_block_hash = last_block_hash;
    }
    void                set_input_txs(const std::vector<xcons_transaction_ptr_t> & input_txs) {
        m_input_txs = input_txs;
    }

    const std::string & get_account() const {return m_account;}
    const std::string & get_last_block_hash() const {return m_last_block_hash;}
    uint64_t            get_last_block_height() const {return m_last_block_height;}
    const std::vector<xcons_transaction_ptr_t> & get_input_txs() const {return m_input_txs;}
    bool                has_recv_or_confirm_tx() const {
        for (auto & tx : m_input_txs) {
            if (tx->is_recv_tx() || tx->is_confirm_tx()) {
                return true;
            }
        }
        return false;
    }

    std::string         dump() const;

 public:
    int32_t do_write(base::xstream_t & stream) const override;
    std::int32_t do_read(base::xstream_t & stream) override;

 private:
    std::string                             m_account;
    uint64_t                                m_last_block_height{0};
    std::string                             m_last_block_hash;
    std::vector<xcons_transaction_ptr_t>    m_input_txs;
};

class xtableblock_proposal_input_t : public xserializable_based_on<void>, public xenable_to_string_t<xtableblock_proposal_input_t> {
 public:
    xtableblock_proposal_input_t()  = default;
    void    add_unit_input(const xunit_proposal_input_t & input) {
        m_unit_inputs.push_back(input);
    }
    void    clear() {
        m_unit_inputs.clear();
    }
    const std::vector<xunit_proposal_input_t> & get_unit_inputs() const {return m_unit_inputs;}

    std::string to_string() const override;
    int32_t from_string(std::string const & str) override;
    using xenable_to_string_t<xtableblock_proposal_input_t>::to_string;
    using xenable_to_string_t<xtableblock_proposal_input_t>::from_string;

    std::string         dump() const;
    size_t              get_total_txs() const;

 private:
    int32_t do_write(base::xstream_t & stream) const override;
    int32_t do_read(base::xstream_t & stream) override;

 private:
    std::vector<xunit_proposal_input_t>     m_unit_inputs;
};


class xtable_proposal_input_t : public xbase_dataunit_t<xtable_proposal_input_t, xdata_type_table_proposal_input> {
 public:
    xtable_proposal_input_t();
    explicit xtable_proposal_input_t(const std::vector<xcons_transaction_ptr_t> & input_txs);

 protected:
    ~xtable_proposal_input_t() {}
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    bool    delete_fail_tx(const xcons_transaction_ptr_t & input_tx);
    void    set_input_tx(const xcons_transaction_ptr_t & tx);
    void    set_other_account(const std::string & account);
    const std::vector<xcons_transaction_ptr_t> &    get_input_txs() const {return m_input_txs;}
    const std::vector<std::string> &                get_other_accounts() const {return m_other_accounts;}

 private:
    std::vector<xcons_transaction_ptr_t>    m_input_txs;
    std::vector<std::string>                m_other_accounts;  // for empty or full unit accounts
};

using xtable_proposal_input_ptr_t = xobject_ptr_t<xtable_proposal_input_t>;

NS_END2
