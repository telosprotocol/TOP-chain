// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xverifier/xwhitelist_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xdata/xdata_common.h"

namespace top {
    namespace xverifier {

        onchain_whitelist xwhitelist_utl::wl{};

        bool xwhitelist_utl::include_in_whitelist(std::string const& addr) {
            auto wl = get_whitelist_from_config();
#ifdef DEBUG
            std::string wl_str{""};
            for (auto const& v: wl) wl_str += v + ",";
            xdbg("[xwhitelist_utl::include_in_whitelist] wl: %s, addr: %s", wl_str.c_str(), addr.c_str());
#endif
            auto iter = std::find_if(wl.begin(), wl.end(), [&addr](std::string const& w) { return w == addr; });
            return iter != wl.end();
        }

        bool xwhitelist_utl::check_whitelist_limit_tx(data::xtransaction_t const * trx_ptr) {
            xdbg("[global_trace][check_whitelist_limit] tx:source:%s target:%s action_name:%s hash:%s",
                    trx_ptr->get_source_addr().c_str(),
                    trx_ptr->get_target_addr().c_str(),
                    trx_ptr->get_target_action_name().c_str(),
                    trx_ptr->get_digest_hex_str().c_str());

            // whether whitelist open
            if (!XGET_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist)) return false;

            bool is_limit_tx = false;

            switch (trx_ptr->get_tx_type())
            {
            case data::xtransaction_type_transfer:
                is_limit_tx = true;
                break;

            case data::xtransaction_type_pledge_token_vote: //stakeVote
                is_limit_tx = true;
                break;
            case data::xtransaction_type_vote: //vote
                is_limit_tx = true;
                break;
            case data::xtransaction_type_run_contract: // run contract
                if ((trx_ptr->get_target_action_name() == vote_interface ||  trx_ptr->get_target_action_name() == claim_reward_interface) &&
                 ((trx_ptr->get_target_addr().substr(0, strlen(top::sys_contract_sharding_vote_addr)) == top::sys_contract_sharding_vote_addr) ||
                   trx_ptr->get_target_addr().substr(0, strlen(top::sys_contract_sharding_reward_claiming_addr)) == top::sys_contract_sharding_reward_claiming_addr)) { // vote & claim user reward
                    is_limit_tx = true;
                } else if (data::is_user_contract_address(common::xaccount_address_t{trx_ptr->get_target_addr()})) { // lua contract
                    is_limit_tx = true;
                }
                break;
            default:
                break;
            }

            if (is_limit_tx && !include_in_whitelist(trx_ptr->get_source_addr())) {
                xdbg("[global_trace][check_whitelist_limit][fail]whitelist limit, tx:source:%s target:%s hash:%s",
                    trx_ptr->get_source_addr().c_str(),
                    trx_ptr->get_target_addr().c_str(),
                    trx_ptr->get_digest_hex_str().c_str());
                return true;
            }

            return false;

        }

        int  xwhitelist_utl::split_string(const std::string & input,const char split_char, std::vector<std::string> & values)
        {
            if(input.empty())
                return 0;

            std::string::size_type begin_pos = 0;
            std::string::size_type pos_of_split = input.find_first_of(split_char,begin_pos);
            while(pos_of_split != std::string::npos)
            {
                if(pos_of_split != begin_pos)
                    values.push_back(input.substr(begin_pos,pos_of_split - begin_pos)); //[)
                begin_pos = pos_of_split + 1; //skip boundary
                pos_of_split = input.find_first_of(split_char, begin_pos);
                if(pos_of_split == std::string::npos) //not find the last split-char
                {
                    if(begin_pos < input.size())
                    {
                        values.push_back(input.substr(begin_pos)); //put the remaining section
                    }
                }
            }
            if(values.empty())
                values.push_back(input);

            return (int)values.size();
        }

        onchain_whitelist xwhitelist_utl::get_whitelist_from_config() {
            auto offchain_config = XGET_CONFIG(local_whitelist);
            auto onchain_config = XGET_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);
            xdbg("[xwhitelist_utl::get_whitelist_from_config] offchain: %s", offchain_config.c_str());
            xdbg("[xwhitelist_utl::get_whitelist_from_config] onchain: %s", onchain_config.c_str());

            onchain_whitelist local_wl;
            std::vector<std::string> vec;
            if (!offchain_config.empty()) {
                vec.clear();
                base::xstring_utl::split_string(offchain_config, ',', vec);
                for (auto const& v: vec)  local_wl.push_back(v);
            }

            if (!onchain_config.empty()) {
                vec.clear();
                base::xstring_utl::split_string(onchain_config, ',', vec);

                for (auto const& v: vec) local_wl.push_back(v);
            }

#ifdef DEBUG
            std::string local_wl_str{""};
            for (auto const& v: local_wl) local_wl_str += v + ",";
            xdbg("[xwhitelist_utl::get_whitelist_from_config] local_wl: %s", local_wl_str.c_str());
#endif
            return local_wl;

        }


    }  // namespace xverifier
}  // namespace top
