// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xBFT/xconsobj.h"
#include "xbase/xbase.h"
#include "xbase/xthread.h"
#include "xvledger/xvblock.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xobject_ptr.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xmbus/xmessage_bus.h"
#include "xunit_service/xcons_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"

#include <string>
#include <vector>

NS_BEG2(top, xunit_service)

class xresources : public xresources_face {
public:
    xresources(const std::string & account,
               const xobject_ptr_t<base::xworkerpool_t> & pwork,
               const xobject_ptr_t<base::xworkerpool_t> & xbft_pwork,
               const xobject_ptr_t<base::xvcertauth_t> & auth,
               const observer_ptr<base::xvblockstore_t> & blockstore,
               const std::shared_ptr<xnetwork_proxy_face> & network,
               const std::shared_ptr<xleader_election_face> & elect_face,
               observer_ptr<time::xchain_time_face_t> const & tx_timer,
               observer_ptr<election::cache::xdata_accessor_face_t> const & accessor,
               observer_ptr<mbus::xmessage_bus_face_t> const & mb,
               const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool);
    virtual ~xresources();

public:
    // certificate auth face
    virtual base::xvcertauth_t * get_certauth();
    // work pool
    virtual base::xworkerpool_t * get_workpool();
    // work pool
    virtual base::xworkerpool_t * get_xbft_workpool();
    // network face
    virtual xnetwork_proxy_face * get_network();
    // block store
    virtual base::xvblockstore_t * get_vblockstore();
    // election face
    virtual xleader_election_face * get_election();
    // chain timer face
    virtual time::xchain_time_face_t * get_chain_timer();
    // elect data accessor face
    virtual election::cache::xdata_accessor_face_t * get_data_accessor();
    // node account
    virtual const std::string & get_account();
    virtual mbus::xmessage_bus_face_t * get_bus();
    virtual xtxpool_v2::xtxpool_face_t * get_txpool();

private:
    xobject_ptr_t<base::xworkerpool_t> m_worker_pool;
    xobject_ptr_t<base::xworkerpool_t> m_xbft_worker_pool;
    std::shared_ptr<xnetwork_proxy_face> m_network;
    xobject_ptr_t<base::xvcertauth_t> m_certauth;
    observer_ptr<base::xvblockstore_t> m_blockstore;
    std::shared_ptr<xleader_election_face> m_election;
    std::string m_account;
    observer_ptr<time::xchain_time_face_t> m_timer;
    observer_ptr<election::cache::xdata_accessor_face_t> m_accessor;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus{};
    observer_ptr<xtxpool_v2::xtxpool_face_t> m_txpool{};
};

// consensuss parameter
class xconsensus_para : public xconsensus_para_face {
public:
    xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type pacemaker,
                    // xconsensus::enum_xconsensus_algorithm_type alg,
                    base::enum_xconsensus_threshold threshold);
    virtual ~xconsensus_para();

public:
    // get pacemaker type
    virtual xconsensus::enum_xconsensus_pacemaker_type get_pacemaker_type();
    // get algorithm type
    // virtual xconsensus::enum_xconsensus_algorithm_type get_algorithm_type();
    // get consensus threshold
    virtual base::enum_xconsensus_threshold get_threshold();
    // add block maker
    virtual void add_block_maker(e_cons_type cons_type, const xblock_maker_ptr & block_maker);
    // get block maker
    virtual xblock_maker_ptr get_block_maker(e_cons_type cons_type);

private:
    xconsensus::enum_xconsensus_pacemaker_type m_pacemaker;
    base::enum_xconsensus_threshold m_threshold;
    std::map<e_cons_type, xblock_maker_ptr> m_makers;
};

// block service create parameter & depends resources
class xcons_service_para : public xcons_service_para_face {
public:
    xcons_service_para(xresources_face * p_res, xconsensus_para_face * p_para);
    virtual ~xcons_service_para();

public:
    // get system resources
    virtual xresources_face * get_resources();

    // get consensus para
    virtual xconsensus_para_face * get_consensus_para();

private:
    std::shared_ptr<xresources_face> m_res;
    std::shared_ptr<xconsensus_para_face> m_para;
};

NS_END2
