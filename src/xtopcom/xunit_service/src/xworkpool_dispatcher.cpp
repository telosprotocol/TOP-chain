// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xworkpool_dispatcher.h"

#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xunit_service/xcons_utl.h"

#include <cinttypes>
NS_BEG2(top, xunit_service)
#define WORK_DISPATCH_WATCHER "table_dispatch_timer"
xworkpool_dispatcher::xworkpool_dispatcher(observer_ptr<mbus::xmessage_bus_face_t> const &mb,
    std::shared_ptr<xcons_service_para_face> const & p_para, std::shared_ptr<xblock_maker_face> const & block_maker)
  : xcons_dispatcher(e_table), m_mbus(mb), m_para(p_para), m_blockmaker(block_maker) {
    xinfo("xworkpool_dispatcher::xworkpool_dispatcher,create,this=%p", this);
}

xworkpool_dispatcher::~xworkpool_dispatcher() {
    xinfo("xworkpool_dispatcher::~xworkpool_dispatcher,destroy,this=%p", this);
    xassert(m_packers.empty());
}

int16_t xworkpool_dispatcher::get_thread_index(base::xworkerpool_t * pool, uint16_t table_id) {
    auto pool_size = pool->get_count();
    auto pool_index = table_id % pool_size;
    return pool_index;
}

bool xworkpool_dispatcher::dispatch(base::xworkerpool_t * pool, base::xcspdu_t * pdu, const xvip2_t & xip_from, const xvip2_t & xip_to) {
    auto            table_id = get_tableid(pdu->get_block_account());
    xbatch_packer * packer = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        packer_iter = m_packers.find(table_id);
        if (packer_iter != m_packers.end()) {
            packer = packer_iter->second.get();
        }
    }
    // xdbg("[xunitservice] dispatch table id %d, packer %p", table_id, packer);
    if (packer != nullptr) {
        auto packer_xip = packer->get_xip2_addr();
        if (is_xip2_equal(packer_xip, xip_to) && packer->get_account() == pdu->get_block_account()) {
            xdbg("xworkpool_dispatcher::dispatch succ.pdu=%s,at_node:%s,packer=%p", pdu->dump().c_str(), xcons_utl::xip_to_hex(packer_xip).c_str(), packer);
            return async_dispatch(pdu, xip_from, xip_to, packer) == 0;
        } else {
            xwarn("xworkpool_dispatcher::dispatch fail. pdu=%s,table id %d failed packer %p with invalid xip to: %s vs packer: %s, account %s",
                pdu->dump().c_str(),
                table_id,
                packer,
                xcons_utl::xip_to_hex(xip_to).c_str(),
                xcons_utl::xip_to_hex(packer_xip).c_str(),
                packer->get_account().c_str());
        }
    }
    return false;
}

void xworkpool_dispatcher::chain_timer(const time::xchain_time_st & time) {
    xdbg("xworkpool_dispatcher::chain_timer call on_clock, block height %" PRIu64 ", time round %" PRIu64, time.timer_block->get_height(), time.xtime_round);
    on_clock(time.timer_block);
}

void xworkpool_dispatcher::on_clock(base::xvblock_t * clock_block) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto const & pair : m_packers) {
            fire_clock(clock_block, pair.second);
        }
    }
}

std::string xworkpool_dispatcher::account(uint16_t tableid, const xvip2_t & xip) {
    auto zoneid = get_zone_id_from_xip2(xip);
    return data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)zoneid, tableid);
}

uint16_t xworkpool_dispatcher::get_tableid(const std::string & account) {
    auto     xid1 = base::xvaccount_t::get_xid_from_account(account);
    uint8_t  zone1 = get_vledger_zone_index(xid1);
    uint16_t subaddr1 = get_vledger_subaddr(xid1);
    return subaddr1;
}

std::string watcher_name(const xvip2_t & xip) {
    return std::string(WORK_DISPATCH_WATCHER).append("_").append(std::to_string(xip.low_addr));
}

bool xworkpool_dispatcher::start(const xvip2_t & xip) {
    // 1. get tableid from election by xip
    // 2. subscribe tableid
    // 3. reset xip
    xinfo("xworkpool_dispatcher::start %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_para->get_resources()->get_chain_timer()->watch(watcher_name(xip), 1, std::bind(&xworkpool_dispatcher::chain_timer, shared_from_this(), std::placeholders::_1));
    auto election_face = m_para->get_resources()->get_election();
    auto elect_face = election_face->get_election_cache_face();
    if (elect_face != nullptr) {
        std::vector<uint16_t> tables;
        elect_face->get_tables(xip, &tables);
        subscribe(tables, xip);
    }
    return false;
}

bool xworkpool_dispatcher::fade(const xvip2_t & xip) {
    // do nothing
    xinfo("xworkpool_dispatcher::fade %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_para->get_resources()->get_chain_timer()->unwatch(watcher_name(xip));
    return false;
}

bool xworkpool_dispatcher::destroy(const xvip2_t & xip) {
    // 1. get tableid from election by xip
    // 2. erase all datas
    xinfo("xworkpool_dispatcher::destroy %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto & packer : m_packers) {
        packer.second->close();
    }
    m_packers.clear();
    return true;

    // auto election_face = m_para->get_resources()->get_election();
    // auto elect_face = election_face->get_election_cache_face();
    // xassert(elect_face != nullptr);
    // if (elect_face != nullptr) {
    //     std::vector<uint16_t> tables;
    //     elect_face->get_tables(xip, &tables);
    //     {
    //         std::lock_guard<std::mutex> lock(m_mutex);
    //         for (size_t index = 0; index < tables.size(); index++) {
    //             // get account info
    //             auto table_id = tables[index];
    //             auto iter = m_packers.find(table_id);
    //             if (iter != m_packers.end()) {
    //                 iter->second->destroy();
    //                 m_packers.erase(iter);
    //             }
    //         }
    //     }
    //     return true;
    // }
    // return false;
}

bool xworkpool_dispatcher::subscribe(const std::vector<uint16_t> & tables, const xvip2_t & xip) {
    auto           pool = m_para->get_resources()->get_workpool();
    auto           pool_size = pool->get_count();
    auto           pool_thread_ids = pool->get_thread_ids();
    xbatch_packers reset_packers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t index = 0; index < tables.size(); index++) {
            // get account info
            auto table_id = tables[index];
            auto iter = m_packers.find(table_id);
            if (iter == m_packers.end()) {
                auto pool_index = get_thread_index(pool, table_id);
                auto thread_id = pool_thread_ids[pool_index];
                auto account_id = account(table_id, xip);

                // build packer and push to container
                auto packer_ptr = make_object_ptr<xbatch_packer>(m_mbus, table_id, account_id, m_para, m_blockmaker, base::xcontext_t::instance(), thread_id);
                // packer_ptr->reset_xip_addr(xip);
                m_packers[table_id] = packer_ptr;
                reset_packers.push_back(packer_ptr);
                xdbg("[xunitservice] subscribe %s %d @ %s %p", account_id.c_str(), table_id, xcons_utl::xip_to_hex(xip).c_str(), this);
            } else {
                // iter->second->reset_xip_addr(xip);
                reset_packers.push_back(iter->second);
            }
        }
    }

    // for each packer reset xip address async
    if (!reset_packers.empty()) {
        for (auto packer_ptr : reset_packers) {
            xbatch_packer * packer = packer_ptr.get();
            auto            async_reset = [xip](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
                auto packer = dynamic_cast<xbatch_packer *>(call.get_param1().get_object());
                packer->reset_xip_addr(xip);
                return true;
            };
            base::xcall_t asyn_call(async_reset, packer_ptr.get());
            packer->send_call(asyn_call);
        }
    }
    return true;
}

base::xworker_t * xworkpool_dispatcher::get_worker(base::xworkerpool_t * pool, uint16_t table_id) {
    auto pool_index = get_thread_index(pool, table_id);
    auto pool_thread_ids = pool->get_thread_ids();
    return pool->get_thread(pool_index);
}

void xworkpool_dispatcher::fire_clock(base::xvblock_t * block, xbatch_packer_ptr_t packer) {
    auto _call = [](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xbatch_packer *>(call.get_param1().get_object());
        auto block_ptr = dynamic_cast<base::xvblock_t *>(call.get_param2().get_object());
        packer->fire_clock(*block_ptr, 0, 0);
        return true;
    };
    base::xcall_t asyn_call((base::xcallback_t)_call, packer.get(), block);
    packer->send_call(asyn_call);
}

NS_END2
