// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xnetwork_proxy.h"

#include "xunit_service/xcons_utl.h"
#include "xvnetwork/xvnetwork_error2.h"

#include <cinttypes>

NS_BEG2(top, xunit_service)

xnetwork_proxy::xnetwork_proxy(const std::shared_ptr<xelection_cache_face> & face, observer_ptr<router::xrouter_face_t> const & router) : m_elect_face(face), m_router(router)  {}

// network proxy, just send msg according by to address
bool xnetwork_proxy::send_out(uint32_t msg_type, const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {
    base::xstream_t stream(base::xcontext_t::instance());
    int32_t ret = const_cast<base::xcspdu_t *>(&packet)->serialize_to(stream);
    if (ret <= 0) {
        xerror("xnetwork_proxy::send_out serialize fail");
        return false;
    }

    auto pdu_type = packet.get_msg_type();
    common::xmessage_id_t id = static_cast<common::xmessage_id_t>(msg_type + pdu_type);

    xunit_dbg("[xunitservice] network sendout message.packet=%s,msg_size=%d,%" PRIx32 " from:%s to:%s",
         packet.dump().c_str(),
         stream.size(),
         static_cast<uint32_t>(id),
         xcons_utl::xip_to_hex(from_addr).c_str(),
         xcons_utl::xip_to_hex(to_addr).c_str());

    return send_out(id, from_addr, to_addr, stream, packet.get_block_account());
}

bool xnetwork_proxy::send_out(common::xmessage_id_t const & id, const xvip2_t & from_addr, const xvip2_t & to_addr, base::xvblock_t * block) {
    base::xstream_t stream(base::xcontext_t::instance());
    data::xblock_t * block_ptr = dynamic_cast<data::xblock_t *>(block);
    xassert(block_ptr != nullptr);
    block_ptr->full_block_serialize_to(stream);
    return send_out(id, from_addr, to_addr, stream, block->get_account());
}

bool xnetwork_proxy::send_out(common::xmessage_id_t const & id, const xvip2_t & from_addr, const xvip2_t & to_addr, base::xstream_t & stream, const std::string & account) {
    auto network = find(from_addr);
    if (network == nullptr) {
        return false;
    }

    top::vnetwork::xmessage_t msg({stream.data(), stream.data() + stream.size()}, id);

    std::error_code ec = vnetwork::xvnetwork_errc2_t::success;
    common::xip2_t dst{to_addr.low_addr, to_addr.high_addr};
    assert(dst.raw_high_part() == to_addr.high_addr);
    assert(dst.raw_low_part() == to_addr.low_addr);
    // validator forward to auditor
    if ((common::has<common::xnode_type_t::consensus_validator>(network->type()) || common::has<common::xnode_type_t::evm_validator>(network->type())) &&
        common::broadcast(dst.slot_id())) {
        auto to = network->parent_group_address();
        bool forward = false;
        if (!common::broadcast(to.network_id()) && !common::broadcast(to.zone_id()) && !common::broadcast(to.cluster_id()) && !common::broadcast(to.group_id())) {
            if (common::broadcast(to.slot_id())) {
                if (common::xauditor_group_id_begin <= to.group_id() && to.group_id() < common::xauditor_group_id_end) {
                    //network->forward_broadcast_message(msg, to);
                    std::error_code broadcast_ec;
                    network->broadcast(to.xip2(), msg, broadcast_ec);
                    if (broadcast_ec) {
                        xwarn("validator forward to auditor failed. src %s dst %s", network->address().to_string().c_str(), to.to_string().c_str());
                    }
                    forward = true;
#ifdef DEBUG
                    xunit_dbg("[xunitservice] network forward from %s to %#016" PRIx64 ".%016" PRIx64, network->address().to_string().c_str(), to_addr.low_addr, to_addr.high_addr);
#endif
                }
            }
        }

        if (!forward) {
            xunit_warn("[xunitservice] network forward from %s to %s failed" PRIx64, network->address().to_string().c_str(), to.to_string().c_str());
        }
    }

    // auditor forward to validator
    if ((common::has<common::xnode_type_t::consensus_auditor>(network->type()) || common::has<common::xnode_type_t::evm_auditor>(network->type())) &&
        common::broadcast(dst.slot_id())) {
        auto group_id = xcons_utl::get_groupid_by_account(from_addr, account);
        xelection_cache_face::elect_set elect_set;
        m_elect_face->get_group_election(from_addr, group_id, &elect_set);
        if (!elect_set.empty()) {
            auto to = static_cast<xvip2_t>(elect_set.front().address.xip2());
            reset_node_id_to_xip2(to);
            set_node_id_to_xip2(to, 0x3FF);
            auto dest_to = xcons_utl::to_address(to, network->address().election_round());
            //network->forward_broadcast_message(msg, dest_to);
            std::error_code broadcast_ec;
            network->broadcast(dest_to.xip2(), msg, broadcast_ec);
            if (broadcast_ec) {
                xwarn("auditor forward to validator failed. src %s dst %s", network->address().to_string().c_str(), dest_to.to_string().c_str());
            }
#ifdef DEBUG
            xunit_dbg("[xunitservice] network forward from %s to %#016" PRIx64 ".%016" PRIx64, network->address().to_string().c_str(), to_addr.low_addr, to_addr.high_addr);
#endif
        }
    }

    if (common::broadcast(dst.network_id()) || common::broadcast(dst.zone_id()) || common::broadcast(dst.cluster_id()) || common::broadcast(dst.group_id()) ||
        common::broadcast(dst.slot_id())) {
        network->broadcast(dst, msg, ec);
        if (ec) {
#ifdef DEBUG
            xunit_dbg("[xunitservice] network sendout broadcast failed: from %s to %#016" PRIx64 ".%016" PRIx64 " ec category: %s ec msg: %s",
                 network->address().to_string().c_str(),
                 to_addr.low_addr,
                 to_addr.high_addr,
                 ec.category().name(),
                 ec.message().c_str());
#endif
        } else {
#ifdef DEBUG
            xunit_dbg("[xunitservice] network sendout broadcast successful: from %s to %#016" PRIx64 ".%016" PRIx64 " ec category: %s ec msg: %s",
                 network->address().to_string().c_str(),
                 to_addr.low_addr,
                 to_addr.high_addr,
                 ec.category().name(),
                 ec.message().c_str());
#endif
        }
    } else {
        network->send_to(dst, msg, ec);
        if (ec) {
#ifdef DEBUG
            xunit_dbg("[xunitservice] network sendout send_to failed: from %s to %#016" PRIx64 ".%016" PRIx64 " ec category: %s ec msg: %s",
                 network->address().to_string().c_str(),
                 to_addr.low_addr,
                 to_addr.high_addr,
                 ec.category().name(),
                 ec.message().c_str());
#endif
        } else {
#ifdef DEBUG
            xunit_dbg("[xunitservice] network sendout send_to successful: from %s to %#016" PRIx64 ".%016" PRIx64 " ec category: %s ec msg: %s",
                 network->address().to_string().c_str(),
                 to_addr.low_addr,
                 to_addr.high_addr,
                 ec.category().name(),
                 ec.message().c_str());
#endif
        }
    }

    return !ec;
}

std::shared_ptr<vnetwork::xvnetwork_driver_face_t> xnetwork_proxy::find(xvip2_t const & from_addr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto iter = m_networks.find(from_addr);
    if (iter != m_networks.end()) {
        return iter->second;
    }
    return nullptr;
}

// listen network message, call while vnode start
bool xnetwork_proxy::listen(const xvip2_t & addr, common::xmessage_category_t category, const xpdu_reactor_ptr & reactor) {
    // auto addr = reactor->get_ip();
    // auto category = reactor->get_category();
    xkinfo("[xunitservice] network listen %s msg:%x %p", xcons_utl::xip_to_hex(addr).c_str(), category, this);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_networks.find(addr);
        if (iter != m_networks.end()) {
            auto network = iter->second.get();

            // register virtual network message callback
            network->register_message_ready_notify(category, std::bind(&xnetwork_proxy::on_message, this, std::placeholders::_1, network->address(), std::placeholders::_2));
            // add bridge call back
            auto listen_iter = m_reactors.find(addr);
            if (listen_iter == m_reactors.end()) {
                // build category to callback map
                std::map<common::xmessage_category_t, xpdu_reactor_ptr> cb_map;
                cb_map[category] = reactor;
                m_reactors[addr] = cb_map;
            } else {
                // add category to callback map
                auto & cb_map = listen_iter->second;
                cb_map[category] = reactor;
            }
            return true;
        } else {
            return false;
        }
    }
}

// unlisten network message, call while vnode outdate (unreg)
bool xnetwork_proxy::unlisten(const xvip2_t & addr, common::xmessage_category_t category) {
    // auto addr = reactor->get_ip();
    // auto category = reactor->get_category();
    xkinfo("[xunitservice] network unlisten %s msg:%x %p", xcons_utl::xip_to_hex(addr).c_str(), category, this);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_networks.find(addr);
        if (iter != m_networks.end()) {
            auto network = iter->second;
            // unregister virtual network message callback
            network->unregister_message_ready_notify(category);
        }
        // erase bridge callback
        auto listen_iter = m_reactors.find(addr);
        if (listen_iter != m_reactors.end()) {
            // add category to callback map
            auto & cb_map = listen_iter->second;
            auto cb_iter = cb_map.find(category);
            if (cb_iter != cb_map.end()) {
                cb_map.erase(cb_iter);
            }
            if (cb_map.empty()) {
                m_reactors.erase(listen_iter);
            }
        }
    }
    return true;
}

// network message callback
void xnetwork_proxy::on_message(top::vnetwork::xvnode_address_t const & sender, top::vnetwork::xvnode_address_t const & receiver, top::vnetwork::xmessage_t const & message) {
    auto to = xcons_utl::to_xip2(receiver);
    auto from = xcons_utl::to_xip2(sender);
    auto category = get_message_category(message.id());
    xpdu_reactor_ptr cb{nullptr};
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto listen_iter = m_reactors.find(to);
        if (listen_iter != m_reactors.end()) {
            auto cb_map = listen_iter->second;
            auto cb_iter = cb_map.find(category);
            if (cb_iter != cb_map.end()) {
                cb = cb_iter->second;
            }
        }
    }

    if (cb != nullptr) {
        base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)(message.payload().data()), (uint32_t)message.payload().size());
        base::xcspdu_t * pdu = new base::xcspdu_t(base::xcspdu_t::enum_xpdu_type_consensus_xbft);
        pdu->serialize_from(stream);
        auto xip_from = xcons_utl::to_xip2(sender);
        cb->on_pdu(xip_from, to, *pdu);
        xunit_info("xnetwork_proxy::on_message succ,category=%x,pdu=%s,at_node:%s %p", category, pdu->dump().c_str(), xcons_utl::xip_to_hex(to).c_str(), &cb);
        pdu->release_ref();
    } else {
        xunit_warn("xnetwork_proxy::on_message fail-no reactor for %" PRIx64 " category %x from: %" PRIx64, to.low_addr, category, from.low_addr);
    }
}

// add networkdriver, call while new vnode build
// add must before listen
bool xnetwork_proxy::add(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & network) {
    auto xip = xcons_utl::to_xip2(network->address(), true);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_networks.find(xip);
        if (iter == m_networks.end()) {
            xunit_info("[xunitservice] network add %s %p", xcons_utl::xip_to_hex(xip).c_str(), network.get());
            m_networks.insert({xip, network});
        } else {
            xunit_info("[xunitservice] network exist %s %p", xcons_utl::xip_to_hex(xip).c_str(), (iter->second).get());
        }
    }
    return true;
}

// erase networkdriver, call before vnode destroy
// erase must after unlisten
bool xnetwork_proxy::erase(const xvip2_t & addr) {
    // auto xip = xcons_utl::to_xip2(network->address());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        assert(m_reactors.find(addr) == m_reactors.end());  // TODO: jimmy fix release compiling error
        // assert(listen_iter == m_reactors.end());
        auto iter = m_networks.find(addr);
        if (iter != m_networks.end()) {
            xunit_info("[xunitservice] network erase %s %p", xcons_utl::xip_to_hex(addr).c_str(), (iter->second).get());
            m_networks.erase(iter);
            return true;
        }

        for (auto iter = m_networks.begin(); iter != m_networks.end(); iter++) {
            auto & network_xip = iter->first;
            common::xip2_t const group_xip2 = common::xip2_t{network_xip}.group_xip2();
            xvip2_t network_group_xip = {group_xip2.raw_low_part(), group_xip2.raw_high_part()};
            if (xcons_utl::xip_equals(addr, network_group_xip)) {
                xunit_info("[xunitservice] network erase %s %p", xcons_utl::xip_to_hex(addr).c_str(), (iter->second).get());
                m_networks.erase(iter);
                return true;
            }
        }

        xunit_info("[xunitservice] network erase %s failed", xcons_utl::xip_to_hex(addr).c_str());
    }
    return false;
}

void xnetwork_proxy::send_receipt_msgs(const xvip2_t & from_addr,
                                       const std::vector<data::xcons_transaction_ptr_t> & receipts,
                                       std::vector<data::xcons_transaction_ptr_t> & non_shard_cross_receipts) {
    auto net_driver = find(from_addr);

    uint32_t recv_tx_num = 0;
    for (auto & receipt : receipts) {
        if (net_driver != nullptr) {
            send_receipt_msg(net_driver, receipt, non_shard_cross_receipts);
        } else {
            xunit_warn("xnetwork_proxy::send_receipt_msgs net_driver not found,can not send receipt:%s addr:%s", receipt->dump().c_str(), xcons_utl::xip_to_hex(from_addr).c_str());
        }

        if (receipt->is_recv_tx()) {
            recv_tx_num++;
        }
    }

    XMETRICS_GAUGE(metrics::txpool_receipt_first_send_succ, (net_driver != nullptr) ? 1 : 0);
}

bool xnetwork_proxy::get_election_round(const xvip2_t & xip, uint64_t & election_round) {
    auto net_driver = find(xip);
    if (net_driver == nullptr) {
        return false;
    }
    election_round = net_driver->address().election_round().value();
    return true;
}

void xnetwork_proxy::send_receipt_msg(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> net_driver,
                                      const data::xcons_transaction_ptr_t & receipt,
                                      std::vector<data::xcons_transaction_ptr_t> & non_shard_cross_receipts) {
    try {
        xassert(receipt->is_recv_tx() || receipt->is_confirm_tx());
        base::xtable_index_t target_tableindex = receipt->get_self_table_index(); // receipt should send to self table

        top::base::xautostream_t<4096> stream(top::base::xcontext_t::instance());
        receipt->serialize_to(stream);
        vnetwork::xmessage_t msg = vnetwork::xmessage_t({stream.data(), stream.data() + stream.size()},
                                                        receipt->is_recv_tx() ? xtxpool_msg_send_receipt : xtxpool_msg_recv_receipt);

        common::xnode_type_t auditor_type = (target_tableindex.get_zone_index() == base::enum_chain_zone_consensus_index) ? common::xnode_type_t::consensus_auditor : common::xnode_type_t::evm_auditor;
        auto auditor_cluster_addr =
            m_router->sharding_address_from_tableindex(target_tableindex, net_driver->network_id(), auditor_type);
        xassert(common::has<common::xnode_type_t::consensus_auditor>(auditor_cluster_addr.type()) || common::has<common::xnode_type_t::committee>(auditor_cluster_addr.type()) ||
                common::has<common::xnode_type_t::zec>(auditor_cluster_addr.type()) || common::has<common::xnode_type_t::evm_auditor>(auditor_cluster_addr.type()) || 
                common::has<common::xnode_type_t::relay>(auditor_cluster_addr.type()));

        if (net_driver->address().cluster_address() == auditor_cluster_addr) {
            xunit_info("xnetwork_proxy::send_receipt_msg broadcast receipt=%s,size=%zu,from_vnode:%s", receipt->dump().c_str(), stream.size(), net_driver->address().to_string().c_str());
            // net_driver->broadcast(msg);
            std::error_code ec;
            net_driver->broadcast(net_driver->address().xip2().group_xip2(), msg, ec);
            if (ec) {
                // todo ?
                xunit_warn("xnetwork_proxy::send_receipt_msg broadcast failed. receipt=%s,size=%zu,from_vnode:%s",
                            receipt->dump().c_str(),
                            stream.size(),
                            net_driver->address().to_string().c_str());
            }
            non_shard_cross_receipts.push_back(receipt);
        } else {
            xunit_info("xnetwork_proxy::send_receipt_msg forward receipt=%s,size=%zu,from_vnode:%s,to_vnode:%s", receipt->dump().c_str(), stream.size(), net_driver->address().to_string().c_str(), auditor_cluster_addr.to_string().c_str());
            //net_driver->forward_broadcast_message(msg, vnetwork::xvnode_address_t{std::move(auditor_cluster_addr)});
            std::error_code ec;
            net_driver->broadcast(common::xnode_address_t{auditor_cluster_addr}.xip2(), msg, ec);
            if (ec) {
                // todo ?
                xunit_warn("xnetwork_proxy::send_receipt_msg forward failed. receipt=%s,size=%zu,from_vnode:%s,to_vnode:%s",
                            receipt->dump().c_str(),
                            stream.size(),
                            net_driver->address().to_string().c_str(),
                            auditor_cluster_addr.to_string().c_str());
            }
        }

        // auditor cluster is different with validator for consensus table
        if (target_tableindex.get_zone_index() == base::enum_chain_zone_consensus_index || target_tableindex.get_zone_index() == base::enum_chain_zone_evm_index) {
            common::xnode_type_t type = (target_tableindex.get_zone_index() == base::enum_chain_zone_consensus_index) ? common::xnode_type_t::consensus_validator : common::xnode_type_t::evm_validator;

            auto validator_cluster_addr =
                m_router->sharding_address_from_tableindex(target_tableindex, net_driver->network_id(), type);
            xassert(common::has<common::xnode_type_t::consensus_validator>(validator_cluster_addr.type()) || common::has<common::xnode_type_t::evm_validator>(validator_cluster_addr.type()) ||
                    common::has<common::xnode_type_t::committee>(validator_cluster_addr.type()) || common::has<common::xnode_type_t::zec>(validator_cluster_addr.type()));

            xassert(validator_cluster_addr != auditor_cluster_addr);
            if (net_driver->address().cluster_address() == validator_cluster_addr) {
                xunit_info("xnetwork_proxy::send_receipt_msg broadcast receipt=%s,size=%zu,from_vnode:%s", receipt->dump().c_str(), stream.size(), net_driver->address().to_string().c_str());
                //net_driver->broadcast(msg);
                std::error_code ec;
                net_driver->broadcast(net_driver->address().xip2().group_xip2(), msg, ec);
                if (ec) {
                    // todo ?
                    xunit_warn("xnetwork_proxy::send_receipt_msg broadcast failed. receipt=%s,size=%zu,from_vnode:%s",
                                receipt->dump().c_str(),
                                stream.size(),
                                net_driver->address().to_string().c_str());
                }
                non_shard_cross_receipts.push_back(receipt);
            } else {
                xunit_info("xnetwork_proxy::send_receipt_msg forward receipt=%s,size=%zu,from_vnode:%s,to_vnode:%s", receipt->dump().c_str(), stream.size(), net_driver->address().to_string().c_str(), validator_cluster_addr.to_string().c_str());
                //net_driver->forward_broadcast_message(msg, vnetwork::xvnode_address_t{std::move(validator_cluster_addr)});
                std::error_code ec;
                net_driver->broadcast(common::xnode_address_t{validator_cluster_addr}.xip2(), msg, ec);
                if (ec) {
                    // todo ?
                    xunit_warn("xnetwork_proxy::send_receipt_msg forward failed. receipt=%s,size=%zu,from_vnode:%s,to_vnode:%s",
                                receipt->dump().c_str(),
                                stream.size(),
                                net_driver->address().to_string().c_str(),
                                validator_cluster_addr.to_string().c_str());
                }
            }
        }
    } catch (top::error::xtop_error_t const & eh) {
        xunit_warn("xnetwork_proxy::send_receipt_msg xvnetwork_error_t exception caught: %s; error code: %d", eh.what(), eh.code().value());
    } catch (const std::exception & eh) {
        xunit_warn("xnetwork_proxy::send_receipt_msg std exception caught: %s;", eh.what());
    }
}

NS_END2
