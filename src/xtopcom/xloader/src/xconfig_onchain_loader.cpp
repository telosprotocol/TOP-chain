#include "xloader/xconfig_onchain_loader.h"

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xvledger/xvledger.h"
#include "xbasic/xmap_utl.hpp"
#include "xconfig/xconfig_update_parameter_action.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xdata/xelect_transaction.hpp"
#include "xmbus/xevent.h"
#include "xmbus/xevent_store.h"
#include "xmetrics/xmetrics.h"

#include <inttypes.h>

NS_BEG2(top, loader)

xconfig_onchain_loader_t::xconfig_onchain_loader_t(observer_ptr<store::xstore_face_t> const &     store_ptr,
                                                   observer_ptr<mbus::xmessage_bus_face_t> const &     bus,
                                                   observer_ptr<time::xchain_time_face_t> const & logic_timer)
  : m_store_ptr{store_ptr}, m_bus{bus}, m_logic_timer{logic_timer}, m_monitor(nullptr) {
    // create actions
    m_action_map[UPDATE_ACTION_PARAMETER] = std::make_shared<config::xconfig_update_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_add)] = std::make_shared<config::xconfig_incremental_add_update_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_delete)] = std::make_shared<config::xconfig_incremental_delete_update_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_add_parameter)] = std::make_shared<config::xconfig_add_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_delete_parameter)] = std::make_shared<config::xconfig_delete_parameter_action_t>();
}

void xconfig_onchain_loader_t::start() {
    assert(m_bus);
    // register block event listener
    if (m_monitor == nullptr) {
        m_monitor = new xconfig_bus_monitor(this);
        m_monitor->init();
    }

    m_logic_timer->watch("xconfig_onchain_parameter_loader", 1, std::bind(&xconfig_onchain_loader_t::chain_timer, shared_from_this(), std::placeholders::_1));
}

void xconfig_onchain_loader_t::stop() {
    // unregister block event listener
    assert(m_bus != nullptr);
    if (m_monitor != nullptr) {
        m_monitor->uninit();
        delete m_monitor;
        m_monitor = nullptr;
    }
}

xconfig_onchain_loader_t::xconfig_bus_monitor::xconfig_bus_monitor(xconfig_onchain_loader_t* parent)
    : xbase_sync_event_monitor_t(parent->m_bus) ,m_parent(parent) {
}

xconfig_onchain_loader_t::xconfig_bus_monitor::~xconfig_bus_monitor() {
    m_parent = nullptr;
}

void xconfig_onchain_loader_t::xconfig_bus_monitor::init() {
    mbus::xevent_queue_cb_t cb = std::bind(&xconfig_onchain_loader_t::xconfig_bus_monitor::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_store, cb);
}

void xconfig_onchain_loader_t::xconfig_bus_monitor::uninit() {
    m_reg_holder.clear();
}

bool xconfig_onchain_loader_t::xconfig_bus_monitor::filter_event(const mbus::xevent_ptr_t & e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_to_db) {
        return false;
    }
    mbus::xevent_store_block_to_db_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_to_db_t>(e);
    std::string & owner = block_event->owner;
    return owner == sys_contract_rec_tcc_addr;
}

void xconfig_onchain_loader_t::xconfig_bus_monitor::process_event(const mbus::xevent_ptr_t & e) {
    if (m_parent != nullptr) {
        m_parent->update(e);
    }
}

void xconfig_onchain_loader_t::update(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_to_db) {
        return;
    }

    mbus::xevent_store_block_to_db_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_to_db_t>(e);
    if (block_event == nullptr) {
        xassert(0);
        return;
    }

    std::string & owner = block_event->owner;
    if (owner != sys_contract_rec_tcc_addr) {
        return;
    }

    auto block = mbus::extract_block_from(block_event, metrics::blockstore_access_from_mbus_onchain_loader_t_update);
    xassert(block != nullptr);

    xdbg("xconfig_onchain_loader_t::update tcc update begin,height=%ld", block->get_height());
    base::xauto_ptr<base::xvbstate_t> _bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(block.get(), metrics::statestore_access_from_xconfig_update);
    if (nullptr == _bstate) {
        xwarn("xconfig_onchain_loader_t::update get target state fail.block=%s", block->dump().c_str());
        return;
    }
    data::xunit_bstate_t state(_bstate.get());
    std::string voted_proposal;
    state.string_get(CURRENT_VOTED_PROPOSAL, voted_proposal);
    if (voted_proposal.empty()) {
        xwarn("xconfig_onchain_loader_t::update get property fail.block=%s", block->dump().c_str());
        return;
    }
    tcc::proposal_info proposal{};

    top::base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)voted_proposal.data(), voted_proposal.size());
    if (stream.size() <= 0) {
        xwarn("[CONFIG] failed to get stream for block height: %" PRIu64, block->get_height());
        return;
    }
    proposal.deserialize(stream);
    xdbg("[CONFIG] in update, receiving voting done proposal: %s, effective height: %" PRIu64, proposal.proposal_id.c_str(), proposal.effective_timer_height);

    uint64_t chain_timer = m_logic_timer->logic_time();
    if (proposal.effective_timer_height > chain_timer) {
        // store for later invoke
        std::lock_guard<std::mutex> lock(m_action_param_mutex);
        m_pending_proposed_parameters.insert({proposal.effective_timer_height, proposal});
        xkinfo("[CONFIG] proposal id: %s, current chain timer: %" PRIu64 ", effective height: %" PRIu64 ", future parameter size(): %zu",
                proposal.proposal_id.c_str(),
                chain_timer,
                proposal.effective_timer_height,
                m_pending_proposed_parameters.size());
        return;
    }

    std::string action_type;
    switch (proposal.type) {
        case tcc::proposal_type::proposal_update_parameter:
            action_type = UPDATE_ACTION_PARAMETER;
            break;
        case tcc::proposal_type::proposal_add_parameter:
            action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_add_parameter);
            break;
        case tcc::proposal_type::proposal_delete_parameter:
            action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_delete_parameter);
            break;
        case tcc::proposal_type::proposal_update_parameter_incremental_add:
            action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_add);
            break;
        case tcc::proposal_type::proposal_update_parameter_incremental_delete:
            action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_delete);
            break;

        default:
            break;
    }

    if (!action_type.empty()) {
        config::xconfig_update_action_ptr_t action_ptr = find(action_type);
        if (action_ptr != nullptr) {
            std::map<std::string, std::string> m;
            m.insert({proposal.parameter, proposal.new_value});
            m_action_map[action_type]->do_update(m);
            xkinfo("[CONFIG] current chain timer: %" PRIu64 ", proposal: %s, parameter: %s value: %s applied successfully",
                    chain_timer,
                    proposal.proposal_id.c_str(),
                    proposal.parameter.c_str(),
                    proposal.new_value.c_str());
        }
    }
}

void xconfig_onchain_loader_t::chain_timer(common::xlogic_time_t time) {
    xdbg("[xconfig_onchain_loader_t::chain_timer] %" PRIu64, m_logic_timer->logic_time());
    std::lock_guard<std::mutex> lock(m_action_param_mutex);

    auto it = m_pending_proposed_parameters.begin();
    while (it != m_pending_proposed_parameters.end()) {
        uint64_t                   chain_timer = m_logic_timer->logic_time();
        const tcc::proposal_info & proposal = it->second;
        xdbg("[CONFIG] proposal id: %s, parameter: %s, current chain timer: %" PRIu64 ", effective timer: %" PRIu64,
             proposal.proposal_id.c_str(),
             proposal.parameter.c_str(),
             chain_timer,
             proposal.effective_timer_height);
        auto old_it = it;
        ++it;

        if (chain_timer >= old_it->first) {
            std::string action_type;
            switch (proposal.type)
            {
            case tcc::proposal_type::proposal_update_parameter:
                action_type = UPDATE_ACTION_PARAMETER;
                break;
            case tcc::proposal_type::proposal_add_parameter:
                action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_add_parameter);
                break;
            case tcc::proposal_type::proposal_delete_parameter:
                action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_delete_parameter);
                break;
            case tcc::proposal_type::proposal_update_parameter_incremental_add:
                action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_add);
                break;
            case tcc::proposal_type::proposal_update_parameter_incremental_delete:
                action_type = XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_delete);
                break;
            default:
                break;
            }


            if (!action_type.empty()) {
                config::xconfig_update_action_ptr_t action_ptr = find(action_type);
                if (action_ptr != nullptr) {
                    std::map<std::string, std::string> m;
                    m.insert({proposal.parameter, proposal.new_value});
                    m_action_map[action_type]->do_update(m);
                    m_pending_proposed_parameters.erase(old_it);
                    xkinfo("[CONFIG] current chain timer: %" PRIu64 ", proposal: %s, parameter: %s value: %s applied successfully",
                         chain_timer,
                         proposal.proposal_id.c_str(),
                         proposal.parameter.c_str(),
                         proposal.new_value.c_str());
                }
            }
        }
    }
}

config::xconfig_update_action_ptr_t xconfig_onchain_loader_t::find(const std::string & type) {
    auto it = m_action_map.find(type);
    if (it != m_action_map.end()) {
        return it->second;
    }
    return nullptr;
}

bool xconfig_onchain_loader_t::save_conf(const std::map<std::string, std::string> & map) {
    return true;  // do nothing
}

bool xconfig_onchain_loader_t::fetch_all(std::map<std::string, std::string> & params_map) {
    if (m_store_ptr != nullptr) {
        std::map<std::string, std::string> initial_values;
        auto                               status = m_store_ptr->map_copy_get(sys_contract_rec_tcc_addr, ONCHAIN_PARAMS, initial_values);
        if (status == 0) {
            xinfo("xconfig_onchain_loader_t::fetch_all[tcc] second time get onchain parameters");
            params_map.insert(initial_values.begin(), initial_values.end());
        } else {
            xinfo("xconfig_onchain_loader_t::fetch_all[tcc] first time get onchain parameters");
            auto tx = std::make_shared<data::xtcc_transaction_t>();
            params_map.insert(tx->m_initial_values.begin(), tx->m_initial_values.end());
        }
#ifdef DEBUG
        for (auto & v : params_map) {
            xdbg("xconfig_onchain_loader_t::fetch_all k=%s,v=%s", v.first.c_str(), v.second.c_str());
        }
#endif
    }
    return true;
}

NS_END2
