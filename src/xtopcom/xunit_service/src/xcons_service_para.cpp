// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xcons_service_para.h"
NS_BEG2(top, xunit_service)

xresources::xresources(const std::string & account,
                       const xobject_ptr_t<base::xworkerpool_t> & pwork,
                       const xobject_ptr_t<base::xworkerpool_t> & xbft_pwork,
                       const xobject_ptr_t<base::xvcertauth_t> & auth,
                       const observer_ptr<base::xvblockstore_t> & blockstore,
                       const std::shared_ptr<xnetwork_proxy_face> & network,
                       const std::shared_ptr<xleader_election_face> & elect_face,
                       observer_ptr<time::xchain_time_face_t> const & timer,
                       observer_ptr<election::cache::xdata_accessor_face_t> const & accessor,
                       observer_ptr<mbus::xmessage_bus_face_t> const & mb,
                       const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                       const observer_ptr<state_sync::xstate_downloader_t> & downloader)
  : m_worker_pool(pwork)
  , m_xbft_worker_pool(xbft_pwork)
  , m_network(network)
  , m_certauth(auth)
  , m_blockstore(blockstore)
  , m_election(elect_face)
  , m_account(account)
  , m_timer(timer)
  , m_accessor(accessor)
  , m_bus(mb)
  , m_txpool(txpool)
  , m_downloader(downloader) {}
xresources::~xresources() {}

// certificate auth face
base::xvcertauth_t * xresources::get_certauth() {
    return m_certauth.get();
}

// work pool
base::xworkerpool_t * xresources::get_workpool() {
    return m_worker_pool.get();
}

// xbft work pool
base::xworkerpool_t * xresources::get_xbft_workpool() {
    return m_xbft_worker_pool.get();
}

// network face
xnetwork_proxy_face * xresources::get_network() {
    return m_network.get();
}

// node account
const std::string & xresources::get_account() {
    return m_account;
}

// block store
base::xvblockstore_t * xresources::get_vblockstore() {
    return m_blockstore.get();
}

// election face
xleader_election_face * xresources::get_election() {
    return m_election.get();
}

// chain timer face
time::xchain_time_face_t * xresources::get_chain_timer() {
    return m_timer.get();
}

// elect data accessor face
election::cache::xdata_accessor_face_t * xresources::get_data_accessor() {
    return m_accessor.get();
}

mbus::xmessage_bus_face_t * xresources::get_bus() {
    return m_bus.get();
}

xtxpool_v2::xtxpool_face_t * xresources::get_txpool() {
    return m_txpool.get();
}

state_sync::xstate_downloader_t * xresources::get_state_downloader() {
    return m_downloader.get();
}

// get pacemaker type
xconsensus::enum_xconsensus_pacemaker_type xconsensus_para::get_pacemaker_type() {
    return m_pacemaker;
}

// get algorithm type
// xconsensus::enum_xconsensus_algorithm_type xconsensus_para::get_algorithm_type() {
//    return m_algtype;
//}
// get consensus threshold
base::enum_xconsensus_threshold xconsensus_para::get_threshold() {
    return m_threshold;
}

xconsensus_para::xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type pacemaker,
                                 // xconsensus::enum_xconsensus_algorithm_type alg,
                                 base::enum_xconsensus_threshold threshold)
  : m_pacemaker(pacemaker), m_threshold(threshold) {}

xconsensus_para::~xconsensus_para() {}

// add block maker
void xconsensus_para::add_block_maker(e_cons_type cons_type, const xblock_maker_ptr & block_maker) {
    m_makers[cons_type] = block_maker;
}

// get block maker
xblock_maker_ptr xconsensus_para::get_block_maker(e_cons_type cons_type) {
    return m_makers[cons_type];
}

// block service create parameter & depends resources
xcons_service_para::xcons_service_para(xresources_face * p_res, xconsensus_para_face * p_para) : m_res(p_res), m_para(p_para) {}

xcons_service_para::~xcons_service_para() {}

// get system resources
xresources_face * xcons_service_para::get_resources() {
    return m_res.get();
}

// get consensus para
xconsensus_para_face * xcons_service_para::get_consensus_para() {
    return m_para.get();
}

NS_END2
