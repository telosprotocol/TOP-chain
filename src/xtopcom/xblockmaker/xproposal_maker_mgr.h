// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xblockmaker/xproposal_maker.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, blockmaker)

class xproposal_maker_mgr : public xunit_service::xblock_maker_face{
 public:
    explicit xproposal_maker_mgr(const xblockmaker_resources_ptr_t & resources)
    : m_resources(resources) {}

 public:
    virtual std::shared_ptr<xunit_service::xproposal_maker_face>   get_proposal_maker(const std::string & account) override;

 private:
    xblockmaker_resources_ptr_t m_resources;
};

class xblockmaker_factory {
 public:
    static std::shared_ptr<xunit_service::xblock_maker_face> create_table_proposal(const observer_ptr<store::xstore_face_t> & store,
                                                                                   const observer_ptr<base::xvblockstore_t> & blockstore,
                                                                                   const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                                                                   const observer_ptr<store::xindexstorehub_t> & indexstore);
};


NS_END2
