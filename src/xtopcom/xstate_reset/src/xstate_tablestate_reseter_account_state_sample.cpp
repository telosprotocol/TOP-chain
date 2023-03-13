// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_tablestate_reseter_account_state_sample.h"

#include "xcommon/xtoken_metadata.h"
#include "xcommon/common.h"
#include "xstatectx/xstatectx.h"

NS_BEG2(top, state_reset)

xstate_tablestate_reseter_account_state_sample::xstate_tablestate_reseter_account_state_sample(statectx::xstatectx_face_ptr_t statectx_ptr, std::string const & fork_name)
  : xstate_tablestate_reseter_base{statectx_ptr}, m_json_parser{statectx_ptr->get_table_address(), fork_name} {
}

bool xstate_tablestate_reseter_account_state_sample::exec_reset_tablestate(std::size_t) {
    xinfo("[exec_reset_tablestate] table %s at fork: %s, reset size: %zu", m_json_parser.table_account_str(), m_json_parser.fork_name_str(), m_json_parser.size());
    for (auto iter = m_json_parser.begin(); iter != m_json_parser.end(); ++iter) {
        xinfo("  account: %s", iter.key().c_str());
        // TODO: get unit bstate object.
        // auto unit_state = m_statectx->load_unit_state(base::xvaccount_t{iter.key()});
        // assert(unit_state);
        auto const & account = iter.key();
        auto const & account_json = iter.value();

        // state_data
        if (account_json.find("bstate") != account_json.end()) {
            auto const & account_state_json = account_json.at("bstate");
            auto sdata = account_state_json.get<std::string>();
            xinfo("    will reset state data to %s", sdata.c_str());
            account_set_state(account, sdata);
        }
    }
    xinfo("[exec_reset_tablestate] done");
    return true;
}

NS_END2