#pragma once

#include "task/task_info.h"
#include "xtopcl/include/xtop/topchain_type.h"
#include "user_info.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"

#include <functional>
#include <map>
#include <queue>
#include <sstream>
#include <string>

namespace xChainSDK {

class api_method_imp final {
public:
    api_method_imp() {
    }
    ~api_method_imp() {
    }

    bool ChangeHost(const std::string host);

    bool make_private_key(std::array<uint8_t, PRI_KEY_LEN> & private_key, std::string & address);

    bool make_child_private_key(const std::string & parent_addr, std::array<uint8_t, PRI_KEY_LEN> & private_key, std::string & address);

    bool set_private_key(user_info & uinfo, const std::string & private_key);

    bool validate_key(const std::string & priv, const std::string & pub, const std::string & addr, const std::string & main_addr = "");

    bool make_keys_base64(std::string & private_key, std::string & public_key, std::string & address);
    void make_keys_base64(std::ostream & out);

public:
    template <typename T>
    bool stream_data(std::string & result, T &);

    bool create_account(const user_info & uinfo, std::function<void(ResultBase *)> func = nullptr);

    bool getAccount(const user_info & uinfo, const std::string & account, std::ostringstream & out_str, std::function<void(AccountInfoResult *)> func = nullptr);

    bool passport(const user_info & uinfo, std::function<void(RequestTokenResult *)> func = nullptr);

    bool key_store(const user_info & uinfo, const std::string & type, const std::string & value, std::function<void(ResultBase *)> func = nullptr);

    bool transfer(const user_info & uinfo,
                  const std::string & from,
                  const std::string & to,
                  uint64_t amount,
                  const std::string & memo,
                  std::ostringstream & out_str,
                  std::function<void(TransferResult *)> func = nullptr);

    bool stakeGas(const user_info & uinfo,
                  const std::string & from,
                  const std::string & to,
                  uint64_t amount,
                  std::ostringstream & out_str,
                  std::function<void(TransferResult *)> func = nullptr);

    bool unStakeGas(const user_info & uinfo,
                    const std::string & from,
                    const std::string & to,
                    uint64_t amount,
                    std::ostringstream & out_str,
                    std::function<void(TransferResult *)> func = nullptr);

    bool getTransaction(const user_info & uinfo,
                        const std::string & account,
                        const std::string & last_hash,
                        std::ostringstream & out_str,
                        std::function<void(AccountTransactionResult *)> func = nullptr);

    bool deployContract(user_info & uinfo,
                        std::string & contract_account,
                        const uint64_t tgas_limit,
                        const uint64_t amount,
                        const std::string & contract_code,
                        std::ostringstream & out_str,
                        std::function<void(PublishContractResult *)> func = nullptr);

    bool runContract(const user_info & uinfo,
                     const uint64_t amount,
                     const std::string & contract_account,
                     const std::string & contract_func,
                     const std::string & contract_params,
                     std::ostringstream & out_str,
                     std::function<void(CallContractResult *)> func = nullptr);

    bool getProperty(const user_info & uinfo,
                     const std::string & account,
                     const std::string & type,
                     const std::string & data1,
                     const std::string & data2,
                     std::function<void(GetPropertyResult *)> func = nullptr);

    bool create_sub_account(const user_info & uinfo,
                            const std::string & child_address,
                            const std::array<std::uint8_t, PRI_KEY_LEN> & child_private_key,
                            std::function<void(CreateSubAccountResult *)> func = nullptr);

    bool get_vote(const user_info & uinfo, uint64_t amount, uint64_t validity_period, std::function<void(GetVoteResult *)> func = nullptr);

    bool vote(const user_info & uinfo,
              const std::string & addr_to,
              const std::string & lock_hash,
              uint64_t amount,
              uint64_t expiration,
              std::function<void(VoteResult *)> func = nullptr);

    bool unvoteNode(const user_info & uinfo,
                    const std::string & candidate_address,
                    const std::string & lock_hash,
                    uint64_t amount,
                    std::function<void(AbolishVoteResult *)> func = nullptr);

    bool getBlock(const user_info & uinfo,
                  const std::string & owner,
                  const std::string & height,
                  std::ostringstream & out_str,
                  std::function<void(GetBlockResult *)> func = nullptr);

    bool getBlocksByHeight(const user_info & uinfo,
                           const std::string & owner,
                           const std::string & height,
                           std::ostringstream & out_str,
                           std::function<void(GetBlockResult *)> func = nullptr);

    bool registerNode(const user_info & uinfo,
                      const uint64_t mortgage,
                      const std::string & role,
                      const std::string & nickname,
                      const std::string & signing_key,
                      const uint32_t dividend_rate,
                      std::ostringstream & out_str,
                      std::function<void(NodeRegResult *)> func = nullptr);

    bool updateNodeType(const user_info & uinfo, const std::string & role, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool updateNodeInfo(const user_info & uinfo,
                        const std::string & role,
                        const std::string & name,
                        const uint32_t type,
                        uint64_t mortgage,
                        const uint32_t rate,
                        const std::string & node_sign_key,
                        std::ostringstream & out_str,
                        std::function<void(NodeRegResult *)> func = nullptr);

    bool stake_node_deposit(const user_info & uinfo, uint64_t deposit, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool unstake_node_deposit(const user_info & uinfo, uint64_t unstake_deposit, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool setNodeName(const user_info & uinfo, const std::string & nickname, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool updateNodeSignKey(const user_info & uinfo, const std::string & node_sign_key, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool unRegisterNode(const user_info & uinfo, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool redeemNodeDeposit(const user_info & uinfo, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool setDividendRatio(const user_info & uinfo, uint32_t dividend_rate, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool stakeVote(const user_info & uinfo, uint64_t mortgage, uint16_t lock_duration, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool unStakeVote(const user_info & uinfo, uint64_t amount, std::ostringstream & out_str, std::function<void(NodeRegResult *)> func = nullptr);

    bool voteNode(const user_info & uinfo, std::map<std::string, int64_t> & vote_infos, std::ostringstream & out_str, std::function<void(SetVoteResult *)> func = nullptr);

    bool unVoteNode(const user_info & uinfo, std::map<std::string, int64_t> & vote_infos, std::ostringstream & out_str, std::function<void(SetVoteResult *)> func = nullptr);

    bool claimNodeReward(const user_info & uinfo, std::ostringstream & out_str, std::function<void(ClaimRewardResult *)> func = nullptr);

    bool claimVoterDividend(const user_info & uinfo, std::ostringstream & out_str, std::function<void(ClaimRewardResult *)> func = nullptr);

    bool submitProposal(const user_info & uinfo,
                        uint8_t type,
                        const std::string & target,
                        const std::string & value,
                        uint64_t deposit,
                        uint64_t effective_timer_height,

                        std::ostringstream & out_str,
                        std::function<void(AddProposalResult *)> func = nullptr);

    bool withdrawProposal(const user_info & uinfo, const std::string & proposal_id, std::ostringstream & out_str, std::function<void(WithdrawProposalResult *)> func = nullptr);

    bool tccVote(const user_info & uinfo, const std::string & proposal_id, bool option, std::ostringstream & out_str, std::function<void(VoteProposalResult *)> func = nullptr);

    bool getChainInfo(const user_info & uinfo, std::ostringstream & out_str, std::function<void(ChainInfoResult *)> func = nullptr);
    bool queryNodeInfo(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(NodeInfoResult *)> func = nullptr);
    bool getElectInfo(const user_info & uinfo, const std::string & target, std::function<void(ElectInfoResult *)> func = nullptr);
    bool getStandbys(const user_info & uinfo, const std::string & account, std::function<void(AccountInfoResult *)> func = nullptr);
    bool queryNodeReward(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(NodeRewardResult *)> func = nullptr);
    bool listVoteUsed(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(VoteDistResult *)> func = nullptr);
    bool queryVoterDividend(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(VoterRewardResult *)> func = nullptr);
    bool queryProposal(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(GetProposalResult *)> func = nullptr);
    bool getCGP(const user_info & uinfo, const std::string & target, std::ostringstream & out_str, std::function<void(GetProposalResult *)> func = nullptr);

private:
    bool hash_signature(top::data::xtransaction_t * trans_action, const std::array<uint8_t, PRI_KEY_LEN> & private_key);

    uint64_t get_timestamp();

public:
    // void handle_result(std::string const& reuslt, ChainInfoResult* reformat_result);

    bool make_private_key(std::array<uint8_t, PRI_KEY_LEN> & private_key);

    std::string get_public_key(const std::array<uint8_t, PRI_KEY_LEN> & private_key);

    std::string make_account_address(std::array<uint8_t, PRI_KEY_LEN> & private_key, const uint8_t addr_type, const std::string & parent_addr);

    std::string get_sign(const top::uint256_t & hash, std::array<uint8_t, PRI_KEY_LEN> & private_key);

    template <typename... Args>
    std::string stream_params(top::base::xstream_t & stream, Args...);

    template <typename T, typename... Args>
    std::string stream_params(top::base::xstream_t & stream, T, Args...);
    void set_tx_deposit(uint64_t deposit) {
        m_deposit = deposit;
    };
    void reset_tx_deposit() {
        m_deposit = 100000;
    };

private:
    uint64_t m_deposit{100000};
    std::queue<std::string> result_queue;
};

template <typename T>
inline bool api_method_imp::stream_data(std::string & result, T & data) {
    top::base::xstream_t stream(top::base::xcontext_t::instance());
    result = stream_params(stream, data);
    return true;
}

template <typename... Args>
inline std::string api_method_imp::stream_params(top::base::xstream_t & stream, Args... args) {
    std::string param;
    param.assign((char *)stream.data(), stream.size());
    return param;
}

template <typename T, typename... Args>
inline std::string api_method_imp::stream_params(top::base::xstream_t & stream, T val, Args... args) {
    stream << val;
    return stream_params(stream, args...);
}
}  // namespace xChainSDK
