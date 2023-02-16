// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_tablestate_reseter_continuous_sample.h"

#include "xcommon/xtoken_metadata.h"
#include "xcommon/common.h"
#include "xstatectx/xstatectx.h"

NS_BEG2(top, state_reset)

xstate_tablestate_reseter_continuous_sample::xstate_tablestate_reseter_continuous_sample(statectx::xstatectx_face_ptr_t statectx_ptr, std::string const & fork_name)
  : xstate_tablestate_reseter_base{statectx_ptr}, m_json_parser{statectx_ptr->get_table_address(), fork_name} {
}

static const std::size_t each_reset_maximum_reset_account_num = 100;

bool xstate_tablestate_reseter_continuous_sample::exec_reset_tablestate(std::size_t cnt) {
    xinfo("[exec_reset_tablestate] table %s at fork: %s, json total size: %zu, current cnt: %zu",
          m_json_parser.table_account_str(),
          m_json_parser.fork_name_str(),
          m_json_parser.size(),
          cnt);
    std::size_t reset_account_cnt = 0;
    std::size_t skip_account_cnt = 0;

    auto iter = m_json_parser.begin();
    for (; iter != m_json_parser.end(); ++iter) {
        if (skip_account_cnt < cnt * each_reset_maximum_reset_account_num) {
            skip_account_cnt++;
            continue;
        }
        break;
    }
    for (; iter != m_json_parser.end(); ++iter) {
        if (++reset_account_cnt > each_reset_maximum_reset_account_num) {
            break;
        }
        xinfo("  account: %s", iter.key().c_str());
        // TODO: get unit bstate object.
        // auto unit_state = m_statectx->load_unit_state(base::xvaccount_t{iter.key()});
        // assert(unit_state);
        auto const & account = iter.key();
        auto const & account_json = iter.value();

        // tep1 token
        if (account_json.find("TEP1") != account_json.end()) {
            auto const & account_tep1_json = account_json.at("TEP1");
            xinfo("    will reset TEP1 token %s", account_tep1_json.dump().c_str());
            for (auto tep1_token_iter = account_tep1_json.begin(); tep1_token_iter != account_tep1_json.end(); ++tep1_token_iter) {
                assert(tep1_token_iter.value().is_string());
                common::xtoken_id_t token_id = static_cast<common::xtoken_id_t>(std::atoi(tep1_token_iter.key().c_str()));
                evm_common::u256 token_value{tep1_token_iter.value().get<std::string>()};
                xdbg("        TEP1 token %s, value %s", top::to_string(token_id).c_str(), token_value.str().c_str());
                // set unit bstate
                account_set_tep1_token(account, token_id, token_value);
            }
        }

        // top balance
        if (account_json.find("TOP") != account_json.end()) {
            auto const & account_top_json = account_json.at("TOP");
            xinfo("    will reset TOP balance %s", account_top_json.dump().c_str());
            for (auto top_balance_iter = account_top_json.begin(); top_balance_iter != account_top_json.end(); ++top_balance_iter) {
                assert(top_balance_iter.value().is_string());
                std::string property_name = top_balance_iter.key();
                uint64_t property_value = std::atoll(top_balance_iter.value().get<std::string>().c_str());
                xdbg("        TOP balance %s, value: % " PRIu64, property_name.c_str(), property_value);
                // set unit bstate
                account_set_top_balance(account, property_name, property_value);
            }
        }

        // properties
        // if (account_json.find("properties") != account_json.end()) {
        //     auto const & account_properties_json = account_json.at("properties");
        //     xinfo("    will reset properties %s", account_properties_json.dump().c_str());
        //     for (auto properties_iter = account_properties_json.begin(); properties_iter != account_properties_json.end(); ++properties_iter) {
        //         std::string property_name = properties_iter.key();
        //         json property_value = properties_iter.value();
        //         assert(property_value.find("type") != property_value.end());
        //         assert(property_value.find("data") != property_value.end());
        //         assert(property_value.at("type").is_string());
        //         assert(property_value.at("data").is_string());
        //         std::string property_type = property_value.at("type").get<std::string>();
        //         std::string property_data = property_value.at("data").get<std::string>();
        //         xdbg("        property %s ,type: %s, data: %s", property_name.c_str(), property_type.c_str(), property_data.c_str());
        //         // set unit bstate
        //         account_set_property(account, property_name, property_type, property_data);
        //     }
        // }
    }
    return true;
}

NS_END2