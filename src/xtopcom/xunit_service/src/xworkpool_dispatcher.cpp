// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xworkpool_dispatcher.h"

#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xunit_service/xcons_utl.h"
#include "xblockstore/xblockstore_face.h"

#include <cinttypes>

NS_BEG2(top, xunit_service)
#define WORK_DISPATCH_WATCHER "table_dispatch_timer"
xworkpool_dispatcher::xworkpool_dispatcher(observer_ptr<mbus::xmessage_bus_face_t> const &mb, std::shared_ptr<xcons_service_para_face> const & p_para, std::shared_ptr<xblock_maker_face> const & block_maker)
  : xcons_dispatcher(e_table), m_mbus(mb), m_para(p_para), m_blockmaker(block_maker) {
    xunit_info("xworkpool_dispatcher::xworkpool_dispatcher,create,this=%p", this);

}

xworkpool_dispatcher::~xworkpool_dispatcher() {
    xunit_info("xworkpool_dispatcher::~xworkpool_dispatcher,destroy,this=%p", this);
    xassert(m_packers.empty());
}

int16_t xworkpool_dispatcher::get_thread_index(base::xworkerpool_t * pool, base::xtable_index_t& table_id) {
    auto pool_size = pool->get_count();
    if (table_id.get_zone_index() == base::enum_chain_zone_beacon_index || table_id.get_zone_index() == base::enum_chain_zone_zec_index) {
        // zec & rec always dispatch to thread 0
        return 0;
    } else {
        // other table will dispatch to other thread
        auto pool_index = table_id.get_subaddr() % (pool_size - 1);
        return pool_index + 1;
    }
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
    // xunit_dbg("[xunitservice] dispatch table id %d, packer %p", table_id, packer);
    if (packer != nullptr) {
        auto packer_xip = packer->get_xip2_addr();
        if (is_xip2_equal(packer_xip, xip_to) && packer->get_account() == pdu->get_block_account()) {
            xunit_dbg("xworkpool_dispatcher::dispatch succ.pdu=%s,at_node:%s,packer=%p", pdu->dump().c_str(), xcons_utl::xip_to_hex(packer_xip).c_str(), packer);
            return async_dispatch(pdu, xip_from, xip_to, packer) == 0;
        } else {
            xunit_warn("xworkpool_dispatcher::dispatch fail. pdu=%s,table id %d failed packer %p with invalid xip to: %s vs packer: %s, account %s",
                pdu->dump().c_str(),
                table_id.to_table_shortid(),
                packer,
                xcons_utl::xip_to_hex(xip_to).c_str(),
                xcons_utl::xip_to_hex(packer_xip).c_str(),
                packer->get_account().c_str());
        }
    }
    return false;
}

void xworkpool_dispatcher::chain_timer(common::xlogic_time_t time) {
    assert(m_para);

    auto * blkstore = m_para->get_resources()->get_vblockstore();
    auto timer_block = blkstore->get_latest_cert_block(base::xvaccount_t(sys_contract_beacon_timer_addr), metrics::blockstore_access_from_us_dispatcher_load_tc);

    xunit_dbg("xworkpool_dispatcher::chain_timer call on_clock, logic time %" PRIu64 " TC %" PRIu64, time, timer_block->get_height());
    if (time <= timer_block->get_height()) {
        on_clock(timer_block.get());
    }
}

void xworkpool_dispatcher::on_clock(base::xvblock_t * clock_block) {
    auto work_pool = m_para->get_resources()->get_workpool();
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto const & pair : m_packers) {
            auto table_index = pair.first;
            auto worker = get_worker(work_pool, table_index);
            fire_clock(clock_block, worker, pair.second);
        }
    }
}

std::string xworkpool_dispatcher::account(base::xtable_index_t & tableid) {
    return data::xblocktool_t::make_address_table_account(tableid.get_zone_index(), tableid.get_subaddr());
}

base::xtable_index_t xworkpool_dispatcher::get_tableid(const std::string & account) {
    auto     xid1 = base::xvaccount_t::get_xid_from_account(account);
    base::enum_xchain_zone_index  zone1 = (base::enum_xchain_zone_index)get_vledger_zone_index(xid1);
    uint8_t table_id = get_vledger_subaddr(xid1);
    return {zone1, table_id};
}

std::string watcher_name(const xvip2_t & xip) {
    return std::string(WORK_DISPATCH_WATCHER).append("_").append(std::to_string(xip.low_addr));
}

bool xworkpool_dispatcher::start(const xvip2_t & xip, const common::xlogic_time_t& start_time) {
    // 1. get tableid from election by xip
    // 2. subscribe tableid
    // 3. reset xip
    xunit_info("xworkpool_dispatcher::start %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_para->get_resources()->get_chain_timer()->watch(watcher_name(xip), 1, std::bind(&xworkpool_dispatcher::chain_timer, shared_from_this(), std::placeholders::_1));
    auto election_face = m_para->get_resources()->get_election();
    auto elect_face = election_face->get_election_cache_face();
    if (elect_face != nullptr) {
        std::vector<base::xtable_index_t> tables;
        elect_face->get_tables(xip, &tables);
        subscribe(tables, xip, start_time);
    }
    return false;
}

bool xworkpool_dispatcher::fade(const xvip2_t & xip) {
    xunit_info("xworkpool_dispatcher::fade %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    auto election_face = m_para->get_resources()->get_election();
    auto elect_face = election_face->get_election_cache_face();
    if (elect_face != nullptr) {
        std::vector<base::xtable_index_t> tables;
        elect_face->get_tables(xip, &tables);

        auto pool = m_para->get_resources()->get_workpool();
        auto pool_thread_ids = pool->get_thread_ids();
        xbatch_packers fade_packers;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (size_t index = 0; index < tables.size(); index++) {
                // get account info
                auto table_id = tables[index];
                auto iter = m_packers.find(table_id);
                if (iter != m_packers.end()) {
                    fade_packers.push_back(iter->second);
                }
            }
        }

        // for each packer reset xip address async
        if (!fade_packers.empty()) {
            for (auto packer_ptr : fade_packers) {
                xbatch_packer * packer = packer_ptr.get();
                auto async_reset = [xip](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
                    auto packer = dynamic_cast<xbatch_packer *>(call.get_param1().get_object());
                    packer->set_fade_xip_addr(xip);
                    return true;
                };
                base::xcall_t asyn_call(async_reset, packer_ptr.get());
                packer->send_call(asyn_call);
            }
        }
        return true;
    }
    return false;
}

bool xworkpool_dispatcher::unreg(const xvip2_t & xip) {
    // do nothing
    xunit_info("xworkpool_dispatcher::unreg %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    m_para->get_resources()->get_chain_timer()->unwatch(watcher_name(xip));
    return false;
}

bool xworkpool_dispatcher::destroy(const xvip2_t & xip) {
    // 1. get tableid from election by xip
    // 2. erase all datas
    xunit_info("xworkpool_dispatcher::destroy %s %p", xcons_utl::xip_to_hex(xip).c_str(), this);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto & packer : m_packers) {
        packer.second->close();
    }
    m_packers.clear();
    return true;
}

bool xworkpool_dispatcher::subscribe(const std::vector<base::xtable_index_t> & tables, const xvip2_t & xip, const common::xlogic_time_t& start_time) {
    auto           pool = m_para->get_resources()->get_workpool();
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
                auto account_id = account(table_id);

                // build packer and push to container
                auto packer_ptr = make_object_ptr<xbatch_packer>(m_mbus, table_id, account_id, m_para, m_blockmaker, base::xcontext_t::instance(), thread_id);
                // packer_ptr->reset_xip_addr(xip);
                m_packers[table_id] = packer_ptr;
                reset_packers.push_back(packer_ptr);
                xunit_dbg("[xunitservice] subscribe %s %d @ %s %p", account_id.c_str(), table_id.to_table_shortid(), xcons_utl::xip_to_hex(xip).c_str(), this);
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
            auto            async_reset = [xip, start_time](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
                auto packer = dynamic_cast<xbatch_packer *>(call.get_param1().get_object());
                packer->set_start_time(start_time);
                packer->reset_xip_addr(xip);
                return true;
            };
            base::xcall_t asyn_call(async_reset, packer_ptr.get());
            packer->send_call(asyn_call);
        }
    }
    return true;
}

base::xworker_t * xworkpool_dispatcher::get_worker(base::xworkerpool_t * pool, base::xtable_index_t & table_id) {
    auto pool_index = get_thread_index(pool, table_id);
//    auto pool_thread_ids = pool->get_thread_ids();
    return pool->get_thread(pool_index);
}

void xworkpool_dispatcher::fire_clock(base::xvblock_t * block, base::xworker_t * worker, xbatch_packer_ptr_t packer) {
    auto _call = [](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        auto packer = dynamic_cast<xbatch_packer *>(call.get_param1().get_object());
        auto block_ptr = dynamic_cast<base::xvblock_t *>(call.get_param2().get_object());
        packer->fire_clock(*block_ptr, 0, 0);
        return true;
    };
    base::xcall_t asyn_call((base::xcallback_t)_call, packer.get(), block);
    worker->send_call(asyn_call);
}

NS_END2
