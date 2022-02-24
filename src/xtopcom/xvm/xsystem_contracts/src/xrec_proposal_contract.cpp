#include "xvm/xsystem_contracts/tcc/xrec_proposal_contract.h"

#include "xbase/xbase.h"
#include "xbase/xutl.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xconfig_update_parameter_action.h"
#include "xdata/xblock.h"
#include "xdata/xelect_transaction.hpp"
#include "xmbus/xevent_store.h"
#include "xmetrics/xmetrics.h"
#include "xverifier/xverifier_errors.h"
#include "xverifier/xverifier_utl.h"
#include "xvledger/xvaccount.h"

#include <algorithm>
#include <cinttypes>
#include <ctime>

using namespace top::data;

NS_BEG2(top, tcc)

xrec_proposal_contract::xrec_proposal_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xrec_proposal_contract::setup() {
    xtcc_transaction_ptr_t tcc_genesis = std::make_shared<xtcc_transaction_t>();

    MAP_CREATE(ONCHAIN_PARAMS);
    for (const auto & entry : tcc_genesis->m_initial_values) {
        MAP_SET(ONCHAIN_PARAMS, entry.first, entry.second);
    }

    STRING_CREATE(SYSTEM_GENERATED_ID);
    STRING_SET(SYSTEM_GENERATED_ID, "0");

    MAP_CREATE(PROPOSAL_MAP_ID);
    // MAP_CREATE(COSIGN_MAP_ID);
    MAP_CREATE(VOTE_MAP_ID);
    STRING_CREATE(CURRENT_VOTED_PROPOSAL);

    top::chain_data::data_processor_t data;
    top::chain_data::xtop_chain_data_processor::get_contract_data(common::xlegacy_account_address_t{SELF_ADDRESS()}, data);
    TOP_TOKEN_INCREASE(data.top_balance);
}

bool xrec_proposal_contract::get_proposal_info(const std::string & proposal_id, proposal_info & proposal) {
    std::string value;
    try {
        value = MAP_GET(PROPOSAL_MAP_ID, proposal_id);
    } catch (top::error::xtop_error_t const &) {
        xdbg("[xrec_proposal_contract::get_proposal_info] can't find proposal for id: %s", proposal_id.c_str());
        return false;  // not exist
    }

    top::base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.data(), value.size());
    proposal.deserialize(stream);
    return true;
}

void xrec_proposal_contract::submitProposal(const std::string & target,
                                        const std::string & value,
                                        proposal_type type,
                                        uint64_t effective_timer_height) {
    XMETRICS_TIME_RECORD("sysContract_recTccProposal_submit_proposal");
    XMETRICS_CPU_TIME_RECORD("sysContract_recTccProposal_submit_proposal_cpu");

    // XCONTRACT_ENSURE(modification_description.size() <= MODIFICATION_DESCRIPTION_SIZE, "[xrec_proposal_contract::submitProposal] description size too big!");
    XCONTRACT_ENSURE(is_valid_proposal_type(type) == true, "[xrec_proposal_contract::submitProposal] input invalid proposal type!");

    const std::string src_account = SOURCE_ADDRESS();
    xdbg("[xrec_proposal_contract::submitProposal] account: %s, target: %s, value: %s, type: %d, effective_timer_height: %" PRIu64,
         src_account.c_str(), target.c_str(), value.c_str(), type, effective_timer_height);


    proposal_info proposal;
    switch (type)
    {
    case proposal_type::proposal_update_parameter:
        {
            XCONTRACT_ENSURE(MAP_FIELD_EXIST(ONCHAIN_PARAMS, target), "[xrec_proposal_contract::submitProposal] proposal_add_parameter target do not exist");
            std::string config_value = MAP_GET(ONCHAIN_PARAMS, target);
            if (config_value.empty()) {
                xwarn("[xrec_proposal_contract::submitProposal] parameter: %s, not found", target.c_str());
                std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
                top::error::throw_error(ec);
            }

            if (config_value == value) {
                xwarn("[xrec_proposal_contract::submitProposal] parameter: %s, provide onchain value: %s, new value: %s", target.c_str(), config_value.c_str(), value.c_str());
                std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
                top::error::throw_error(ec);
            }
        }
        break;

    case proposal_type::proposal_update_asset:
        XCONTRACT_ENSURE(xverifier::xtx_utl::address_is_valid(target) == xverifier::xverifier_error::xverifier_success, "[xrec_proposal_contract::submitProposal] proposal_update_asset type proposal, target invalid!");
        break;
    case proposal_type::proposal_add_parameter:
        {
            XCONTRACT_ENSURE(not MAP_FIELD_EXIST(ONCHAIN_PARAMS, target), "[xrec_proposal_contract::submitProposal] proposal_add_parameter target already exist");
            if (target == "whitelist" || target == "blacklist") check_bwlist_proposal(value);
        }

        break;
    case proposal_type::proposal_delete_parameter:
        XCONTRACT_ENSURE(MAP_FIELD_EXIST(ONCHAIN_PARAMS, target), "[xrec_proposal_contract::submitProposal] proposal_add_parameter target do not exist");
        break;
    case proposal_type::proposal_update_parameter_incremental_add:
    case proposal_type::proposal_update_parameter_incremental_delete:
        // current only support whitelist/blacklist
        XCONTRACT_ENSURE(target == "whitelist" || target == "blacklist", "[xrec_proposal_contract::submitProposal] current target cannot support proposal_update_parameter_increamental_add/delete");
        check_bwlist_proposal(value);
        break;
    default:
        xwarn("[xrec_proposal_contract::submitProposal] proposal type %u current not support", type);
        std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec);
        break;
    }


    const xtransaction_ptr_t trx_ptr = GET_TRANSACTION();
    top::base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)trx_ptr->get_source_action_para().data(), trx_ptr->get_source_action_para().size());

    data::xproperty_asset asset_out{0};
    stream >> asset_out.m_token_name;
    stream >> asset_out.m_amount;

    xdbg("[xrec_proposal_contract::submitProposal] the sender transaction token: %s, amount: %" PRIu64, asset_out.m_token_name.c_str(), asset_out.m_amount);
    auto min_tcc_proposal_deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tcc_proposal_deposit);
    XCONTRACT_ENSURE(asset_out.m_amount >= min_tcc_proposal_deposit * TOP_UNIT, "[xrec_proposal_contract::submitProposal] deposit less than minimum proposal deposit!");

    uint32_t proposal_expire_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(tcc_proposal_expire_time);
    std::string expire_time = MAP_GET(ONCHAIN_PARAMS, "tcc_proposal_expire_time");
    if (expire_time.empty()) {
        xwarn("[xrec_proposal_contract::submitProposal] parameter tcc_proposal_expire_time not found in blockchain");
    } else {
        try {
            proposal_expire_time = std::stoi(expire_time);
        } catch (std::exception & e) {
            xwarn("[xrec_proposal_contract::submitProposal] parameter tcc_proposal_expire_time in tcc chain: %s", expire_time.c_str());
        }
    }

    // set proposal info
    std::string system_id;
    try {
        system_id = STRING_GET(SYSTEM_GENERATED_ID);
    } catch (...) {
        xwarn("[xrec_proposal_contract::submitProposal] get system generated id error");
        throw;
    }
    proposal.proposal_id = base::xstring_utl::tostring(base::xstring_utl::touint64(system_id) + 1);
    proposal.parameter = target;
    proposal.new_value = value;
    proposal.deposit = asset_out.m_amount;
    proposal.type = type;
    // proposal.modification_description = modification_description;
    proposal.effective_timer_height = effective_timer_height;
    proposal.proposal_client_address = src_account;
    proposal.end_time = TIME() + proposal_expire_time;
    proposal.priority = priority_critical;


    // first phase no cosigning process
    proposal.cosigning_status = cosigning_success;
    proposal.voting_status = status_none;


    top::base::xstream_t newstream(base::xcontext_t::instance());
    proposal.serialize(newstream);
    std::string proposal_value((char *)newstream.data(), newstream.size());

    MAP_SET(PROPOSAL_MAP_ID, proposal.proposal_id, proposal_value);
    STRING_SET(SYSTEM_GENERATED_ID, proposal.proposal_id);
    xinfo("[xrec_proposal_contract::submitProposal] timer round: %" PRIu64 ", added new proposal: %s, stream detail size: %zu", TIME(), proposal.proposal_id.c_str(), value.size());
    XMETRICS_PACKET_INFO("sysContract_recTccProposal_submit_proposal", "timer round", std::to_string(TIME()), "proposal id", proposal.proposal_id);
    delete_expired_proposal();
}

void xrec_proposal_contract::withdrawProposal(const std::string & proposal_id) {
    XMETRICS_TIME_RECORD("sysContract_recTccProposal_withdraw_proposal");
    XMETRICS_CPU_TIME_RECORD("sysContract_recTccProposal_withdraw_proposal_cpu");
    auto src_account = SOURCE_ADDRESS();
    xdbg("[xrec_proposal_contract::withdrawProposal] proposal_id: %s, account: %s", proposal_id.c_str(), src_account.c_str());

    proposal_info proposal;
    if (!get_proposal_info(proposal_id, proposal)) {
        xdbg("[xrec_proposal_contract::withdrawProposal] can't find proposal: %s", proposal_id.c_str());
        std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec);
    }
    XCONTRACT_ENSURE(src_account == proposal.proposal_client_address, "[xrec_proposal_contract::withdrawProposal] only proposer can cancel the proposal!");

    // if (proposal.voting_status != status_none) {
    //     // cosigning in phase one is done automatically,
    //     // only check if voting in progress, can't withdraw
    //     xdbg("[PROPOSAL] in withdrawProposal proposal: %s, voting status: %u", proposal_id.c_str(), proposal.voting_status);
    //     return;
    // }
    if (MAP_FIELD_EXIST(VOTE_MAP_ID, proposal_id)) {
        MAP_REMOVE(VOTE_MAP_ID, proposal_id);
    }
    MAP_REMOVE(PROPOSAL_MAP_ID, proposal_id);

    xdbg("[xrec_proposal_contract::withdrawProposal] transfer deposit back, proposal id: %s, account: %s, deposit: %" PRIu64, proposal_id.c_str(), src_account.c_str(), proposal.deposit);
    TRANSFER(proposal.proposal_client_address, proposal.deposit);
    delete_expired_proposal();

}

void xrec_proposal_contract::tccVote(std::string & proposal_id, bool option) {
    XMETRICS_TIME_RECORD("sysContract_recTccProposal_tcc_vote");
    XMETRICS_CPU_TIME_RECORD("sysContract_recTccProposal_tcc_vote_cpu");
    const std::string src_account = SOURCE_ADDRESS();
    xdbg("[xrec_proposal_contract::tccVote] tccVote start, proposal_id: %s, account: %s vote: %d", proposal_id.c_str(), src_account.c_str(), option);

    // check if the voting client address exists in initial comittee
    if (!voter_in_committee(src_account)) {
        xwarn("[xrec_proposal_contract::tccVote] source addr is not a commitee voter: %s", src_account.c_str());
        std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec);
    }

    proposal_info proposal;
    if (!get_proposal_info(proposal_id, proposal)) {
        xwarn("[xrec_proposal_contract::tccVote] can't find proposal: %s", proposal_id.c_str());
        std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec);
    }

    if (proposal.voting_status == status_none) {
        proposal.voting_status = voting_in_progress;
    } else if (proposal.voting_status == voting_failure || proposal.voting_status == voting_success) {
        xdbg("[xrec_proposal_contract::tccVote] proposal: %s already voted done, status: %d", proposal_id.c_str(), proposal.voting_status);
        return;
    }


    auto committee_list = XGET_ONCHAIN_GOVERNANCE_PARAMETER(tcc_member);
    std::vector<std::string> vec_committee;
    uint32_t voter_committee_size = base::xstring_utl::split_string(committee_list, ',', vec_committee);


    std::map<std::string, bool> voting_result;
    get_value_map<bool>(VOTE_MAP_ID, proposal_id, voting_result);

    if (proposal_expired(proposal_id)) {
        uint32_t yes_voters = 0;
        uint32_t no_voters = 0;
        uint32_t not_yet_voters = 0;
        for (const auto & entry : voting_result) {
            if (entry.second) {
                ++yes_voters;
            } else {
                ++no_voters;
            }
        }
        not_yet_voters = voter_committee_size - yes_voters - no_voters;

        if (proposal.priority == priority_critical) {
            if (((yes_voters * 1.0 / voter_committee_size) >= (2.0 / 3)) && ((no_voters * 1.0 / voter_committee_size) < 0.20)) {
                proposal.voting_status = voting_success;
            } else {
                proposal.voting_status = voting_failure;
            }
        } else if (proposal.priority == priority_important) {
            if ((yes_voters * 1.0 / voter_committee_size) >= 0.51 && (not_yet_voters * 1.0 / voter_committee_size < 0.25)) {
                proposal.voting_status = voting_success;
            } else {
                proposal.voting_status = voting_failure;
            }
        } else {
            // normal priority
            if ((yes_voters * 1.0 / voter_committee_size) >= 0.51) {
                proposal.voting_status = voting_success;
            } else {
                proposal.voting_status = voting_failure;
            }
        }
        xdbg("[xrec_proposal_contract::tccVote] proposal: %s has expired, priority: %" PRIu8 ", yes voters: %u, no voters: %u, not yet voters: %u, voting status: %d",
             proposal_id.c_str(),
             proposal.priority,
             yes_voters,
             no_voters,
             not_yet_voters,
             proposal.voting_status);

    } else {
        auto it = voting_result.find(src_account);
        if (it != voting_result.end()) {
            xinfo("[xrec_proposal_contract::tccVote] client addr(%s) already voted", src_account.c_str());
            std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
            top::error::throw_error(ec);
        }
        // record the voting for this client address
        voting_result.insert({src_account, option});
        uint32_t yes_voters = 0;
        uint32_t no_voters = 0;
        uint32_t not_yet_voters = 0;
        for (const auto & entry : voting_result) {
            if (entry.second) {
                ++yes_voters;
            } else {
                ++no_voters;
            }
        }
        not_yet_voters = voter_committee_size - yes_voters - no_voters;

        if (proposal.priority == priority_critical) {
            if (((yes_voters * 1.0 / voter_committee_size) >= (2.0 / 3)) && ((no_voters * 1.0 / voter_committee_size) < 0.20) && (not_yet_voters == 0)) {
                proposal.voting_status = voting_success;
            } else if ((no_voters * 1.0 / voter_committee_size) >= 0.20) {
                proposal.voting_status = voting_failure;
            }
        } else if (proposal.priority == priority_important) {
            if ((yes_voters * 1.0 / voter_committee_size) >= 0.51 && (not_yet_voters * 1.0 / voter_committee_size < 0.25)) {
                proposal.voting_status = voting_success;
            } else if ((no_voters * 1.0 / voter_committee_size) >= 0.51) {
                proposal.voting_status = voting_failure;
            }
        } else {
            // normal priority
            if ((yes_voters * 1.0 / voter_committee_size) >= 0.51) {
                proposal.voting_status = voting_success;
            } else if ((no_voters * 1.0 / voter_committee_size) >= 0.51) {
                proposal.voting_status = voting_failure;
            }
        }
        xdbg("[xrec_proposal_contract::tccVote] proposal: %s has NOT expired, priority: %" PRIu8 ", yes voters: %u, no voters: %u, not yet voters: %u, voting status: %d",
             proposal_id.c_str(),
             proposal.priority,
             yes_voters,
             no_voters,
             not_yet_voters,
             proposal.voting_status);
    }

    if (proposal.voting_status == voting_failure || proposal.voting_status == voting_success) {
        xdbg("[xrec_proposal_contract::tccVote] proposal: %s, status: %d, transfer (%lu) deposit to client: %s",
             proposal_id.c_str(),
             proposal.voting_status,
             proposal.deposit,
             proposal.proposal_client_address.c_str());
        if (MAP_FIELD_EXIST(VOTE_MAP_ID, proposal_id)) {
            MAP_REMOVE(VOTE_MAP_ID, proposal_id);
        }
        MAP_REMOVE(PROPOSAL_MAP_ID, proposal_id);
        TRANSFER(proposal.proposal_client_address, proposal.deposit);

        if (proposal.voting_status == voting_success) {
            // save the voting status
            top::base::xstream_t stream(base::xcontext_t::instance());
            stream.reset();
            proposal.serialize(stream);
            std::string voted_proposal((char *)stream.data(), stream.size());
            std::string new_list{""};
            switch (proposal.type)
            {
            case proposal_type::proposal_update_parameter:
                // notify config center to load the changed parameter
                STRING_SET(CURRENT_VOTED_PROPOSAL, voted_proposal);
                MAP_SET(ONCHAIN_PARAMS, proposal.parameter, proposal.new_value);
                break;

            case proposal_type::proposal_update_asset:
                TRANSFER(proposal.parameter, base::xstring_utl::touint64(proposal.new_value));
                break;
            case proposal_type::proposal_add_parameter:
                MAP_SET(ONCHAIN_PARAMS, proposal.parameter, proposal.new_value);
                STRING_SET(CURRENT_VOTED_PROPOSAL, voted_proposal);
                break;
            case proposal_type::proposal_delete_parameter:
                MAP_REMOVE(ONCHAIN_PARAMS, proposal.parameter);
                STRING_SET(CURRENT_VOTED_PROPOSAL, voted_proposal);
                break;
            case proposal_type::proposal_update_parameter_incremental_add:
                new_list = top::config::xconfig_utl::incremental_add_bwlist(MAP_GET(ONCHAIN_PARAMS, proposal.parameter), proposal.new_value);
                MAP_SET(ONCHAIN_PARAMS, proposal.parameter, new_list);
                STRING_SET(CURRENT_VOTED_PROPOSAL, voted_proposal);
                break;
            case proposal_type::proposal_update_parameter_incremental_delete:
                new_list = top::config::xconfig_utl::incremental_delete_bwlist(MAP_GET(ONCHAIN_PARAMS, proposal.parameter), proposal.new_value);
                MAP_SET(ONCHAIN_PARAMS, proposal.parameter, new_list);
                STRING_SET(CURRENT_VOTED_PROPOSAL, voted_proposal);
                break;

            default:
                xwarn("[xrec_proposal_contract::tccVote] proposal type %u current not support", proposal.type);
                std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
                top::error::throw_error(ec);
                break;
            }

        }
    } else if (proposal.voting_status == voting_in_progress) {
        top::base::xstream_t stream(base::xcontext_t::instance());
        MAP_SERIALIZE_SIMPLE(stream, voting_result);
        MAP_SET(VOTE_MAP_ID, proposal_id, std::string((char *)stream.data(), stream.size()));

        stream.reset();
        proposal.serialize(stream);
        std::string voted_proposal((char *)stream.data(), stream.size());
        MAP_SET(PROPOSAL_MAP_ID, proposal_id, voted_proposal);
    }

    delete_expired_proposal();
}

bool xrec_proposal_contract::proposal_expired(const std::string & proposal_id) {
    proposal_info proposal{};
    if (!get_proposal_info(proposal_id, proposal)) {
        return false;
    }

    return proposal.end_time < TIME();
}

bool xrec_proposal_contract::is_valid_proposal_type(proposal_type type) {
    switch ( type)
    {
    case proposal_type::proposal_update_parameter:
    case proposal_type::proposal_update_asset:
    case proposal_type::proposal_add_parameter:
    case proposal_type::proposal_delete_parameter:
    case proposal_type::proposal_update_parameter_incremental_add:
    case proposal_type::proposal_update_parameter_incremental_delete:
        return true;

    default:
        return false;
    }
}

void xrec_proposal_contract::check_bwlist_proposal(std::string const& bwlist) {
    std::vector<std::string> vec_member;
    uint32_t size = base::xstring_utl::split_string(bwlist, ',', vec_member);
    XCONTRACT_ENSURE(size > 0, "[xrec_proposal_contract::check_bwlist_proposal] target value error, size zero");
    for (auto const& v: vec_member) {
        XCONTRACT_ENSURE(v.size() > top::base::xvaccount_t::enum_vaccount_address_prefix_size, "[xrec_proposal_contract::check_bwlist_proposal]  target value error, addr not support");
        auto const addr_type = top::base::xvaccount_t::get_addrtype_from_account(v);
        XCONTRACT_ENSURE(addr_type == top::base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_eth_user_account || addr_type == top::base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_user_account,
                            "[xrec_proposal_contract::check_bwlist_proposal]  target value error, addr type not support");
        XCONTRACT_ENSURE(top::xverifier::xverifier_success == top::xverifier::xtx_utl::address_is_valid(v), "[xrec_proposal_contract::check_bwlist_proposal]  target value error, addr invalid");
    }

    std::sort(vec_member.begin(), vec_member.end());
    vec_member.erase(std::unique(vec_member.begin(), vec_member.end()), vec_member.end());
    XCONTRACT_ENSURE(vec_member.size() == size, "[xrec_proposal_contract::check_bwlist_proposal]  target value error, addr duplicated");
}

template <typename value_type>
bool xrec_proposal_contract::get_value_map(const std::string & map_key_id, const std::string & proposal_id, std::map<std::string, value_type> & result) {
    std::string value;
    try {
        value = READ(enum_type_t::map, map_key_id, proposal_id);
    } catch (top::error::xtop_error_t const &) {
        return false;  // not exist
    }

    top::base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.data(), value.size());
    MAP_DESERIALIZE_SIMPLE(stream, result);
    return true;
}

bool xrec_proposal_contract::voter_in_committee(const std::string & voter_client_addr) {
    auto committee_list = XGET_ONCHAIN_GOVERNANCE_PARAMETER(tcc_member);
    std::vector<std::string> vec_committee;
    base::xstring_utl::split_string(committee_list, ',', vec_committee);

    for (auto const& addr: vec_committee) {
        if (addr == voter_client_addr) return true;
    }

    return false;
}

void xrec_proposal_contract::delete_expired_proposal() {

    std::map<std::string, std::string> proposals;

    try {
        MAP_COPY_GET(PROPOSAL_MAP_ID, proposals);
    } catch (...) {
        xkinfo("[xrec_proposal_contract::delete_expired_proposal] current cannot find any proposals");
        throw;
    }

    for (auto const& item: proposals) {
        proposal_info proposal{};
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)item.second.data(), item.second.size());
        proposal.deserialize(stream);

        if (proposal.end_time < TIME()) { //expired
            TRANSFER(proposal.proposal_client_address, proposal.deposit);
            MAP_REMOVE(PROPOSAL_MAP_ID, proposal.proposal_id);

            if (MAP_FIELD_EXIST(VOTE_MAP_ID, proposal.proposal_id)) {
                MAP_REMOVE(VOTE_MAP_ID, proposal.proposal_id);
            }

            xkinfo("[xrec_proposal_contract::delete_expired_proposal] delete proposal id: %s", proposal.proposal_id.c_str());

        }


    }
}

NS_END2
