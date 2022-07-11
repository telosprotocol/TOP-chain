#pragma once
#include "CLI11.hpp"
#include "api_method_imp.h"
#include "base/config_file.h"
#include "xtopcl/include/web/client_http.hpp"
#include "xtopcl/include/xtop/topchain_type.h"
#include "user_info.h"

#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace xChainSDK {
using std::string;
std::vector<std::string> const COMMAND_LEVEL_FILTER = {"get", "system", "sendtx", "debug", "wallet"};
std::vector<std::string> const COMMAND_HELP_STRING = {"-h", "--help"};
constexpr int INDENT_WIDTH = 4;
constexpr int HELP_WIDTH = 36;
static const uint32_t kExpirePeriod = 2 * 60 * 60 * 1000;  // expire  after 2 * 60 * 60 s92h)

enum class Command_type : uint8_t { toplevel, get, system, sendtransaction, wallet, subcommands, debug };


class ApiMethod final {
public:
    using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

    ApiMethod();
    ~ApiMethod();
    bool set_default_prikey(std::ostringstream & out_str, const bool is_query = false);
    int update_account(std::ostringstream & out_str);
    int update_account(std::ostringstream & out_str, xJson::Value & root);
    xJson::Value get_response_from_daemon();
    string get_account_from_daemon();
    string get_prikey_from_daemon(std::ostringstream & out_str);
    int set_prikey_to_daemon(const string & account, const string & pri_key, std::ostringstream & out_str, uint32_t expired_time = kExpirePeriod);
    int set_pw_by_file(const string & pw_path, string & pw);
    void tackle_null_query(std::ostringstream & out_str, std::string null_out = "");
    void tackle_send_tx_request(std::ostringstream & out_str);

    /*
     * wallet
     */
    void create_account(const int32_t & pf, const string & pw_path, std::ostringstream & out_str);
    void create_key(std::string & owner_account, const int32_t & pf, const string & pw_path, std::ostringstream & out_str);
    std::unordered_map<std::string, std::string> queryNodeInfos();
    void list_accounts(std::ostringstream & out_str);
    void set_default_account(const std::string & account, const string & pw_path, std::ostringstream & out_str);
    void import_keystore(const std::string & keystore, std::ostringstream & out_str);
    void reset_keystore_password(std::string & path, std::ostringstream & out_str);
    int set_default_miner(const std::string & pub_key, const std::string & pw_path, std::ostringstream & out_str);
    void import_account(const int32_t & pf, std::ostringstream & out_str);
    void export_account(const std::string & account, std::ostringstream & out_str);
    /*
     * debug
     */
    void create_chain_account(std::ostringstream & out_str);
    void import_key(std::string & pri_key, std::ostringstream & out_str);

    /*
     * transfer
     */
    void transfer1(std::string & to, std::string & amount, std::string & note, std::string & tx_deposit, std::ostringstream & out_str);

    /*
     * query transaction
     */
    void query_tx(std::string & account, std::string & tx_hash, std::ostringstream & out_str);

    /*
    *  estimate gas
    */
    void estimategas(std::string & to, std::string & amount_d, std::string & note, std::string & tx_deposit_d, std::ostringstream & out_str);

    /*
     * mining
     */
    void query_miner_info(std::string & target, std::ostringstream & out_str);
    void register_node(const std::string & mortgage_d,
                       const std::string & role,
                       const std::string & nickname,
                       const uint32_t & dividend_rate,
                       std::string & signing_key,
                       std::ostringstream & out_str);
    void query_miner_reward(std::string & target, std::ostringstream & out_str);
    void claim_miner_reward(std::ostringstream & out_str);
    void set_dividend_ratio(const uint32_t & dividend_rate, const std::string & tx_deposit, std::ostringstream & out_str);
    void set_miner_name(std::string & name, std::ostringstream & out_str);
    void change_miner_type(std::string & role, std::ostringstream & out_str);
    void unregister_node(std::ostringstream & out_str);
    void update_miner_info(const std::string & role,
                           const std::string & name,
                           const uint32_t & type,
                           const std::string & mortgage,
                           const uint32_t & rate,
                           const std::string & node_sign_key,
                           std::ostringstream & out_str);
    void add_deposit(const std::string & deposit, std::ostringstream & out_str);
    void reduce_deposit(const std::string & deposit, std::ostringstream & out_str);
    void withdraw_deposit(std::ostringstream & out_str);

    /*
     * chain
     */
    void query_account(std::string & target, std::ostringstream & out_str);
    void query_block(std::string & target, std::string & height, std::ostringstream & out_str);
    void getBlocksByHeight(std::string & target, std::string & height, std::ostringstream & out_str);
    void chain_info(std::ostringstream & out_str);
    void general_info(std::ostringstream & out_str);
    void deploy_contract(const uint64_t & gas_limit, const std::string & amount, const std::string & path, const std::string & deposit, std::ostringstream & out_str);
    void call_contract(const std::string & amount, const string & addr, const std::string & func, const string & params, const std::string & tx_deposit, std::ostringstream & out_str);
    void block_prune(std::string & prune_enable, std::ostringstream & out_str);
    /*
     * govern
     */
    void get_proposal(std::string & target, std::ostringstream & out_str);
    void cgp(std::ostringstream & out_str);
    void submit_proposal(uint8_t & type, const std::string & target, const std::string & value, std::string & deposit, uint64_t & effective_timer_height, std::ostringstream & out_str);
    void withdraw_proposal(const std::string & proposal_id, std::ostringstream & out_str);
    void tcc_vote(const std::string & proposal_id, const std::string & opinion, std::ostringstream & out_str);

    /*
     * resource
     */
    void stake_for_gas(std::string & amount, std::ostringstream & out_str);
    void withdraw_fund(std::string & amount, std::ostringstream & out_str);

    /*
     * staking
     */
    void stake_fund(uint64_t & amount, uint16_t & lock_duration, std::ostringstream & out_str);
    void stake_withdraw_fund(uint64_t & amount, const std::string & tx_deposit, std::ostringstream & out_str);
    void vote_miner(std::vector<std::pair<std::string, int64_t>> & vote_infos, std::ostringstream & out_str);
    void withdraw_votes(std::vector<std::pair<std::string, int64_t>> & vote_infos, std::ostringstream & out_str);
    void query_votes(std::string & target, std::ostringstream & out_str);
    void query_reward(std::string & target, std::ostringstream & out_str);
    void claim_reward(std::ostringstream & out_str);

    void outAccountBalance(const std::string & account, std::ostringstream & out_str);

    // helper setting function
    int set_userinfo();
    void change_trans_mode(bool use_http);
    void set_keystore_path(const std::string & data_dir);
    bool check_password();
    std::string get_password() {
        return cache_pw;
    }
    void reset_tx_deposit() {
        api_method_imp_.reset_tx_deposit();
    }
    void get_token();
    int get_account_info(std::ostringstream & out_str, xJson::Value & root);

private:
    void dump_userinfo(const user_info & info);
    std::string input_hiding();
    std::string input_no_hiding();
    static int parse_top_double(const std::string &amount, const uint32_t unit, uint64_t &out);
    int input_pri_key(std::string& pri_str);
    int get_eth_file(std::string& account);
private:
    api_method_imp api_method_imp_;
    std::string cache_pw{" "};
    const std::string empty_pw{" "};
    bool is_account{false};

public:
    bool is_reset_pw{false};
};


}  // namespace xChainSDK
