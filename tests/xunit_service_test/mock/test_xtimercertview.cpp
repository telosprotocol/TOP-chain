// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xBFT/src/xtimercertview.h"
#include "xvledger/xvblock.h"
#include "xBFT/xconsaccount.h"
#include "xbase/xthread.h"
#include "xdata/xgenesis_data.h"
#include "xcrypto/xckey.h"
#include "xcertauth/xcertauth_face.h"
#include "xBFT/test/common/xunitblock.hpp"
#include "xdata/xemptyblock.h"
#include "xunit_service/xcons_utl.h"
#include "../../mock/xmock_auth.hpp"
#include "../../mock/xmock_network.hpp"

using namespace top;
using namespace top::data;
using namespace top::base;
using namespace top::xconsensus;
using namespace top::mock;

class test_xtimercertview_t : public xconspacemaker_t {
public:
    test_xtimercertview_t(xcscoreobj_t & parent_object, xvcertauth_t *cert_auth, xvblockstore_t* bs)
        : xconspacemaker_t(parent_object) {
        xcspacemaker_t::register_plugin(cert_auth);
        xcspacemaker_t::register_plugin(bs);
    }

    bool reset_xip_addr(const xvip2_t & new_addr) {
        return xconspacemaker_t::reset_xip_addr(new_addr);
    }

    virtual uint64_t get_gmt_clock_second() const {
        uint64_t clock = xconspacemaker_t::get_gmt_clock_second();
        return uint64_t((int64_t)clock + m_shift);
    }

    void set_clock_shift(int64_t shift) {
        m_shift = shift;
    }

private:
    int64_t m_shift{};
};

using xmock_tc_dispatch_callback = std::function<void(const xvip2_t &, const base::xvevent_t &)>;

class test_parent_obj_t : public xcsaccount_t {
public:
    test_parent_obj_t(xcontext_t & _context,const int32_t target_thread_id,const std::string & account_addr, const xvip2_t &xip, xmock_tc_dispatch_callback dispatch_cb)
        : xcsaccount_t(_context, target_thread_id, account_addr), m_xip(xip), m_dispatch_cb(dispatch_cb) {}

    void set_child(test_xtimercertview_t* child) {
        m_child = child;
    }

    bool on_pdu_event_up(const base::xvevent_t & event, xcsobject_t * from_child, const int32_t cur_thread_id, const uint64_t timenow_ms) {

        m_dispatch_cb(m_xip, event);
        return false;
    }

    bool recv_in(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) {

        base::xauto_ptr<xcspdu_fire> _event_obj(new xcspdu_fire(packet));
        _event_obj->set_from_xip(from_addr);
        _event_obj->set_to_xip(to_addr);

        return m_child->on_pdu_event_down(*_event_obj, this, cur_thread_id, timenow_ms);
    }

    bool on_create_block_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override {
        xconsensus::xcscreate_block_evt const & e = (xconsensus::xcscreate_block_evt const &)event;
        auto clock = e.get_clock();
        auto context_id = e.get_context_id();

        base::xvblock_t * _block = data::xemptyblock_t::create_emptyblock(sys_contract_beacon_timer_addr, clock, base::enum_xvblock_level_root, clock, clock, base::enum_xvblock_type_clock);
        _block->get_cert()->set_viewtoken(-1);
        _block->get_cert()->set_gmtime(-1);
        _block->get_cert()->set_nonce(-1);

        base::xauto_ptr<xconsensus::xcscreate_block_evt> _event_obj(new xconsensus::xcscreate_block_evt(e.get_xip(), e.get_vote(), _block, context_id));
        m_child->push_event_down(*_event_obj, this, 0, 0);
        _block->release_ref();

        return true;
    }

    bool on_time_cert_event(const base::xvevent_t & event,xcsobject_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override {
        printf("on_time_cert_event\n");
        return true;
    }

private:
    xvip2_t m_xip;
    xmock_tc_dispatch_callback m_dispatch_cb{};
    test_xtimercertview_t* m_child{};
};

class xmock_tc_thread_event_para_t : public top::base::xobject_t {
public:
    xmock_tc_thread_event_para_t(test_parent_obj_t *node_obj, const xvip2_t &from_addr, const xvip2_t &to_addr, base::xcspdu_t &packet, int32_t cur_thread_id, uint64_t timenow_ms):
    m_node_obj(node_obj),
    m_from_addr(from_addr),
    m_to_addr(to_addr),
    m_packet(packet),
    m_cur_thread_id(cur_thread_id),
    m_timenow_ms(timenow_ms) {
    }
private:
    ~xmock_tc_thread_event_para_t() override {}

public:
    test_parent_obj_t *m_node_obj{};
    xvip2_t m_from_addr;
    xvip2_t m_to_addr;
    base::xcspdu_t m_packet;
    int32_t m_cur_thread_id;
    uint64_t m_timenow_ms;
};

static bool xmock_tc_thread_event(top::base::xcall_t& call, const int32_t thread_id, const uint64_t timenow_ms) {
    xmock_tc_thread_event_para_t* para = dynamic_cast<xmock_tc_thread_event_para_t*>(call.get_param1().get_object());

    test_parent_obj_t *node_obj = para->m_node_obj;

    xvip2_t from_addr = para->m_from_addr;
    xvip2_t to_addr = para->m_to_addr;
    base::xcspdu_t packet = para->m_packet;
    int32_t cur_thread_id = para->m_cur_thread_id;
    uint64_t tm = para->m_timenow_ms;

    node_obj->recv_in(from_addr, to_addr, packet, cur_thread_id, tm);

    //para->release_ref();

    return true;
}

class test_dispatcher_t {
public:

    void add_node(const xvip2_t &xip, test_parent_obj_t *node, top::base::xiothread_t* thread) {
        m_nodes[xip] = node;
        m_node_threads[xip] = thread;
    }

    void dispatch(const xvip2_t &from_addr, const base::xvevent_t & event) {

        for (auto &it: m_nodes) {
            if (!xunit_service::xcons_utl::xip_equals(from_addr, it.first)) {

                xvip2_t xip = it.first;

                top::base::xiothread_t* _thread = m_node_threads[xip];

                xconsensus::xcspdu_fire const& _evt_obj = (xconsensus::xcspdu_fire const&)event;

                test_parent_obj_t *node_obj = it.second;
                base::xbftpdu_t packet = _evt_obj._packet;

                base::xauto_ptr<xmock_tc_thread_event_para_t> para =
                        new xmock_tc_thread_event_para_t(node_obj, from_addr, xip, packet, _thread->get_thread_id(), 0);
                top::base::xparam_t param(para.get());
                top::base::xcall_t tmp_func((top::base::xcallback_ptr)xmock_tc_thread_event, param);
                auto ret = _thread->send_call(tmp_func);

                if (ret != 0) {
                    printf("send call failed %d\n", ret);
                    assert(0);
                }
            }
        }
    }

private:
    std::map<xvip2_t, test_parent_obj_t*, xvip2_compare> m_nodes;
    std::map<xvip2_t, top::base::xiothread_t*, xvip2_compare> m_node_threads;
};

#if 0
class test_ca_mgr_t {
public:
    test_ca_mgr_t(uint32_t nodes) {
        base::xvnodehouse_t* _nodesvr_ptr = new base::xvnodehouse_t();

        xvip2_t _shard_xipaddr = {0};
        _shard_xipaddr.high_addr = (((uint64_t)nodes) << 54) | 1; //encode node'size of group
        _shard_xipaddr.low_addr  = 1 << 10; //at group#1

        for(uint32_t i = 0; i < nodes; ++i)
        {
            xvip2_t node_addr;
            node_addr.high_addr = _shard_xipaddr.high_addr;
            node_addr.low_addr  = _shard_xipaddr.low_addr | i;

            utl::xecprikey_t node_prv_key;
            std::string _node_prv_key_str((const char*)node_prv_key.data(),node_prv_key.size());
            std::string _node_pub_key_str = node_prv_key.get_compress_public_key();
            const std::string node_account  = node_prv_key.to_account_address('0', 0);

            _consensus_nodes.push_back(new base::xvnode_t(node_account,node_addr,_node_pub_key_str,_node_prv_key_str));
        }
        base::xauto_ptr<base::xvnodegroup_t> _consensus_group(new base::xvnodegroup_t(_shard_xipaddr,0,_consensus_nodes));
        _nodesvr_ptr->add_group(_consensus_group.get());
    }

    virtual ~test_ca_mgr_t() {
        _nodesvr_ptr->release_ref();
    }

    xvip2_t addr_at(uint32_t index) {
        return _consensus_nodes[index]->get_xip2_addr();
    }

    base::xvcertauth_t & get_ca() {
        return auth::xauthcontext_t::instance(*_nodesvr_ptr);
    }

    base::xvnodehouse_t* _nodesvr_ptr{};
    std::vector<base::xvnode_t*> _consensus_nodes;
};
#endif

// beacon(4node)
static Json::Value create_timercert() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";

    v["group"]["beacon"]["type"] = "beacon";
    v["group"]["beacon"]["parent"] = "zone0";

    v["node"]["node0"]["parent"] = "beacon";
    v["node"]["node1"]["parent"] = "beacon";
    v["node"]["node2"]["parent"] = "beacon";
    v["node"]["node3"]["parent"] = "beacon";

    return v;
}

void test_xtc() {

    // auto bs = new test::xunitblockstore_t{}; // error: cannot allocate an object of abstract type ‘top::test::xunitblockstore_t’ auto bs = new test::xunitblockstore_t{};

    Json::Value timercert_cfg = create_timercert();

    // 4 nodes
    xmock_network_config_t cfg_network(timercert_cfg);
    xmock_network_t network(cfg_network);
    std::vector<std::shared_ptr<xmock_node_info_t>> nodes = network.get_all_nodes();

    test_dispatcher_t dispatcher;
    std::vector<test_xtimercertview_t*> tcs;
    std::vector<test_parent_obj_t*> parents;
    std::vector<top::base::xiothread_t*> threads;

    for (auto &it: nodes) {

        std::shared_ptr<xmock_node_info_t> &node = it;

        xvip2_t node_addr = node->m_xip;

        auto * _thread = xiothread_t::create_thread(xcontext_t::instance(), xiothread_t::enum_xthread_type_general, -1);

        test_parent_obj_t *parent = new test_parent_obj_t(*_thread->get_context(), _thread->get_thread_id(), sys_contract_beacon_timer_addr, node_addr,
                    std::bind(&test_dispatcher_t::dispatch, &dispatcher, std::placeholders::_1, std::placeholders::_2));
        test_xtimercertview_t *tc = new test_xtimercertview_t(*parent, node->m_certauth.get(), nullptr);

        parent->set_child(tc);

        tcs.push_back(tc);
        parents.push_back(parent);
        threads.push_back(_thread);
        dispatcher.add_node(node_addr, parent, _thread);
    }

    uint32_t i = 0;
    for (auto &it: nodes) {
        std::shared_ptr<xmock_node_info_t> &node = it;
        xvip2_t node_addr = node->m_xip;

        tcs[i]->reset_xip_addr(node_addr);
        i++;
    }

    sleep(20);

    tcs[0]->set_clock_shift(100);
    //tcs[1]->set_clock_shift(30);

    sleep(10);

//    tcs[0]->set_clock_shift(0);
  //  tcs[1]->set_clock_shift(0);

    while (1)
        sleep(1);

#if 0
    sleep(605);

    for (uint32_t i=0; i<node_count; i++) {
        threads[i]->close();
        {
            uint32_t count = threads[i]->get_refcount();
            for (uint32_t a=0; a<count; a++) {
                threads[i]->release_ref();
            }
        }

        //tcs[i]->close();
        {
            uint32_t count = tcs[i]->get_refcount();
            for (uint32_t a=0; a<count; a++) {
                tcs[i]->release_ref();
            }
        }

        parents[i]->close();
        {
            uint32_t count = parents[i]->get_refcount();
            for (uint32_t a=0; a<count; a++) {
                parents[i]->release_ref();
            }
        }
    }
#endif
}
