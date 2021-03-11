// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xdata/xblock.h"
#include "xtxexecutor/xtable_maker.h"
#include "xtxexecutor/xblock_maker_para.h"
#include "xtxexecutor/xtxexecutor_face.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, txexecutor)

using xunit_service::xblock_maker_para_t;

class xproposal_maker_t : public xunit_service::xproposal_maker_face {
 public:
    xproposal_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);

 public:
    virtual bool                can_make_proposal(xblock_maker_para_t & proposal_para) override;
    virtual xblock_ptr_t        make_proposal(const xblock_maker_para_t & proposal_para) override;
    virtual int                 verify_proposal(base::xvblock_t* proposal_block, base::xvqcert_t * bind_clock_cert) override;

    bool                        update_txpool_txs(const xblock_maker_para_t & proposal_para, uint64_t timestamp, xtablemaker_para_t & table_para);
 protected:
    const std::string &         get_account() const {return m_table_maker->get_account();}
    base::xvblockstore_t*       get_blockstore() const {return m_table_maker->get_blockstore();}
    store::xstore_face_t*       get_store() const {return m_table_maker->get_store();}
    xtxpool::xtxpool_face_t*    get_txpool() const {return m_table_maker->get_txpool();}

    std::string                 calc_random_seed(base::xvblock_t* latest_cert_block, base::xvqcert_t* drand_cert, uint64_t viewtoken);
    bool                        leader_set_consensus_para(base::xvblock_t* latest_cert_block, const xblock_maker_para_t & blockmaker_para, xblock_consensus_para_t & cs_para);
    bool                        backup_set_consensus_para(base::xvblock_t* latest_cert_block, base::xvblock_t* proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para);

    xblock_ptr_t                verify_proposal_prev_block(base::xvblock_t* proposal, base::xvblock_t* default_latest_cert) const;
    bool                        verify_proposal_drand_block(base::xvblock_t *proposal_block, xblock_ptr_t & drand_block) const;
    bool                        verify_proposal_class(base::xvblock_t *proposal_block) const;
    bool                        verify_proposal_input(base::xvblock_t *proposal_block, xtablemaker_para_t & table_para);
    bool                        verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const;

 private:
    xtable_maker_ptr_t                             m_table_maker{nullptr};
    observer_ptr<xtxpool::xtxpool_table_face_t>    m_txpool_table{nullptr};
    int32_t                                        m_tableblock_batch_tx_num_residue{0};
};

// class xtablechecker_factory {
//  public:
//     static std::shared_ptr<xunit_service::xblock_maker_face> create_table_maker(const observer_ptr<store::xstore_face_t> & store, base::xvblockstore_t* blockstore, xtxpool::xtxpool_face_t* txpool);
// };


NS_END2
