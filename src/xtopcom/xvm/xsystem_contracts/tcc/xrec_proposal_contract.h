#pragma once

#include "xbase/xdata.h"
#include "xdata/xdata_common.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xproposal_data.h"
#include "xvm/xcontract/xcontract_register.h"

NS_BEG2(top, tcc)

using namespace xvm;
using namespace xvm::xcontract;

/**
 * @brief the proposal contract
 *
 */
class xrec_proposal_contract final : public xcontract_base {
    using xbase_t = xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xrec_proposal_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xrec_proposal_contract);

    explicit
    xrec_proposal_contract(common::xnetwork_id_t const & network_id);

    xcontract_base*  clone() override {return new xrec_proposal_contract(network_id());}

    virtual void setup() override;

    /**
     * @brief sumbit a tcc proposal
     *
     * @param target  the proposal target
     * @param value  the proposal target value
     * @param type  the proposal type
     * @param effective_timer_height  the proposal take effect timer height
     */
    void submitProposal(const std::string& target,
                        const std::string& value,
                        proposal_type type,
                        uint64_t effective_timer_height);

    /**
     * @brief withdraw the proposal
     *
     * @param proposal_id  the proposal id
     */
    void withdrawProposal(const std::string& proposal_id);


    /**
     * @brief tcc member vote the proposal
     *
     * @param proposal_id  the proposal id
     * @param option  the vote option, true means agree, false means disagree
     */
    void tccVote(std::string& proposal_id, bool option);

    BEGIN_CONTRACT_WITH_PARAM(xrec_proposal_contract)
        CONTRACT_FUNCTION_PARAM(xrec_proposal_contract, submitProposal);
        CONTRACT_FUNCTION_PARAM(xrec_proposal_contract, withdrawProposal);
        CONTRACT_FUNCTION_PARAM(xrec_proposal_contract, tccVote);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief Get the proposal info object
     *
     * @param proposal_id  the proposal id
     * @param proposal  to store the proposal geted
     * @return true
     * @return false
     */
    bool get_proposal_info(const std::string& proposal_id, proposal_info& proposal);

    /**
     * @brief check whether the proposal expired
     *
     * @param proposal_id  the proposal id
     * @return true
     * @return false
     */
    bool proposal_expired(const std::string& proposal_id);

    /**
     * @brief check whether the voter is committee member
     *
     * @param voter_client_addr
     * @return true
     * @return false
     */
    bool voter_in_committee(const std::string& voter_client_addr);

    /**
     * @brief check whether the proposal tpye is valid
     *
     * @param type  the proposal type
     * @return true
     * @return false
     */
    bool is_valid_proposal_type(proposal_type type);

    /**
     * @brief delete the expired proposal
     *
     */
    void delete_expired_proposal();

    /**
     * @brief check bwlist proposal parameter
     *
     * @param bwlist the whitelist or blacklist
     *
     */
    void check_bwlist_proposal(std::string const& bwlist);

    /**
     * @brief Get the value map object
     *
     * @tparam value_type
     * @param map_key_id  the map property key
     * @param proposal_id  the proposal id
     * @param result  to store the map property geted
     * @return true
     * @return false
     */
    template <typename value_type>
    bool get_value_map(const std::string& map_key_id, const std::string& proposal_id, std::map<std::string, value_type>& result);
};

// REGISTER_NAME_CONTRACT(sys_contract_rec_tcc_addr, xrec_proposal_contract);
NS_END2
