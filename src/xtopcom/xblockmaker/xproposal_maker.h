// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xdata/xblock.h"
#include "xblockmaker/xtable_maker.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, blockmaker)

class xproposal_maker_t : public xunit_service::xproposal_maker_face {
 public:
    xproposal_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xproposal_maker_t();
 public:
    virtual bool                can_make_proposal(xblock_consensus_para_t & proposal_para) override;
    virtual xblock_ptr_t        make_proposal(xblock_consensus_para_t & proposal_para, uint32_t min_tx_num) override;
    virtual int                 verify_proposal(xblock_consensus_para_t & proposal_para, base::xvblock_t* proposal_block, base::xvqcert_t * bind_clock_cert) override;
    virtual void                set_certauth(base::xvcertauth_t* _ca);
    virtual data::xblock_consensus_para_ptr_t   leader_set_consensus_para_basic(base::xvblock_t* _cert_block, uint64_t viewid, uint64_t clock, std::error_code & ec) override;
    virtual bool                account_index_upgrade() override;

    bool                        update_txpool_txs(const xblock_consensus_para_t & proposal_para, xtablemaker_para_t & table_para);
    static std::set<base::xtable_shortid_t> select_peer_sids_for_confirm_id(const std::vector<base::xtable_shortid_t> & all_sid_vec, uint64_t height);
 protected:
    const std::string &         get_account() const {return m_table_maker->get_account();}
    base::xvblockstore_t*       get_blockstore() const {return m_table_maker->get_blockstore();}
    xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_table_maker->get_txpool();}

    std::string                 calc_random_seed(base::xvblock_t* latest_cert_block, base::xvqcert_t* drand_cert, uint64_t viewtoken);
    bool                        leader_set_consensus_para(base::xvblock_t* latest_cert_block, xblock_consensus_para_t & cs_para);
    bool                        backup_set_consensus_para(base::xvblock_t* latest_cert_block, base::xvblock_t* proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para);

    bool                        verify_proposal_drand_block(base::xvblock_t *proposal_block, xblock_ptr_t & drand_block) const;
    bool                        verify_proposal_input(base::xvblock_t *proposal_block, xtablemaker_para_t & table_para);

private:
    data::xtablestate_ptr_t get_target_tablestate(base::xvblock_t * block);
    bool leader_xip_to_leader_address(xvip2_t _xip, common::xaccount_address_t & _addr) const;
    void update_txpool_table_state(base::xvblock_t* _commit_block, base::xvblock_t* _lock_block, base::xvblock_t* _cert_block, data::xtablestate_ptr_t const& commit_tablestate);
    int  backup_verify_and_set_consensus_para_basic(xblock_consensus_para_t & cs_para, base::xvblock_t *proposal_block);

    xblockmaker_resources_ptr_t     m_resources{nullptr};
    xtable_maker_ptr_t          m_table_maker{nullptr};
    int32_t                     m_tableblock_batch_tx_num_residue{0};
    int32_t                     m_max_account_num{0};
};

NS_END2
