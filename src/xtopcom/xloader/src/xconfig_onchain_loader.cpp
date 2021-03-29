#include "xloader/xconfig_onchain_loader.h"

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbasic/xmap_utl.hpp"
#include "xconfig/xconfig_update_parameter_action.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xdata/xelect_transaction.hpp"
#include "xmbus/xevent.h"
#include "xmbus/xevent_store.h"
#include "xstore/xstore.h"
#include "xstore/xstore_error.h"

#include <inttypes.h>

NS_BEG2(top, loader)

xconfig_onchain_loader_t::xconfig_onchain_loader_t(observer_ptr<store::xstore_face_t> const &     store_ptr,
                                                   observer_ptr<mbus::xmessage_bus_face_t> const &     bus,
                                                   observer_ptr<time::xchain_time_face_t> const & logic_timer)
  : m_store_ptr{store_ptr}, m_bus{bus}, m_logic_timer{logic_timer} {
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
    m_db_id = m_bus->add_listener(top::mbus::xevent_major_type_store, std::bind(&xconfig_onchain_loader_t::update, shared_from_this(), std::placeholders::_1));

    m_logic_timer->watch("xconfig_onchain_parameter_loader", 1, std::bind(&xconfig_onchain_loader_t::chain_timer, shared_from_this(), std::placeholders::_1));
}

void xconfig_onchain_loader_t::stop() {
    // unregister block event listener
    assert(m_bus != nullptr);
    m_bus->remove_listener(top::mbus::xevent_major_type_store, m_db_id);
}

void xconfig_onchain_loader_t::update(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_to_db) {
        return;
    }

    mbus::xevent_store_block_to_db_ptr_t block_event = std::static_pointer_cast<mbus::xevent_store_block_to_db_t>(e);

    if (block_event == nullptr) {
        xassert(0);
        return;
    }
    auto block = block_event->block;
    xassert(block != nullptr);

    if (block->is_unitblock()) {
        std::string & owner = block_event->owner;

        if (owner != sys_contract_rec_tcc_addr) {
            return;
        }

        auto proposal_detail = block->get_native_property().string_get(CURRENT_VOTED_PROPOSAL);
        if (proposal_detail == nullptr) {
            return;
        }
        std::string voted_proposal = proposal_detail->get();

        tcc::proposal_info proposal{};

        top::base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)voted_proposal.data(), voted_proposal.size());
        if (stream.size() <= 0) {
            xwarn("[CONFIG] failed to get stream for block height: %" PRIu64, block_event->block->get_height());
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
            xinfo("[tcc] second time get onchain parameters");
            params_map.insert(initial_values.begin(), initial_values.end());
        } else {
            xinfo("[tcc] first time get onchain parameters");
            auto tx = std::make_shared<data::xtcc_transaction_t>();
            params_map.insert(tx->m_initial_values.begin(), tx->m_initial_values.end());
        }
    }
    return true;
}

NS_END2
