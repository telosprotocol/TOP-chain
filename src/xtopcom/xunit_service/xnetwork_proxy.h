// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xbase.h"
#include "xbase/xthread.h"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_utl.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xrouter/xrouter_face.h"
#include <map>

NS_BEG2(top, xunit_service)

// network proxy, bridge for network and consensus service
class xnetwork_proxy : public xnetwork_proxy_face {
public:
    explicit xnetwork_proxy(const std::shared_ptr<xelection_cache_face> & face, observer_ptr<router::xrouter_face_t> const & router);
public:
    // network proxy, just send msg according by address
    virtual bool send_out(uint32_t msg_type, const xvip2_t &from_addr,
                          const xvip2_t &to_addr, const base::xcspdu_t &packet,
                          int32_t cur_thread_id, uint64_t timenow_ms);

    bool send_out(common::xmessage_id_t const &id, const xvip2_t &from_addr, const xvip2_t &to_addr, base::xvblock_t *block) override;

    // listen network message, call while vnode fade in
    bool listen(const xvip2_t &xip, common::xmessage_category_t category, const xpdu_reactor_ptr &reactor) override;

    // unlisten network message, call while vnode fade out
    bool unlisten(const xvip2_t &xip, common::xmessage_category_t category) override;

    // add networkdriver, call while new vnode build
    virtual bool add(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &network);

    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> find(xvip2_t const &from_addr);

    // erase networkdriver, call before vnode destroy
    virtual bool erase(const xvip2_t &addr);

    void send_receipt_msgs(const xvip2_t & from_addr, const std::vector<data::xcons_transaction_ptr_t> & receipts, std::vector<data::xcons_transaction_ptr_t> & non_shard_cross_receipts) override;


protected:
    // network message callback
    void on_message(top::vnetwork::xvnode_address_t const &sender,
                    top::vnetwork::xvnode_address_t const &receiver,
                    top::vnetwork::xmessage_t const &      message);

    bool send_out(common::xmessage_id_t const &id, const xvip2_t &from_addr, const xvip2_t &to_addr, base::xstream_t &stream, const std::string & account);
    void send_receipt_msg(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> net_driver,
                          const data::xcons_transaction_ptr_t & receipt,
                          std::vector<data::xcons_transaction_ptr_t> & non_shard_cross_receipts);

    // private:
    //     virtual vnetwork::xvnetwork_driver_face_t* find_network(const xvip2_t& xip);
protected:
    std::map<xvip2_t, std::shared_ptr<vnetwork::xvnetwork_driver_face_t>, xvip2_compare>      m_networks;
    std::map<xvip2_t, std::map<common::xmessage_category_t, xpdu_reactor_ptr>, xvip2_compare> m_reactors;
    std::mutex                                                                                m_mutex;
    const std::shared_ptr<xelection_cache_face>                                               m_elect_face;
    observer_ptr<router::xrouter_face_t>                                                      m_router;
};
NS_END2
