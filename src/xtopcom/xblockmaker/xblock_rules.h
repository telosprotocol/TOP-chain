// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xblockmaker/xblockmaker_face.h"

NS_BEG2(top, blockmaker)

class xblock_rules final : public xblock_rules_face_t{
 public:
    xblock_rules(const xblockmaker_resources_ptr_t & resources)
    : m_resources(resources) {}
    virtual ~xblock_rules() {}

 public:
    virtual bool    unit_rules_filter(const xblock_ptr_t & rules_end_block,
                                            const xaccount_ptr_t & rules_end_state,
                                            const std::vector<xcons_transaction_ptr_t> & origin_txs,
                                            std::vector<xcons_transaction_ptr_t> & valid_txs,
                                            std::vector<xcons_transaction_ptr_t> & pop_txs) override;

 protected:
    bool            check_rule_txs_one_type(const std::vector<xcons_transaction_ptr_t> & txs) const;
    bool            check_rule_sendtx_duplication(const xaccount_ptr_t & rules_end_state,
                                                 const std::vector<xcons_transaction_ptr_t> & txs,
                                                          std::vector<xcons_transaction_ptr_t> & valid_txs,
                                                          std::vector<xcons_transaction_ptr_t> & pop_txs) const;
    void            check_rule_batch_txs(const std::vector<xcons_transaction_ptr_t> & txs,
                                            std::vector<xcons_transaction_ptr_t> & valid_txs,
                                            std::vector<xcons_transaction_ptr_t> & pop_txs) const;

 private:
    xblockmaker_resources_ptr_t     m_resources;
};

NS_END2
