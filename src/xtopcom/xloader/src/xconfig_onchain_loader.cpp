#include "xloader/xconfig_onchain_loader.h"

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbasic/xmap_utl.hpp"
#include "xvledger/xvledger.h"
#include "xconfig/xconfig_update_parameter_action.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xproposal_data.h"
#include "xdata/xelect_transaction.hpp"
#include "xmbus/xevent.h"
#include "xmbus/xevent_store.h"
#include "xmetrics/xmetrics.h"
#include "xstatestore/xstatestore_face.h"

#include <inttypes.h>

NS_BEG2(top, loader)

xconfig_onchain_loader_t::xconfig_onchain_loader_t(observer_ptr<mbus::xmessage_bus_face_t> const &     bus,
                                                   observer_ptr<time::xchain_time_face_t> const & logic_timer)
  : m_bus{bus}, m_logic_timer{logic_timer} {
    // create actions
    m_action_map[UPDATE_ACTION_PARAMETER] = std::make_shared<config::xconfig_update_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_add)] = std::make_shared<config::xconfig_incremental_add_update_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_update_parameter_incremental_delete)] = std::make_shared<config::xconfig_incremental_delete_update_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_add_parameter)] = std::make_shared<config::xconfig_add_parameter_action_t>();
    m_action_map[XPROPOSAL_TYPE_TO_STR(tcc::proposal_type::proposal_delete_parameter)] = std::make_shared<config::xconfig_delete_parameter_action_t>();

    // set initial onchain param
    data::xtcc_transaction_ptr_t tcc_genesis = std::make_shared<data::xtcc_transaction_t>();
    m_last_param_map = tcc_genesis->m_initial_values;
}

void xconfig_onchain_loader_t::start() {
    assert(m_bus);
    m_logic_timer->watch("xconfig_onchain_parameter_loader", 1, std::bind(&xconfig_onchain_loader_t::update_onchain_param, shared_from_this(), std::placeholders::_1));
}

void xconfig_onchain_loader_t::stop() {
    // unregister block event listener
    assert(m_bus != nullptr);
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

void xconfig_onchain_loader_t::update_onchain_param(common::xlogic_time_t time) {
    xdbg("xconfig_onchain_loader_t::update_onchain_param,  logic time is %" PRIu64, time);
    std::lock_guard<std::mutex> lock(m_action_param_mutex);
    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(rec_tcc_contract_address);
    if (nullptr == unitstate) {
        xerror("xconfig_onchain_loader_t::update_onchain_param fail-load state.%s", rec_tcc_contract_address.to_string().c_str());
        return;
    }

    if (unitstate->height() < m_last_update_height) {
        xdbg("xconfig_onchain_loader_t::update_onchain_param, height=%" PRIu64 ", last_update_height: %" PRIu64, unitstate->height(), m_last_update_height);
        return;
    }

    xdbg("xconfig_onchain_loader_t::update_onchain_param, current light height=%ld", unitstate->height());

    m_last_update_height = unitstate->height();

    std::map<std::string, std::string> params = unitstate->map_get(ONCHAIN_PARAMS);
    if (params.empty()) {
        xerror("xconfig_onchain_loader_t::update_onchain_param, get params map fail.");
        return;
    }

    if (!m_last_param_map.empty() && !onchain_param_changed(params)) {
        xinfo("xconfig_onchain_loader_t::update_onchain_param, param map no changed.");
        return;
    }

    std::map<std::string, std::string> change_and_add_param;
    std::map<std::string, std::string> deleted_param;
    filter_changes(params, change_and_add_param);
    get_deleted_params(params, deleted_param);
    config::xconfig_register_t::get_instance().update_cache_and_persist(change_and_add_param);
    if (!deleted_param.empty()) config::xconfig_register_t::get_instance().add_delete_params(deleted_param, false);
    m_last_param_map = params;
    xdbg("xconfig_onchain_loader_t::update_onchain_param, update param sucessfully");
}


void xconfig_onchain_loader_t::get_deleted_params(std::map<std::string, std::string> const& map, std::map<std::string, std::string>& deleted_map) const {
    // fix delete param change
    if (map.size() < m_last_param_map.size()) {
        for (auto& entry: m_last_param_map) {
            if (map.find(entry.first) == map.end()) deleted_map[entry.first] = entry.second;
        }

    }

}

void xconfig_onchain_loader_t::filter_changes(std::map<std::string, std::string> const& map, std::map<std::string, std::string>& filterd_map) const {
    // for modify & add case
    for (auto& entry : map) {
        if (is_param_changed(entry.first, entry.second)) {
            filterd_map[entry.first] = entry.second;
        }
    }
}

bool xconfig_onchain_loader_t::is_param_changed(std::string const& key, std::string const& value) const {
    auto it = m_last_param_map.find(key);
    if (it == m_last_param_map.end()) {
        return true;
    }

    return it->second != value;
}

bool xconfig_onchain_loader_t::onchain_param_changed(std::map<std::string, std::string> const& params) {
    using value_pair = std::pair<std::string, std::string>;
    auto is_same = params.size() == m_last_param_map.size() &&
            std::equal(m_last_param_map.begin(), m_last_param_map.end(), params.begin(), [](value_pair const& lhs, value_pair const& rhs){
            #ifdef DEBUG
                xdbg("xconfig_onchain_loader_t::update_onchain_param, 1: %s,%s; 2: %s,%s\n", lhs.first.c_str(), lhs.second.c_str(), rhs.first.c_str(), rhs.second.c_str());
            #endif
                return lhs.first == rhs.first && lhs.second == rhs.second;
            });
    return !is_same;
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
    std::map<std::string, std::string> initial_values;
    auto status = statestore::xstatestore_hub_t::instance()->map_copy_get(rec_tcc_contract_address, ONCHAIN_PARAMS, initial_values);
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
    return true;
}

NS_END2
