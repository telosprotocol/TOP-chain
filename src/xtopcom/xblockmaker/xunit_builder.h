// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xvledger/xreceiptid.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xdata/xlightunit.h"
#include "xtxexecutor/xtransaction_executor.h"

NS_BEG2(top, blockmaker)

class xlightunit_builder_para_t : public xblock_builder_para_face_t {
 public:
    xlightunit_builder_para_t(const std::vector<xcons_transaction_ptr_t> & origin_txs, const base::xreceiptid_state_ptr_t & receiptid_state, const xblockmaker_resources_ptr_t & resources)
    : xblock_builder_para_face_t(resources), m_origin_txs(origin_txs), m_receiptid_state(receiptid_state) {}
    virtual ~xlightunit_builder_para_t() {}

    const std::vector<xcons_transaction_ptr_t> &    get_origin_txs() const {return m_origin_txs;}
    const std::vector<xcons_transaction_ptr_t> &    get_fail_txs() const {return m_fail_txs;}
    const std::vector<xcons_transaction_ptr_t> &    get_pack_txs() const {return m_pack_txs;}
    const std::vector<xcons_transaction_ptr_t> &    get_unchange_txs() const {return m_unchange_txs;}

    void                                            set_fail_tx(const xcons_transaction_ptr_t & tx) {m_fail_txs.push_back(tx);}
    void                                            set_fail_txs(const std::vector<xcons_transaction_ptr_t> & txs) {m_fail_txs = txs;}
    void                                            set_pack_txs(const std::vector<xcons_transaction_ptr_t> & txs) {m_pack_txs = txs;}
    void                                            set_unchange_txs(const std::vector<xcons_transaction_ptr_t> & txs) {m_unchange_txs = txs;}
    const base::xreceiptid_state_ptr_t &            get_receiptid_state() const {return m_receiptid_state;}
 private:
    std::vector<xcons_transaction_ptr_t>        m_origin_txs;
    std::vector<xcons_transaction_ptr_t>        m_pack_txs;  // txs included in light-unit
    std::vector<xcons_transaction_ptr_t>        m_fail_txs;
    std::vector<xcons_transaction_ptr_t>        m_unchange_txs;
    base::xreceiptid_state_ptr_t                m_receiptid_state;
};

class xlightunit_builder_t : public xblock_builder_face_t {
 public:
    xlightunit_builder_t();
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para) override;
    static int                  construct_block_builder_para(const data::xblock_ptr_t & prev_block,
                                                             const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                             const data::xblock_consensus_para_t & cs_para,
                                                             xblock_builder_para_ptr_t & build_para,
                                                             txexecutor::xbatch_txs_result_t & exec_result);
 protected:
    xblock_ptr_t create_block(const xblock_ptr_t & prev_block, const data::xblock_consensus_para_t & cs_para, const xlightunit_block_para_t & lightunit_para, const base::xreceiptid_state_ptr_t & receiptid_state);
};

class xfullunit_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para) override;

    std::string                 make_binlog(const base::xauto_ptr<base::xvheader_t> & _temp_header,
                                            const xobject_ptr_t<base::xvbstate_t> & prev_bstate);
};

class xemptyunit_builder_t : public xblock_builder_face_t {
 public:
    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                            const data::xblock_consensus_para_t & cs_para,
                                            xblock_builder_para_ptr_t & build_para) override;
};

NS_END2
