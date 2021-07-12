// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xcons_mgr_builder.h"

#include "xblockstore/xblockstore_face.h"
#include "xcertauth/xcertauth_face.h"
#include "xdata/xblocktool.h"
#include "xstore/xstore.h"
#include "xblockmaker/xproposal_maker_mgr.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xunit_service/xcons_service_mgr.h"
#include "xunit_service/xcons_service_para.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xleader_election.h"
#include "xunit_service/xnetwork_proxy.h"
#include "xunit_service/xtableblockservice.h"
#include "xunit_service/xtimer_block_maker.h"
#include "xunit_service/xtimer_dispatcher.h"
#include "xunit_service/xworkpool_dispatcher.h"

NS_BEG2(top, application)

xschnorrcert_t::xschnorrcert_t(const uint32_t total_nodes) {
    m_total_nodes = total_nodes;
}

xschnorrcert_t::~xschnorrcert_t() {}

const std::string xschnorrcert_t::get_signer(const xvip2_t & signer) {
    return base::xstring_utl::tostring(signer.low_addr);
}
// all returned information build into a xvip_t structure
xvip_t xschnorrcert_t::get_validator_addr(const std::string & account_addr) {
    return 0;
}
bool xschnorrcert_t::verify_validator_addr(const base::xvblock_t * test_for_block) {
    return true;
}
bool xschnorrcert_t::verify_validator_addr(const std::string & for_account, const base::xvqcert_t * for_cert) {
    return true;
}

const std::string xschnorrcert_t::do_sign(const xvip2_t & signer, const base::xvqcert_t * sign_for_cert, const uint64_t random_seed) {
    std::string random_seed_string = base::xstring_utl::tostring(base::xtime_utl::get_fast_random64());
    if (random_seed != 0)
        random_seed_string += base::xstring_utl::tostring(random_seed);

    std::string bin;
    ((base::xvqcert_t *)sign_for_cert)->serialize_to_string(bin);
    return bin + random_seed_string;
}

const std::string xschnorrcert_t::do_sign(const xvip2_t & signer, const base::xvblock_t * sign_for_block, const uint64_t random_seed) {
    return do_sign(signer, sign_for_block->get_cert(), random_seed);
}

base::enum_vcert_auth_result xschnorrcert_t::verify_sign(const xvip2_t & signer, const base::xvqcert_t * test_for_cert, const std::string & block_account) {
    return base::enum_vcert_auth_result::enum_successful;
}

base::enum_vcert_auth_result xschnorrcert_t::verify_sign(const xvip2_t & signer, const base::xvblock_t * test_for_block) {
    return base::enum_vcert_auth_result::enum_successful;
}

base::enum_vcert_auth_result xschnorrcert_t::verify_muti_sign(const base::xvqcert_t * test_for_cert, const std::string & block_account) {
    return base::enum_vcert_auth_result::enum_successful;
}

base::enum_vcert_auth_result xschnorrcert_t::verify_muti_sign(const base::xvblock_t * test_for_block) {
    return base::enum_vcert_auth_result::enum_successful;
}

const std::string xschnorrcert_t::merge_muti_sign(const std::vector<xvip2_t> & muti_nodes, const std::vector<std::string> & muti_signatures, const base::xvqcert_t * for_cert) {
    std::string bin;
    ((base::xvqcert_t *)for_cert)->serialize_to_string(bin);
    for (auto const & s : muti_signatures) {
        bin += s;
    }
    return bin;
}

const std::string xschnorrcert_t::merge_muti_sign(const std::map<xvip2_t, std::string, xvip2_compare> & muti_nodes_signatures, const base::xvqcert_t * for_cert) {
    std::string bin;
    ((base::xvqcert_t *)for_cert)->serialize_to_string(bin);
    for (auto const & pair : muti_nodes_signatures) {
        bin += pair.second;
    }
    return bin;
}

const std::string xschnorrcert_t::merge_muti_sign(const std::map<xvip2_t, std::string, xvip2_compare> & muti_nodes_signatures, const base::xvblock_t * for_block) {
    return merge_muti_sign(muti_nodes_signatures, for_block->get_cert());
}

xunit_service::xcons_service_mgr_ptr xcons_mgr_builder::build(std::string const & node_account,
                                                              observer_ptr<store::xstore_face_t> const & store,
                                                              observer_ptr<base::xvblockstore_t> const & blockstore,
                                                              observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                                                              observer_ptr<time::xchain_time_face_t> const & tx_timer,
                                                              xobject_ptr_t<base::xvcertauth_t> const & certauth,
                                                              observer_ptr<election::cache::xdata_accessor_face_t> const & accessor,
                                                              observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                                                              observer_ptr<router::xrouter_face_t> const & router) {
    // xobject_ptr_t<base::xvcertauth_t> certauth;
    // certauth.attach(&auth::xauthcontext_t::instance(*node_service.get()));
    // TODO(justin): remove mock
    // auto certauth = make_object_ptr<xschnorrcert_t>((uint32_t)1);
    auto work_pool = make_object_ptr<base::xworkerpool_t_impl<3>>(top::base::xcontext_t::instance());
    auto xbft_work_pool = make_object_ptr<base::xworkerpool_t_impl<3>>(top::base::xcontext_t::instance());

    auto face = std::make_shared<xunit_service::xelection_cache_imp>();
    std::shared_ptr<xunit_service::xleader_election_face> pelection = std::make_shared<xunit_service::xrotate_leader_election>(blockstore, face);
    std::shared_ptr<xunit_service::xnetwork_proxy_face> network = std::make_shared<xunit_service::xnetwork_proxy>(face, router);

    // global lifecyle
    auto p_res = new xunit_service::xresources(node_account, work_pool, xbft_work_pool, certauth, blockstore, network, pelection, tx_timer, accessor, mbus, txpool);
    auto p_para = new xunit_service::xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type_clock_cert,  // useless parameter
                                                     base::enum_xconsensus_threshold_2_of_3);
    auto p_srv_para = std::make_shared<xunit_service::xcons_service_para>(p_res, p_para);

    auto block_maker = blockmaker::xblockmaker_factory::create_table_proposal(store, make_observer(blockstore.get()), txpool, mbus);
    p_para->add_block_maker(xunit_service::e_table, block_maker);
    p_para->add_block_maker(xunit_service::e_timer, std::make_shared<xunit_service::xtimer_block_maker_t>(p_srv_para));
    return std::make_shared<xunit_service::xcons_service_mgr>(mbus, network, std::make_shared<xdispatcher_builder>(), p_srv_para);
}

xunit_service::xcons_dispatcher_ptr xdispatcher_builder::build(observer_ptr<mbus::xmessage_bus_face_t> const &mb,
            std::shared_ptr<xunit_service::xcons_service_para_face> const & p_srv_para, xunit_service::e_cons_type cons_type) {
    if (cons_type == xunit_service::e_table) {
        auto block_maker = p_srv_para->get_consensus_para()->get_block_maker(cons_type);
        return std::make_shared<xunit_service::xworkpool_dispatcher>(mb, p_srv_para, block_maker);
    } else {
        auto block_maker = p_srv_para->get_consensus_para()->get_block_maker(cons_type);
        return std::make_shared<xunit_service::xtimer_dispatcher_t>(p_srv_para, block_maker);
    }
}

NS_END2
