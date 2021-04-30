// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xblockmaker/xproposal_maker_mgr.h"
#include "xindexstore/xindexstore_face.h"

NS_BEG2(top, blockmaker)

std::shared_ptr<xunit_service::xproposal_maker_face>   xproposal_maker_mgr::get_proposal_maker(const std::string & account) {
    // TODO(jimmy)
    std::shared_ptr<xunit_service::xproposal_maker_face> proposal_maker = std::make_shared<xproposal_maker_t>(account, m_resources);
    return proposal_maker;
}

std::shared_ptr<xunit_service::xblock_maker_face> xblockmaker_factory::create_table_proposal(const observer_ptr<store::xstore_face_t> & store,
                                                                                            const observer_ptr<base::xvblockstore_t> & blockstore,
                                                                                            const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                                                                            const observer_ptr<store::xindexstorehub_t> & indexstorehub,
                                                                                            const observer_ptr<mbus::xmessage_bus_face_t> & bus) {
    xblockmaker_resources_ptr_t resources = std::make_shared<xblockmaker_resources_impl_t>(store, blockstore, txpool, indexstorehub, bus);
    std::shared_ptr<xunit_service::xblock_maker_face> blockmaker = std::make_shared<xproposal_maker_mgr>(resources);
    return blockmaker;
}

NS_END2
