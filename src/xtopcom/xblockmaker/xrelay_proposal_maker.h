// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xblockmaker/xrelay_maker.h"
#include "xdata/xblock.h"
#include "xrelay_chain/xrelay_chain_mgr.h"
#include "xunit_service/xcons_face.h"

#include <map>
#include <string>

NS_BEG2(top, blockmaker)

class xrelay_proposal_maker_t : public xunit_service::xproposal_maker_face {
public:
    xrelay_proposal_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, const observer_ptr<xrelay_chain::xrelay_chain_mgr_t> & relay_chain_mgr);
    virtual ~xrelay_proposal_maker_t();

public:
    virtual bool can_make_proposal(xblock_consensus_para_t & proposal_para) override;
    virtual xblock_ptr_t make_proposal(xblock_consensus_para_t & proposal_para, uint32_t min_tx_num) override;
    virtual int verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) override;

protected:
    const std::string & get_account() const {
        return m_relay_maker->get_account();
    }
    data::xtablestate_ptr_t get_target_tablestate(base::xvblock_t * block);
    std::string calc_random_seed(base::xvblock_t * latest_cert_block, uint64_t viewtoken);
    bool leader_set_consensus_para(base::xvblock_t * latest_cert_block, xblock_consensus_para_t & cs_para);
    bool backup_set_consensus_para(base::xvblock_t * latest_cert_block, base::xvblock_t * proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para);

    bool verify_proposal_input(base::xvblock_t * proposal_block, xtablemaker_para_t & table_para);

    bool check_wrap_proposal(const xblock_ptr_t & latest_cert_block, base::xvblock_t * proposal_block); 

private:
    xblockmaker_resources_ptr_t m_resources{nullptr};
    observer_ptr<xrelay_chain::xrelay_chain_mgr_t> m_relay_chain_mgr;
    xrelay_maker_ptr_t m_relay_maker{nullptr};

};

NS_END2
