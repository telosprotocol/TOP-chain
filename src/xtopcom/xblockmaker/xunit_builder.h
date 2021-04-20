// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xblockmaker/xblockmaker_face.h"

NS_BEG2(top, blockmaker)

class xlightunit_builder_para_t : public xblock_builder_para_face_t {
 public:
    xlightunit_builder_para_t(const std::vector<xcons_transaction_ptr_t> & origin_txs, const xblockmaker_resources_ptr_t & resources)
    : xblock_builder_para_face_t(resources), m_origin_txs(origin_txs) {}
    virtual ~xlightunit_builder_para_t() {}

    const std::vector<xcons_transaction_ptr_t> &    get_origin_txs() const {return m_origin_txs;}
    const std::vector<xcons_transaction_ptr_t> &    get_fail_txs() const {return m_fail_txs;}
    void                                            set_fail_tx(const xcons_transaction_ptr_t & tx) {m_fail_txs.push_back(tx);}

 private:
    std::vector<xcons_transaction_ptr_t>        m_origin_txs;
    std::vector<xcons_transaction_ptr_t>        m_fail_txs;
};

class xlightunit_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xaccount_ptr_t & prev_state,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para);
};

class xfullunit_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xaccount_ptr_t & prev_state,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para);
};

class xemptyunit_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xaccount_ptr_t & prev_state,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para);
};

class xtop_lightunit_builder2 : public xblock_builder_face_t {
public:
    xblock_ptr_t build_block(xblock_ptr_t const & prev_block,
                             xaccount_ptr_t const & prev_state,
                             data::xblock_consensus_para_t const & cs_para,
                             xblock_builder_para_ptr_t & build_para) override;
};
using xlightunit_builder2_t = xtop_lightunit_builder2;

NS_END2
