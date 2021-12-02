#include "api_method.h"

#include "base/log.h"
#include "base/utility.h"
#include "task/request_task.h"
#include "task/task_dispatcher.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xcrypto.h"
#include "xcrypto/xckey.h"
#include "xcrypto_util.h"
#include "xdata/xnative_contract_address.h"
#include "xpbase/base/top_utils.h"

#include <dirent.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <cmath>

namespace xChainSDK {
using namespace xcrypto;
using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ostringstream;
static const std::string SAFEBOX_ENDPOINT_ENV = "SAFEBOX_ENDPOINT";

bool check_pri_key(const std::string & str_pri) {
    return BASE64_PRI_KEY_LEN == str_pri.size() || HEX_PRI_KEY_LEN == str_pri.size();
}
bool check_account_address(const std::string& account)
{
    return top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_user_account ||
        top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account;
}

xJson::Value ApiMethod::get_response_from_daemon() {
    std::string daemon_host = "127.0.0.1:7000";
    auto safebox_endpoint = getenv(SAFEBOX_ENDPOINT_ENV.c_str());
    if (safebox_endpoint != NULL) {
        daemon_host = safebox_endpoint;
    }
    HttpClient client(daemon_host);
    xJson::Value j;
    j["method"] = "get";
    std::string token_request = j.toStyledString();
    string token_response_str;
    try {
        SimpleWeb::CaseInsensitiveMultimap header;
        header.insert({"Content-Type", "application/json"});
        auto token_response = client.request("POST", "/api/safebox", token_request, header);
        token_response_str = token_response->content.string();
    } catch (std::exception & e) {
        cout << e.what() << endl;
        return "";
    }
    xJson::Reader reader;
    xJson::Value token_response_json;
    if (!reader.parse(token_response_str, token_response_json)) {
        cout << "json parse error" << endl;
    } else if (token_response_json["status"].asString() != "ok") {
#ifdef DEBUG
        cout << "[debug]" << token_response_json["error"].asString() << endl;
#endif
    }

    return token_response_json;
}

string ApiMethod::get_account_from_daemon() {
    auto token_response_json = get_response_from_daemon();
    try {
        return token_response_json["account"].asString();
    } catch (...) {
        return "";
    }
}
int ApiMethod::get_eth_file(std::string& account) {
    if (top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        std::transform(account.begin() + 1, account.end(), account.begin() + 1, ::tolower);
    std::vector<std::string> files = xChainSDK::xcrypto::scan_key_dir(g_keystore_dir);
    for (int i = 0; i < (int)files.size(); i++)
    {
        std::string file = files[i];
        std::transform(file.begin() + 1, file.end(), file.begin() + 1, ::tolower);
        if (file == account)
        {
            account = files[i];
            break;
        }
    }
    return 0;
}
string ApiMethod::get_prikey_from_daemon(std::ostringstream & out_str) {
    auto token_response_json = get_response_from_daemon();
    try {
        auto hex_ed_key = token_response_json["private_key"].asString();
        auto account = token_response_json["account"].asString();
        string base64_pri;
        if (!hex_ed_key.empty()) {
            if (top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
                get_eth_file(account);
            std::string path = g_keystore_dir + '/' + account;
            base64_pri = decrypt_keystore_by_key(hex_ed_key, path);
        }
        return base64_pri;
    } catch (...) {
        return "";
    }
}

int ApiMethod::set_prikey_to_daemon(const string & account, const string & pri_key, std::ostringstream & out_str, uint32_t expired_time) {
    std::string daemon_host = "127.0.0.1:7000";
    auto safebox_endpoint = getenv(SAFEBOX_ENDPOINT_ENV.c_str());
    if (safebox_endpoint != NULL) {
        daemon_host = safebox_endpoint;
    }
    std::string account_temp(account);
    if (top::base::xvaccount_t::get_addrtype_from_account(account_temp) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        std::transform(account_temp.begin() + 1, account_temp.end(), account_temp.begin() + 1, ::tolower);

    HttpClient client(daemon_host);
    xJson::Value j;
    j["method"] = "set";
    j["account"] = account_temp;
    j["private_key"] = pri_key;
    j["expired_time"] = expired_time;
    std::string token_request = j.toStyledString();
    string token_response_str;
    try {
        SimpleWeb::CaseInsensitiveMultimap header;
        header.insert({"Content-Type", "application/json"});
        auto token_response = client.request("POST", "/api/safebox", token_request, header);
        token_response_str = token_response->content.string();
    } catch (std::exception & e) {
        out_str << e.what() << endl;
        return 1;
    }
    xJson::Reader reader;
    xJson::Value token_response_json;
    if (!reader.parse(token_response_str, token_response_json)) {
        out_str << "connection error" << endl;
        return 1;
    } else if (token_response_json["status"].asString() == "fail") {
        out_str << token_response_json["error"].asString() << endl;
        return 1;
    }
    return 0;
}

bool ApiMethod::set_default_prikey(std::ostringstream & out_str, const bool is_query) {
    if (g_userinfo.account.size() == 0) {
        std::string str_pri = get_prikey_from_daemon(out_str);
        if (!str_pri.empty()) {
            set_g_userinfo(str_pri);
        } else {
            std::vector<std::string> files = xChainSDK::xcrypto::scan_key_dir(g_keystore_dir);
            std::vector<std::string> accounts;
            for (auto file : files) {
                if (check_account_address(file))
                    accounts.push_back(file);
            }
            if (accounts.size() < 1) {
                cout << "You do not have a TOP account, please create an account." << std::endl;
                return false;
            }
            if (accounts.size() > 1) {
                cout << "There are multiple accounts in your wallet, please set a default account first." << std::endl;
                return false;
            }
            if (is_query) {
                g_userinfo.account = accounts[0];
                return true;
            }
            // cout << "set_default_prikey:" <<accounts[0] <<endl;
            std::string str_pri = xChainSDK::xcrypto::import_existing_keystore(cache_pw, g_keystore_dir + "/" + accounts[0]);
            if (str_pri.empty()) {
                //cout << "Please set a default account by command `topio wallet setDefaultAccount`." << std::endl;
                return false;
            }
            if (xChainSDK::xcrypto::set_g_userinfo(str_pri)) {
#ifdef DEBUG
                cout << "[debug]" << g_userinfo.account << " has been set as the default account." << std::endl;
#endif
            } else {
               // cout << "Please set a default account by command `topio wallet setDefaultAccount`." << std::endl;
                return false;
            }
        }
    }
    return true;
}

void ApiMethod::tackle_null_query(std::ostringstream & out_str, std::string null_out) {
    auto tmp = out_str.str();
    xJson::Value jv;
    xJson::Reader reader;
    if (reader.parse(tmp, jv)) {
        auto data = jv["data"];
        if (data.isNull() || (data.isMember("value") && data["value"].isNull())) {
            out_str.str("");
            auto err_no = jv["errno"].asInt();
            auto errmsg = jv["errmsg"].asString();
            if (errmsg == "OK" && err_no == 0) {
                if (null_out.empty()) {
                    cout << "No data!" << endl;
                } else {
                    cout << null_out << endl;
                }
            } else {
                cout << "[" << err_no << "]Errmsg: " << errmsg << endl;
            }
        }
    }
}

void ApiMethod::tackle_send_tx_request(std::ostringstream & out_str) {
    auto result = out_str.str();
    xJson::Value jv;
    xJson::Reader reader;
    if (reader.parse(result, jv)) {
        auto tx_hash = jv["tx_hash"].asString();
        if (!tx_hash.empty()) {
            out_str.str("");
            cout << "Transaction hash: " << tx_hash << endl;
            cout << "Please use command 'topio querytx' to query transaction status later on!!!" << endl;
        }
    }
}

int ApiMethod::update_account(std::ostringstream & out_str) {
    xJson::Value root;
    return update_account(out_str, root);
}

int ApiMethod::update_account(std::ostringstream & out_str, xJson::Value & root) {
    if (!set_default_prikey(out_str)) {
        return 1;
    }

    get_token();

    std::ostringstream oss;
    if (get_account_info(oss, root) != 0) {
        return 1;
    }

    return 0;
}

int ApiMethod::set_pw_by_file(const string & pw_path, string & pw) {
    if (!pw_path.empty()) {
        std::ifstream pw_file(pw_path, std::ios::in);
        if (!pw_file) {
            cout << pw_path << " Open Error!" << std::endl;
            return 1;
        }

        std::getline(pw_file, pw);
    }
    return 0;
}

void ApiMethod::create_account(const int32_t & pf, const string & pw_path, std::ostringstream & out_str) {
    is_account = true;
    cache_pw = empty_pw;
    if (pf == 1) {
        if (!check_password()) {
            return;
        }
    }

    if (set_pw_by_file(pw_path, cache_pw) != 0) {
        return;
    }

    std::string dir;
    auto path = create_new_keystore(cache_pw, dir);
    out_str << "Successfully create an account locally!\n" << std::endl;

    out_str << "Account Address: " << g_userinfo.account << std::endl;
    out_str << "Owner public-Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << "\n\n";
    out_str << "You can share your public key and account address with anyone.Others need them to interact with you!" << std::endl;
    out_str << "You must nerver share the private key or account keystore file with anyone!They control access to your funds!" << std::endl;
    out_str << "You must backup your account keystore file!Without the file, you will not be able to access the funds in your account!" << std::endl;
    out_str << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;

    return;
}

void ApiMethod::create_key(std::string & owner_account, const int32_t & pf, const string & pw_path, std::ostringstream & out_str) {
    cache_pw = empty_pw;
    if (1 == pf) {
        if (!check_password()) {
            return;
        }
    }

    if (set_pw_by_file(pw_path, cache_pw) != 0) {
        return;
    }

    if (owner_account.size() == 0) {
        if (!set_default_prikey(out_str)) {
            out_str << "Create worker key failed, you should specify an account address, or set a default account." << endl;
            return;
        } else {
            owner_account = g_userinfo.account;
        }
    }

    std::string dir = "";
    auto path = create_new_keystore(cache_pw, dir, true, owner_account);
    out_str << "Successfully create an worker keystore file!\n" << std::endl;
    out_str << "Account Address: " << owner_account << std::endl;
    out_str << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << "\n\n";
    out_str << "You can share your public key with anyone.Others need it to interact with you!" << std::endl;
    out_str << "You must nerver share the private key or keystore file with anyone!They can use them to make the node malicious." << std::endl;
    out_str << "You must backup your keystore file!Without the file,you may not be able to send transactions." << std::endl;
    out_str << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    return;
}

std::unordered_map<std::string, std::string> ApiMethod::queryNodeInfos() {
    std::ostringstream oss;
    auto tmp = g_userinfo.account;
    g_userinfo.account = top::sys_contract_rec_registration_addr;
    std::string target = "";  // emtpy target means query all node infos
    api_method_imp_.queryNodeInfo(g_userinfo, target, oss);
    g_userinfo.account = tmp;

    std::unordered_map<std::string, std::string> node_infos;
    xJson::Reader reader;
    xJson::Value root;
    if (reader.parse(oss.str(), root)) {
        for (auto a : root["data"].getMemberNames()) {
            node_infos[a] = root["data"][a]["node_sign_key"].asString();
        }
    }
    return node_infos;
}

void ApiMethod::list_accounts(std::ostringstream & out_str) {
    auto account = get_account_from_daemon();
    if (account.empty()) {
#ifdef DEBUG
        out_str << "[debug]No default account" << std::endl;
#endif
    } else {
        g_userinfo.account = account;
    }
    std::vector<std::string> keys = scan_key_dir(g_keystore_dir);
    if (keys.size() == 0) {
        out_str << "There is no account in wallet." << std::endl;
        return;
    }
    std::reverse(keys.begin(), keys.end());

    std::vector<std::string> av;
    if (!g_userinfo.account.empty()) {
        av.push_back(g_userinfo.account);
    }

    struct account_info {
        std::string owner_key;
        std::vector<std::string> worker_keys;
    };
    std::unordered_map<std::string, account_info> aim;
    for (size_t i = 0; i < keys.size(); ++i) {
        auto path = g_keystore_dir + "/" + keys[i];
        auto key_info = attach_parse_keystore(path, out_str);
        std::string account = key_info["account_address"].asString();
        if (account.empty())
            account = key_info["account address"].asString();
        if (top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
            std::transform(account.begin() + 1, account.end(), account.begin() + 1, ::tolower);

        if (std::find(av.begin(), av.end(), account) == av.end())
        {
            av.push_back(account);
        }

        if (key_info["key_type"].asString() == "owner") {
            aim[account].owner_key = key_info["public_key"].asString();
        } else {
            aim[account].worker_keys.push_back(key_info["public_key"].asString());
        }
    }

    // map of account & corresponding sign public key
    auto node_infos = queryNodeInfos();
    size_t i = 0;
    for (auto account : av) {
        auto ai = aim[account];
        std::string status;
        if (account.size() > 0 && account == g_userinfo.account) {
            status = " [default account]";
        }

        out_str << "account #" << i++ << ": " << account << status << std::endl;

        if (ai.owner_key.size() != 0) {
            out_str << "owner public-key: " << ai.owner_key;
            if (ai.owner_key == node_infos[account]) {
                out_str << " [registered as minerkey]";
            }
            out_str << std::endl;
        }
        outAccountBalance(account, out_str);

        size_t j = 0;
        for (size_t j = 0; j < ai.worker_keys.size(); ++j) {
            if (0 == j) {
                out_str << "worker public-key list:" << std::endl;
            }
            out_str << "public key #" << j << ": " << ai.worker_keys[j];
            if (ai.worker_keys[j] == node_infos[account]) {
                out_str << " [registered as minerkey]";
            }
            out_str << std::endl;
        }
        if (i != av.size()) {
            out_str << "------------------------------------------------------\n\n";
        }
    }
    return;
}

void ApiMethod::set_default_account(const std::string & account, const string & pw_path, std::ostringstream & out_str) {
    std::string account_file(account);
    if (top::base::xvaccount_t::get_addrtype_from_account(account_file) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        get_eth_file(account_file);
    const std::string store_path = g_keystore_dir + "/" + account_file;
    std::fstream store_file;
    store_file.open(store_path, std::ios::in);
    if (!store_file) {
        out_str << "The account's owner keystore file does not exist in wallet." << std::endl;
        return;
    }

    std::string ed_key;
    std::string pw = empty_pw;
    if (set_pw_by_file(pw_path, pw) != 0) {
        return;
    }
    ed_key = get_symmetric_ed_key(pw, store_path);
    std::string str_pri = import_existing_keystore(pw, store_path, true);
    if (str_pri.empty()) {
        std::cout << "Please Input Password." << std::endl;
        pw = input_hiding();
        ed_key = get_symmetric_ed_key(pw, store_path);
        str_pri = import_existing_keystore(pw, store_path);
    }

    if (!str_pri.empty() && set_prikey_to_daemon(account, ed_key, out_str) == 0) {
        out_str << "Set default account successfully." << std::endl;
    } else {
        out_str << "Set default account failed." << std::endl;
    }
    return;
}

void ApiMethod::reset_keystore_password(std::string & public_key, std::ostringstream & out_str) {
    bool found = false;
    std::string path = g_keystore_dir + "/";
    std::vector<std::string> keys = scan_key_dir(g_keystore_dir);
    for (size_t i = 0; i < keys.size(); ++i) {
        auto tmp_path = path + keys[i];
        auto key_info = parse_keystore(tmp_path);
        if (key_info["public_key"] == public_key) {
            found = true;
            path = tmp_path;
            break;
        }
    }
    if (!found) {
        cout << "No file with public_key " << public_key << endl;
        return;
    }

    std::cout << "Please Input Old Password. If the keystore has no password, press Enter directly." << std::endl;
    auto pw = input_hiding();
    if (0 == pw.size()) {
        pw = empty_pw;
    }

    auto new_pw = reset_keystore_pw(pw, path);
    if (!new_pw.empty()) {
        std::ostringstream oss;
        auto default_account = get_account_from_daemon();
        if (g_userinfo.account == default_account) {
            std::string ed_key = get_symmetric_ed_key(new_pw, path);
            if (set_prikey_to_daemon(default_account, ed_key, oss) == 0) {
                std::cout << "Reset default account successfully." << std::endl;
            } else {
                std::cout << "Reset default account failed." << std::endl;
            }
        }
    }
}

void ApiMethod::import_account(const int32_t & pf, std::ostringstream & out_str) {
    is_account = true;
    cache_pw = empty_pw;
    if (pf == 1) {
        if (!check_password()) {
            return;
        }
    }
    std::string pri_str;
    if (input_pri_key(pri_str) != 0)
        return;

    std::string dir;
    std::string path = create_new_keystore(cache_pw, dir, pri_str);
    if (path.empty())
        return;

    out_str << "Import successfully.\n" << std::endl;
    out_str << "Account Address: " << g_userinfo.account << std::endl;
    out_str << "Public-Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << "\n\n";
    return;
}

void ApiMethod::export_account(const std::string & account, std::ostringstream & out_str) {
    if (account.empty()) {
    /*    std::string account_temp = get_account_from_daemon();
        if (!account_temp.empty())
            g_userinfo.account = account_temp;
        else { */
            cache_pw = empty_pw;
            if (!set_default_prikey(out_str))
            {
                std::cout << "Please Input Password." << std::endl;
                cache_pw = input_hiding();
                if (!set_default_prikey(out_str))
                    return;
            }
     //   }
    } else {
        g_userinfo.account = account;
    }

    if (g_userinfo.account.empty())
    {
        out_str << "Please input account address." << std::endl;
        return;
    }
    std::vector<std::string> keys = scan_key_dir(g_keystore_dir);
    if (keys.size() == 0) {
        out_str << "There is no account in wallet." << std::endl;
        return;
    }
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] != g_userinfo.account)
            continue;
        std::string pw = cache_pw;
        std::string keystore_file = g_keystore_dir + "/" + keys[i];
        std::string str_pri = import_existing_keystore(pw, keystore_file, true);
        if (str_pri.empty())
        {
            std::cout << "Please Input Password." << std::endl;
            pw = input_hiding();

            str_pri = import_existing_keystore(pw, keystore_file);
            if (str_pri.empty()) {
                out_str << "Password error！" << std::endl;
                return;
            }
        }
        out_str << "Export successfully.\n" << std::endl;
        out_str << "Keystore file: " << keystore_file << std::endl;
        out_str << "Account Address: " << g_userinfo.account << std::endl;
        out_str << "Private-Key: " << str_pri << "\n\n";

        std::ifstream keyfile(keystore_file, std::ios::in);
        if (!keyfile)
        {
             return;
        }
        out_str << keyfile.rdbuf() << std::endl;
        return;
    }
    out_str << "Account address error! The account does not exist." << std::endl;
    return;
}

int ApiMethod::set_default_miner(const std::string & pub_key, const std::string & pw_path, std::ostringstream & out_str) {
    if (g_keystore_dir.empty() || g_data_dir.empty()) {
        out_str << "invalid keystore path" << std::endl;
        return -1;
    }
    std::vector<std::string> files = xChainSDK::xcrypto::scan_key_dir(g_keystore_dir);
    std::string target_kf;
    std::string target_node_id;
    for (const auto & kf : files) {
        auto kf_path = g_keystore_dir + "/" + kf;

        xJson::Value key_info_js;
        std::ifstream keyfile(kf_path, std::ios::in);
        if (!keyfile) {
            out_str << "open keystore file:" << kf_path << " failed" << std::endl;
            continue;
        }
        std::stringstream buffer;
        buffer << keyfile.rdbuf();
        keyfile.close();
        std::string key_info = buffer.str();
        xJson::Reader reader;
        if (!reader.parse(key_info, key_info_js)) {
            out_str << "parse keystore file:" << kf_path << " failed" << std::endl;
            continue;
        }

        for (const auto & name : key_info_js.getMemberNames()) {
            if (name == "public_key") {
                if (pub_key == key_info_js[name].asString()) {
                    target_kf = kf_path;
                }
            }
            if (name == "account address" || name == "account_address") {
                target_node_id = key_info_js[name].asString();
                if (top::base::xvaccount_t::get_addrtype_from_account(target_node_id) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
                    std::transform(target_node_id.begin() + 1, target_node_id.end(), target_node_id.begin() + 1, ::tolower);
            }
        }  // end for (const auto & name...
        if (!target_kf.empty()) {
            break;
        }
    }  // end for (const auto &kf...

    if (target_kf.empty()) {
        out_str << "The key does not exist in wallet." << std::endl;
        return -1;
    }

    std::string base64_pri;
    std::string hex_pri_token;
    std::string pw = empty_pw;
    if (set_pw_by_file(pw_path, pw) != 0) {
        return -1;
    }
    // do not store private_key directly, using pri_token instend
    hex_pri_token = get_symmetric_ed_key(pw, target_kf);
    base64_pri = import_existing_keystore(pw, target_kf, true);
    if (base64_pri.empty()) {
        std::cout << "Please Input Password." << std::endl;
        pw = input_hiding();
        hex_pri_token = get_symmetric_ed_key(pw, target_kf);
        base64_pri = import_existing_keystore(pw, target_kf);
    }
    if (base64_pri.empty() || hex_pri_token.empty()) {
        out_str << "decrypt private token failed" << std::endl;
        out_str << "Set miner key failed." << std::endl;
        return -1;
    }

    std::string extra_config = g_data_dir + "/.extra_conf.json";
    xJson::Value key_info_js;
    std::ifstream keyfile(extra_config, std::ios::in);
    if (keyfile) {
        std::stringstream buffer;
        buffer << keyfile.rdbuf();
        keyfile.close();
        std::string key_info = buffer.str();
        xJson::Reader reader;
        // ignore any error when parse
        reader.parse(key_info, key_info_js);
    }

    key_info_js["default_miner_node_id"] = target_node_id;
    key_info_js["default_miner_public_key"] = pub_key;
    key_info_js["default_miner_keystore"] = target_kf;
    // key_info_js["default_miner_private_key"] = base64_pri_key;
    // key_info_js["default_miner_private_key"] = hex_pri_token;

    // expired_time = 0 meaning nerver expired
    // if (set_prikey_to_daemon(target_node_id, base64_pri_key, out_str, 0) != 0) {
    // if (set_prikey_to_daemon(target_node_id, hex_pri_token, out_str, 0) != 0) {
    if (set_prikey_to_daemon(pub_key, hex_pri_token, out_str, 0) != 0) {
        // if set worker key target_node_id, will confilict with topcl(account)
        out_str << "keep default miner nfo in cache failed" << std::endl;
        out_str << "Set miner key failed." << std::endl;
        return -1;
    }

    // dump new json to file
    xJson::StyledWriter new_sw;
    std::ofstream os;
    os.open(extra_config);
    if (!os.is_open()) {
        out_str << "dump file failed, Set miner key failed" << std::endl;
        out_str << "Set miner key failed." << std::endl;
        return -1;
    }
    os << new_sw.write(key_info_js);
    os.close();
    out_str << "Set miner key successfully." << std::endl;
    return 0;
}

void ApiMethod::create_chain_account(std::ostringstream & out_str) {
    cache_pw = " "; // the password of create account is fixed to be one whitespace

    if (!set_default_prikey(out_str)) {
        return;
    }
    get_token();

    api_method_imp_.create_account(g_userinfo);
}

void ApiMethod::import_key(std::string & base64_pri, std::ostringstream & out_str) {
    set_g_userinfo(base64_pri);
    string dir = "";
    auto path = create_new_keystore(empty_pw, dir, base64_pri, false);
    std::cout << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    std::cout << "Keystore File Path: " << path << std::endl;
    auto ed_key = get_symmetric_ed_key(empty_pw, path);
    if (set_prikey_to_daemon(g_userinfo.account, ed_key, out_str) == 0) {
        out_str << g_userinfo.account << ": Import private key successfully." << std::endl;
    } else {
        out_str << "Import private key failed." << std::endl;
    }
}

void ApiMethod::transfer1(std::string & to, std::string & amount_d, std::string & note, std::string & tx_deposit_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    if (!check_account_address(to))
    {
        cout << "Invalid transfer account address." <<endl;
        return;
    }
    std::string from = g_userinfo.account;
    if (top::base::xvaccount_t::get_addrtype_from_account(to) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        std::transform(to.begin() + 1, to.end(), to.begin() + 1, ::tolower);

    if (note.size() > 128) {
        std::cout << "note size: " << note.size() << " > maximum size 128" << endl;
        return;
    }

    uint64_t amount;  // = ASSET_TOP(amount_d);
    if (parse_top_double(amount_d, TOP_UNIT_LENGTH, amount) != 0)
        return;
    uint64_t tx_deposit;  // = ASSET_TOP(tx_deposit_d);
    if (parse_top_double(tx_deposit_d, TOP_UNIT_LENGTH, tx_deposit) != 0)
        return;

    if (tx_deposit != 0) {
        api_method_imp_.set_tx_deposit(tx_deposit);
    }
    api_method_imp_.transfer(g_userinfo, from, to, amount, note, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::query_tx(std::string & account, std::string & tx_hash, std::ostringstream & out_str) {
    if (account.empty()) {
        if (!set_default_prikey(out_str)) {
            out_str << "No default account found, you should specify an account address, or set a default account." << endl;
            return;
        }
    } else {
        g_userinfo.account = account;
    }
    api_method_imp_.getTransaction(g_userinfo, g_userinfo.account, tx_hash, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::query_miner_info(std::string & account, std::ostringstream & out_str) {
    if (account.empty()) {
        if (!set_default_prikey(out_str)) {
            out_str << "No default account found, you should specify an account address, or set a default account." << endl;
            return;
        }
    } else {
        g_userinfo.account = account;
    }
    if (top::base::xvaccount_t::get_addrtype_from_account(g_userinfo.account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        std::transform(g_userinfo.account.begin() + 1, g_userinfo.account.end(), g_userinfo.account.begin() + 1, ::tolower);
    auto rtn = api_method_imp_.getStandbys(g_userinfo, g_userinfo.account);
    if (rtn) {
        api_method_imp_.getElectInfo(g_userinfo, g_userinfo.account);
    }
    api_method_imp_.queryNodeInfo(g_userinfo, g_userinfo.account, out_str);
    string null_out = g_userinfo.account + " account has not registered miner.";
    tackle_null_query(out_str, null_out);
}
int ApiMethod::parse_top_double(const std::string &amount, const uint32_t unit, uint64_t &out)
{
    if (!std::all_of(amount.begin(), amount.end(), [](char c) { return (c <= '9' && c >= '0') || c == '.'; }))
    {
        cout << "Data format is invalid." << endl;
        return 1;
    }
    int dot_count = count_if(amount.begin(), amount.end(), [](char c) { return c == '.'; });
    if (dot_count > 1)
    {
        cout << "Data format is invalid." << endl;
        return 1;
    }
    std::string input = amount;
    bool dot_found{false};
    size_t i{0};
    out = 0;
    for (; i < (size_t)std::min(TOP_MAX_LENGTH + 1, (int)input.size()); ++i)
    {
        if (input[i] == '.')
        {
            dot_found = true;
            break;
        }
        out *= 10;
        out += input[i] - '0';
    }
    if (i == input.size())
    {
        out *= (uint32_t)std::pow(10, unit);
        return 0; // parse finished
    }
    if (!dot_found)
    {
        cout << "Data format is invalid." << endl;
        return 1;
    }

    ++i;
    for (auto j = 0; j < (int)unit; ++j, ++i)
    {
        if (i >= input.size())
        {
            out = out * (uint32_t)std::pow(10, unit - j);
            return 0;
        }
        out *= 10;
        out += input[i] - '0';
    }

    return 0;
}
void ApiMethod::register_node(const std::string & mortgage_d,
                              const std::string & role,
                              const std::string & nickname,
                              const uint32_t & dividend_rate,
                              std::string & signing_key,
                              std::ostringstream & out_str) {
    std::ostringstream res;
    xJson::Value root;
    if (update_account(res, root) != 0) {
        return;
    }
    if (signing_key.empty()) {
        signing_key = top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key);
    } else {
        std::vector<std::string> keys = scan_key_dir(g_keystore_dir);
        bool found = false;
        for (size_t i = 0; i < keys.size(); ++i) {
            auto path = g_keystore_dir + "/" + keys[i];
            auto key_info = parse_keystore(path);
            auto public_key = key_info["public_key"].asString();
            if (public_key == signing_key) {
                found = true;
                std::string account = key_info["account_address"].asString();
                if (account.empty())
                    account = key_info["account address"].asString();
                if (top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
                    std::transform(account.begin()+1, account.end(), account.begin()+1, ::tolower);
                if (account == g_userinfo.account) {
                    break;
                } else {
                    cout << "The miner_key does not match default account in wallet." << endl;
                    return;
                }
            }
        }
        if (!found) {
            cout << "No file with public_key " << signing_key << endl;
            return;
        }
    }

    auto node_infos = queryNodeInfos();
    if (node_infos.find(g_userinfo.account) != node_infos.end()) {
        cout << "Register miner failed." << endl;
        cout << g_userinfo.account << " is already a miner." << endl;
        return;
    }

    uint64_t mortgage;
    if (parse_top_double(mortgage_d, TOP_UNIT_LENGTH, mortgage) != 0)
        return;
    api_method_imp_.registerNode(g_userinfo, mortgage, role, nickname, signing_key, dividend_rate, out_str);
    auto result = out_str.str();
    xJson::Reader reader;
    string tx_hash;
    if (!reader.parse(result, root)) {
        cout << result << endl;
        return;
    } else {
        tx_hash = root["tx_hash"].asString();
    }
    cout << "Miner account address: " << g_userinfo.account << endl;
    cout << "Miner public_key: " << signing_key << endl;
    tackle_send_tx_request(out_str);

    const uint16_t query_interval = 30;
    size_t cnt = 1;
    while (1) {
        cout << "Miner registering..., " << cnt++ << endl;
        sleep(query_interval);
        std::ostringstream oss;
        api_method_imp_.getTransaction(g_userinfo, g_userinfo.account, tx_hash, oss);
        auto result = oss.str();
        xJson::Reader reader;
        string tx_hash;
        if (!reader.parse(result, root)) {
            cout << result << endl;
        } else {
            auto tx_state = root["data"]["tx_consensus_state"]["confirm_block_info"]["exec_status"].asString();
            if (tx_state == "success") {
                cout << "Successfully registering to the mining pool." << endl;
                return;
            } else if (tx_state == "failure") {
                cout << "Register miner failed." << endl;
                return;
            }
        }
    }
}

void ApiMethod::query_miner_reward(std::string & target, std::ostringstream & out_str) {
    if (target.empty()) {
        if (!set_default_prikey(out_str, true)) {
            return;
        }
        target = g_userinfo.account;
    } else {
        g_userinfo.account = target;
    }
    api_method_imp_.queryNodeReward(g_userinfo, target, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::claim_miner_reward(std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    api_method_imp_.claimNodeReward(g_userinfo, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::set_dividend_ratio(const uint32_t & dividend_rate, const std::string & tx_deposit_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t tx_deposit; // = ASSET_TOP(tx_deposit_d);
    if (parse_top_double(tx_deposit_d, TOP_UNIT_LENGTH, tx_deposit) != 0)
        return;
    if (tx_deposit != 0) {
        api_method_imp_.set_tx_deposit(tx_deposit);
    }
    api_method_imp_.setDividendRatio(g_userinfo, dividend_rate, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::set_miner_name(std::string & name, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    api_method_imp_.setNodeName(g_userinfo, name, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::change_miner_type(std::string & role, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    api_method_imp_.updateNodeType(g_userinfo, role, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::unregister_node(std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    auto node_infos = queryNodeInfos();
    if (node_infos.find(g_userinfo.account) == node_infos.end()) {
        cout << "Terminate failed." << endl;
        cout << g_userinfo.account << " is not a miner now." << endl;
        return;
    }
    api_method_imp_.unRegisterNode(g_userinfo, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::update_miner_info(const std::string & role,
                                  const std::string & name,
                                  const uint32_t & updated_deposit_type,
                                  const std::string & node_deposit_d,
                                  const uint32_t & dividend_rate,
                                  const std::string & node_sign_key,
                                  std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }

    std::vector<std::string> keys = scan_key_dir(g_keystore_dir);
    bool found = false;
    for (size_t i = 0; i < keys.size(); ++i) {
        auto path = g_keystore_dir + "/" + keys[i];
        auto key_info = parse_keystore(path);
        if (key_info["public_key"] == node_sign_key) {
            found = true;
            std::string account = key_info["account_address"].asString();
            if (account.empty())
                account = key_info["account address"].asString();

            if (account == g_userinfo.account) {
                break;
            } else {
                cout << "The miner key does not match default account in wallet." << endl;
                return;
            }
        }
    }
    if (!found) {
        cout << "No file with public_key " << node_sign_key << endl;
        return;
    }

    uint64_t node_deposit;  // = ASSET_TOP(node_deposit_d);
    if (parse_top_double(node_deposit_d, TOP_UNIT_LENGTH, node_deposit) != 0)
        return;
    api_method_imp_.updateNodeInfo(g_userinfo, role, name, updated_deposit_type, node_deposit, dividend_rate, node_sign_key, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::add_deposit(const std::string & deposit_d, std::ostringstream & out_str) {
    std::ostringstream res;
    xJson::Value root;
    if (update_account(res, root) != 0) {
        return;
    }

    uint64_t deposit;  // = ASSET_TOP(deposit_d);
    if (parse_top_double(deposit_d, TOP_UNIT_LENGTH, deposit) != 0)
        return;
    api_method_imp_.stake_node_deposit(g_userinfo, deposit, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::reduce_deposit(const std::string & deposit_d, std::ostringstream & out_str) {
    cout << "Do you confirm your remaining miner deposit after reducing is still enough for the miner." << endl;
    cout << "Comfirmed: Y or y" << endl;
    cout << "Unconfirmed：Any character other than Y/y" << endl;
    string si;
    cin >> si;
    if (si != "Y" && si != "y") {
        return;
    }
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t deposit;  // = ASSET_TOP(deposit_d);
    if (parse_top_double(deposit_d, TOP_UNIT_LENGTH, deposit) != 0)
        return;
    api_method_imp_.unstake_node_deposit(g_userinfo, deposit, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::withdraw_deposit(std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }

    auto node_infos = queryNodeInfos();
    if (node_infos.find(g_userinfo.account) != node_infos.end()) {
        cout << "Withdraw failed." << endl;
        cout << g_userinfo.account << " miner has not terminated mining." << endl;
        return;
    }

    api_method_imp_.redeemNodeDeposit(g_userinfo, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::query_account(std::string & target, std::ostringstream & out_str) {
    if (target.size() == 0) {
        if (!set_default_prikey(out_str)) {
            return;
        }
    } else {
        g_userinfo.account = target;
        if (top::base::xvaccount_t::get_addrtype_from_account(g_userinfo.account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
            std::transform(g_userinfo.account.begin()+1, g_userinfo.account.end(), g_userinfo.account.begin()+1, ::tolower);
    }
    api_method_imp_.getAccount(g_userinfo, g_userinfo.account, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::query_block(std::string & target, std::string & height, std::ostringstream & out_str) {
    if (target.size() == 0) {
        if (!set_default_prikey(out_str)) {
            return;
        }
    } else {
        g_userinfo.account = target;
    }
    if (height != "latest") {
        try {
            auto hi = std::stoull(height);
        } catch (...) {
            cout << "Could not convert: height=" << height << endl;
            cout << "Parameter 'height' should be an integer or 'latest'." << endl;
            return;
        }
    }
    api_method_imp_.getBlock(g_userinfo, g_userinfo.account, height, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::chain_info(std::ostringstream & out_str) {
    api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
    api_method_imp_.getChainInfo(g_userinfo, out_str);
}

void ApiMethod::deploy_contract(const uint64_t & tgas_limit, const std::string & amount_d, const std::string & code_path, const std::string & tx_deposit_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    ifstream code_file(code_path);
    ostringstream tmp;
    tmp << code_file.rdbuf();
    string code = tmp.str();

    uint64_t amount;  // = ASSET_TOP(amount_d);
    if (parse_top_double(amount_d, TOP_UNIT_LENGTH, amount) != 0)
        return;
    uint64_t tx_deposit;  // = ASSET_TOP(tx_deposit_d);
    if (parse_top_double(tx_deposit_d, TOP_UNIT_LENGTH, tx_deposit) != 0)
        return;
    if (tx_deposit != 0) {
        api_method_imp_.set_tx_deposit(tx_deposit);
    }
    api_method_imp_.deployContract(g_userinfo, g_userinfo.contract.account, tgas_limit, amount, code, out_str);
    cout << "contract account: " << g_userinfo.contract.account << std::endl;
    tackle_send_tx_request(out_str);
}

void ApiMethod::call_contract(const std::string & amount_d,
                              const string & addr,
                              const std::string & func,
                              const string & params,
                              const std::string & tx_deposit_d,
                              std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t amount;  // = ASSET_TOP(amount_d);
    if (parse_top_double(amount_d, TOP_UNIT_LENGTH, amount) != 0)
        return;
    uint64_t tx_deposit;  // = ASSET_TOP(tx_deposit_d);
    if (parse_top_double(tx_deposit_d, TOP_UNIT_LENGTH, tx_deposit) != 0)
        return;
    if (tx_deposit != 0) {
        api_method_imp_.set_tx_deposit(tx_deposit);
    }
    api_method_imp_.runContract(g_userinfo, amount, addr, func, params, out_str);
    tackle_send_tx_request(out_str);
}

/*
govern
*/
void ApiMethod::get_proposal(std::string & target, std::ostringstream & out_str) {
    api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
    api_method_imp_.queryProposal(g_userinfo, target, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::cgp(std::ostringstream & out_str) {
    api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
    std::string target;
    api_method_imp_.getCGP(g_userinfo, target, out_str);
}

void ApiMethod::submit_proposal(uint8_t & type,
                                const std::string & target,
                                const std::string & value,
                                std::string & deposit_d,
                                uint64_t & effective_timer_height,
                                std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t deposit; // = ASSET_TOP(deposit_d);
    if (parse_top_double(deposit_d, TOP_UNIT_LENGTH, deposit) != 0)
        return;
    api_method_imp_.submitProposal(g_userinfo, type, target, value, deposit, effective_timer_height, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::withdraw_proposal(const std::string & proposal_id, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    api_method_imp_.withdrawProposal(g_userinfo, proposal_id, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::tcc_vote(const std::string & proposal_id, const std::string & opinion, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    bool option = false;
    if (opinion == "true" || opinion == "TRUE") {
        option = true;
    }
    api_method_imp_.tccVote(g_userinfo, proposal_id, option, out_str);
    tackle_send_tx_request(out_str);
}

/*
resource
*/
void ApiMethod::stake_for_gas(std::string & amount_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t amount; // = ASSET_TOP(amount_d);
    if (parse_top_double(amount_d, TOP_UNIT_LENGTH, amount) != 0)
        return;
    api_method_imp_.stakeGas(g_userinfo, g_userinfo.account, g_userinfo.account, amount, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::withdraw_fund(std::string & amount_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t amount; // = ASSET_TOP(amount_d);
    if (parse_top_double(amount_d, TOP_UNIT_LENGTH, amount) != 0)
        return;
    api_method_imp_.unStakeGas(g_userinfo, g_userinfo.account, g_userinfo.account, amount, out_str);
    tackle_send_tx_request(out_str);
}

/*
staking
*/
void ApiMethod::stake_fund(uint64_t & amount, uint16_t & lock_duration, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    api_method_imp_.stakeVote(g_userinfo, amount, lock_duration, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::stake_withdraw_fund(uint64_t & amount, const std::string & tx_deposit_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    uint64_t tx_deposit;  // = ASSET_TOP(tx_deposit_d);
    if (parse_top_double(tx_deposit_d, TOP_UNIT_LENGTH, tx_deposit) != 0)
        return;
    if (tx_deposit != 0) {
        api_method_imp_.set_tx_deposit(tx_deposit);
    }
    api_method_imp_.unStakeVote(g_userinfo, amount, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::vote_miner(std::vector<std::pair<std::string, int64_t>> & vote_infos, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    std::map<std::string, int64_t> votes;
    for (auto p : vote_infos) {
        std::string account = p.first;
        if (top::base::xvaccount_t::get_addrtype_from_account(account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
            std::transform(account.begin()+1, account.end(), account.begin()+1, ::tolower);
        votes[account] = p.second;
        cout << account << " " << p.second << endl;
    }
    api_method_imp_.voteNode(g_userinfo, votes, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::withdraw_votes(std::vector<std::pair<std::string, int64_t>> & vote_infos, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    std::map<std::string, int64_t> votes;
    for (auto p : vote_infos) {
        votes[p.first] = p.second;
        cout << p.first << " " << p.second << endl;
    }
    api_method_imp_.unVoteNode(g_userinfo, votes, out_str);
    tackle_send_tx_request(out_str);
}

void ApiMethod::query_votes(std::string & account, std::ostringstream & out_str) {
    if (account.empty()) {
        if (!set_default_prikey(out_str)) {
            out_str << "No default account found, you should specify an account address, or set a default account." << endl;
            return;
        }
    } else {
        g_userinfo.account = account;
    }
    api_method_imp_.listVoteUsed(g_userinfo, g_userinfo.account, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::query_reward(std::string & account, std::ostringstream & out_str) {
    if (account.empty()) {
        if (!set_default_prikey(out_str)) {
            out_str << "No default account found, you should specify an account address, or set a default account." << endl;
            return;
        }
    } else {
        g_userinfo.account = account;
    }
    api_method_imp_.queryVoterDividend(g_userinfo, g_userinfo.account, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::claim_reward(std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    api_method_imp_.claimVoterDividend(g_userinfo, out_str);
    tackle_send_tx_request(out_str);
}

ApiMethod::ApiMethod() {
    // toplevl
    regist_method("help", std::bind(&ApiMethod::Help, this, std::placeholders::_1, std::placeholders::_2), "Show a list of commands.", Command_type::toplevel);

    // subcommands
    regist_method("get", std::bind(&ApiMethod::Get, this, std::placeholders::_1, std::placeholders::_2), "Retrieve information from the blockchain.", Command_type::subcommands);
    regist_method("system", std::bind(&ApiMethod::system, this, std::placeholders::_1, std::placeholders::_2), "Interact with system contracts.", Command_type::subcommands);
    regist_method("sendtx", std::bind(&ApiMethod::sendtx, this, std::placeholders::_1, std::placeholders::_2), "Send transaction to the blockchain.", Command_type::subcommands);
    regist_method("wallet",
                  std::bind(&ApiMethod::wallet, this, std::placeholders::_1, std::placeholders::_2),
                  "Create and manage accounts and public-private key pairs.",
                  Command_type::subcommands);

    // get
    regist_method("chaininfo", std::bind(&ApiMethod::getChainInfo, this, std::placeholders::_1, std::placeholders::_2), "Get chain informaion.", Command_type::get);
    regist_method("block",
                  std::bind(&ApiMethod::getBlock, this, std::placeholders::_1, std::placeholders::_2),
                  "Get block information based on block type and block height.",
                  Command_type::get);
    regist_method("transaction", std::bind(&ApiMethod::getTransaction, this, std::placeholders::_1, std::placeholders::_2), "Get transaction details on chain.", Command_type::get);
    regist_method("account", std::bind(&ApiMethod::getAccount, this, std::placeholders::_1, std::placeholders::_2), "Get account information on chain.", Command_type::get);
    regist_method("CGP", std::bind(&ApiMethod::getCGP, this, std::placeholders::_1, std::placeholders::_2), "Get on-chain governance parameters.", Command_type::get);

    // regist_method("userinfo", std::bind(&ApiMethod::UserInfo,
    //     this, std::placeholders::_1), "Print local user info.", Command_type::get);

    // system
    regist_method("registerNode", std::bind(&ApiMethod::registerNode, this, std::placeholders::_1, std::placeholders::_2), "Register as node.", Command_type::system);
    regist_method("setDividendRatio",
                  std::bind(&ApiMethod::setDividendRatio, this, std::placeholders::_1, std::placeholders::_2),
                  "Set devidend ratio for voters who support you.",
                  Command_type::system);
    regist_method("unregisterNode", std::bind(&ApiMethod::unRegisterNode, this, std::placeholders::_1, std::placeholders::_2), "Node unregister.", Command_type::system);
    regist_method("queryNodeInfo", std::bind(&ApiMethod::queryNodeInfo, this, std::placeholders::_1, std::placeholders::_2), "Query node information.", Command_type::system);
    regist_method("redeemNodeDeposit",
                  std::bind(&ApiMethod::redeemNodeDeposit, this, std::placeholders::_1, std::placeholders::_2),
                  "Redeem node deposit when node regiter.",
                  Command_type::system);
    regist_method("updateNodeType", std::bind(&ApiMethod::updateNodeType, this, std::placeholders::_1, std::placeholders::_2), "Update node type.", Command_type::system);
    regist_method("updateNodeInfo",
                  std::bind(&ApiMethod::updateNodeInfo, this, std::placeholders::_1, std::placeholders::_2),
                  "Update node type, node name, node deposit and dividend ratio at the same time.",
                  Command_type::system);
    regist_method("stakeDeposit", std::bind(&ApiMethod::stakeNodeDeposit, this, std::placeholders::_1, std::placeholders::_2), "Stake deposit.", Command_type::system);
    regist_method("unstakeDeposit", std::bind(&ApiMethod::unstakeNodeDeposit, this, std::placeholders::_1, std::placeholders::_2), "Unstake deposit.", Command_type::system);
    regist_method("setNodeName", std::bind(&ApiMethod::setNodeName, this, std::placeholders::_1, std::placeholders::_2), "Set nodename.", Command_type::system);
    regist_method("updateNodeSignKey", std::bind(&ApiMethod::updateNodeSignKey, this, std::placeholders::_1, std::placeholders::_2), "Update node sign key.", Command_type::system);

    regist_method("voteNode", std::bind(&ApiMethod::voteNode, this, std::placeholders::_1, std::placeholders::_2), "Vote on nodes.", Command_type::system);
    regist_method("unvoteNode", std::bind(&ApiMethod::unVoteNode, this, std::placeholders::_1, std::placeholders::_2), "Unvote on nodes.", Command_type::system);
    regist_method("claimNodeReward", std::bind(&ApiMethod::claimNodeReward, this, std::placeholders::_1, std::placeholders::_2), "Claim the node rewards.", Command_type::system);
    regist_method(
        "claimVoterDividend", std::bind(&ApiMethod::claimVoterDividend, this, std::placeholders::_1, std::placeholders::_2), "Claim the voter dividend.", Command_type::system);
    regist_method(
        "queryNodeReward", std::bind(&ApiMethod::queryNodeReward, this, std::placeholders::_1, std::placeholders::_2), "Query specific node rewards.", Command_type::system);
    regist_method("listVoteUsed", std::bind(&ApiMethod::listVoteUsed, this, std::placeholders::_1, std::placeholders::_2), "Query vote-used distribution.", Command_type::system);
    regist_method("queryVoterDividend",
                  std::bind(&ApiMethod::queryVoterDividend, this, std::placeholders::_1, std::placeholders::_2),
                  "Query specific voter dividend.",
                  Command_type::system);

    regist_method("submitProposal",
                  std::bind(&ApiMethod::submitProposal, this, std::placeholders::_1, std::placeholders::_2),
                  "Submit the on-chain governance proposal.",
                  Command_type::system);
    regist_method("withdrawProposal", std::bind(&ApiMethod::withdrawProposal, this, std::placeholders::_1, std::placeholders::_2), "Withdraw proposal.", Command_type::system);
    regist_method("tccVote",
                  std::bind(&ApiMethod::tccVote, this, std::placeholders::_1, std::placeholders::_2),
                  "TCC(TOP Network Community Council) vote on proposal.",
                  Command_type::system);
    regist_method(
        "queryProposal", std::bind(&ApiMethod::queryProposal, this, std::placeholders::_1, std::placeholders::_2), "Query specific proposal details.", Command_type::system);

    // sendtransaction
    regist_method("transfer",
                  std::bind(&ApiMethod::transfer, this, std::placeholders::_1, std::placeholders::_2),
                  "Transfer TOP tokens from account to account.",
                  Command_type::sendtransaction);
    regist_method("deployContract",
                  std::bind(&ApiMethod::deployContract, this, std::placeholders::_1, std::placeholders::_2),
                  "Create a contract account and publish code on it.",
                  Command_type::sendtransaction);
    regist_method("runContract",
                  std::bind(&ApiMethod::runContract, this, std::placeholders::_1, std::placeholders::_2),
                  "Send a transaction to a contract to execute it.",
                  Command_type::sendtransaction);
    regist_method(
        "stakeVote", std::bind(&ApiMethod::stakeVote, this, std::placeholders::_1, std::placeholders::_2), "Lock TOP tokens to get votes.", Command_type::sendtransaction);
    regist_method("unstakeVote",
                  std::bind(&ApiMethod::unStakeVote, this, std::placeholders::_1, std::placeholders::_2),
                  "Unstake votes from your account and unlock TOP tokens.",
                  Command_type::sendtransaction);
    regist_method("stakeGas", std::bind(&ApiMethod::stakeGas, this, std::placeholders::_1, std::placeholders::_2), "Lock TOP tokens to get gas.", Command_type::sendtransaction);
    regist_method("unstakeGas",
                  std::bind(&ApiMethod::unStakeGas, this, std::placeholders::_1, std::placeholders::_2),
                  "Unstake gas from your account and unlock TOP tokens.",
                  Command_type::sendtransaction);
    // regist_method("buydisk", std::bind(&ApiMethod::PledgeDisk,
    //     this, std::placeholders::_1), "Lock top to get disk limit.", Command_type::sendtransaction);
    // regist_method("selldisk", std::bind(&ApiMethod::RedeemDisk,
    //     this, std::placeholders::_1), "Decrease disk limit to unlock top.", Command_type::sendtransaction);

    // wallet
    regist_method("createAccount", std::bind(&ApiMethod::CreateAccount, this, std::placeholders::_1), "Create accounts locally.", Command_type::wallet);
//    regist_method(
//        "createAccountKeystore", std::bind(&ApiMethod::CreateAccountKeystore, this, std::placeholders::_1), "Create account keystore file by private key.", Command_type::wallet);
    regist_method("createKey", std::bind(&ApiMethod::CreateKey, this, std::placeholders::_1), "Create the public-private key pair.", Command_type::wallet);
    regist_method("createKeypairKeystore",
                  std::bind(&ApiMethod::CreateKeypairKeystore, this, std::placeholders::_1),
                  "Create public-private key pair keystore file by private key.",
                  Command_type::wallet);
    regist_method(
        "attachCreateAccount", std::bind(&ApiMethod::attachCreateAccount, this, std::placeholders::_1, std::placeholders::_2), "Create accounts locally.", Command_type::wallet);
    regist_method(
        "attachCreateKey", std::bind(&ApiMethod::attachCreateKey, this, std::placeholders::_1, std::placeholders::_2), "Create the public-private key pair.", Command_type::wallet);
    regist_method("setDefault",
                  std::bind(&ApiMethod::setDefault, this, std::placeholders::_1),
                  "Import the account keystore file to default account for sending transactions.",
                  Command_type::wallet);
    regist_method("attachSetDefault",
                  std::bind(&ApiMethod::attachsetDefault, this, std::placeholders::_1, std::placeholders::_2),
                  "Import the account keystore file to default account for sending transactions.",
                  Command_type::wallet);
    regist_method("list", std::bind(&ApiMethod::ListKey, this, std::placeholders::_1, std::placeholders::_2), "List existing accounts and keys.", Command_type::wallet);
    regist_method("resetPassword", std::bind(&ApiMethod::ResetPassword, this, std::placeholders::_1), "Reset keystore file password.", Command_type::wallet);
    regist_method("attachResetPassword",
                  std::bind(&ApiMethod::attachResetPassword, this, std::placeholders::_1, std::placeholders::_2),
                  "Reset keystore file password.",
                  Command_type::wallet);

#ifdef DEBUG
    // origin just preserve for debug
    regist_method("passport", std::bind(&ApiMethod::RequestToken, this, std::placeholders::_1), "Get the chain access identity token.");
    regist_method("config", std::bind(&ApiMethod::Config, this, std::placeholders::_1), "Config rpc host.");
    regist_method("create", std::bind(&ApiMethod::CreateChainAccount, this, std::placeholders::_1), "Create an account on chain.");
    regist_method("key", std::bind(&ApiMethod::Key, this, std::placeholders::_1), "Config local account key.");
    regist_method("keystore", std::bind(&ApiMethod::KeyStore, this, std::placeholders::_1), "Key Manager.");
    regist_method("rand", std::bind(&ApiMethod::Random, this, std::placeholders::_1), "Random Number.");
    regist_method("auth", std::bind(&ApiMethod::Authorize, this, std::placeholders::_1), "Authorize.");
    regist_method("cat", std::bind(&ApiMethod::CreateAccountTest, this, std::placeholders::_1), "Create Account Test.");
    regist_method("user", std::bind(&ApiMethod::ChangeUser, this, std::placeholders::_1), "Change User.");

    regist_method("getlist", std::bind(&ApiMethod::GetListProperty, this, std::placeholders::_1), "Get list property.");
    regist_method("cs", std::bind(&ApiMethod::CreateSubAccount, this, std::placeholders::_1), "Create child account.");

    regist_method("gk", std::bind(&ApiMethod::GenerateKeys, this, std::placeholders::_1), "Generate Keys");
    regist_method("ck", std::bind(&ApiMethod::CheckKeys, this, std::placeholders::_1), "Check Keys");
    // regist_method("aa", std::bind(&ApiMethod::ActivateAccounts,
    //     this, std::placeholders::_1), "Activate Accounts");
    // regist_method("caa",  std::bind(&ApiMethod::CheckActivateAccounts,
    //     this, std::placeholders::_1), "Check Activate Accounts");
#endif
}

ApiMethod::~ApiMethod() {
}

int ApiMethod::Help(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() == 1) {
        // print out top help info
        out_str << "NAME:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << "topcl\n\n";
        out_str << "COPYRIGHT:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << "Copyright 2018-2020 The TOP Network Authors\n\n";
        out_str << "USAGE:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << "command [options] [arguments...] subcommand [subcommand options] [arguments...]\n\n";
        out_str << "VERSION:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << "1.1.0\n\n";

        out_str << "COMMANDS:" << '\n';
        for (auto const & it : methods_) {
            out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << cmd_name[it.first];
            out_str << std::left << it.second.first << '\n';
        }

        for (auto const & it : level_methods_) {
            out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << cmd_name[it.first];
            out_str << std::left << it.second.first << '\n';
        }

        out_str << "\n";
        out_str << "OPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show a list of commands.\n\n";

    } else {
        out_str << "Error: Unknown Command.\n";
    }

    return 0;
}

int ApiMethod::Get(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "COMMANDS:" << '\n';
        for (auto & it : get_methods_) {
            out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << cmd_name[it.first];
            out_str << std::left << it.second.first << '\n';
        }

        out_str << "\n";
        out_str << "OPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show a list of commands or help for one command.\n\n";
        return -1;
    }

    return 0;
}

int ApiMethod::system(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "COMMANDS:" << '\n';
        for (auto & it : system_methods_) {
            out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << cmd_name[it.first];
            out_str << std::left << it.second.first << '\n';
        }

        out_str << "\n";
        out_str << "OPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show a list of commands or help for one command.\n\n";

        return -1;
    }

    return 0;
}

int ApiMethod::sendtx(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "COMMANDS:" << '\n';
        for (auto & it : sendtransaction_methods_) {
            out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << cmd_name[it.first];
            out_str << std::left << it.second.first << '\n';
        }

        out_str << "\n";
        out_str << "OPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show a list of commands or help for one command.\n\n";

        return -1;
    }

    return 0;
}

int ApiMethod::wallet(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "COMMANDS:" << '\n';
        for (auto & it : wallet_methods_) {
            if (it.first.find("attach") != std::string::npos)
                continue;
            out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << cmd_name[it.first];
            out_str << std::left << it.second.first << '\n';
        }

        out_str << "\n";
        out_str << "OPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show a list of commands or help for one command.\n\n";

        return -1;
    }

    return 0;
}

int ApiMethod::KeyStore(const ParamList & param_list) {
    // "@vote_key", "T-123456789012345678901234567890123"
    if (param_list.size() != 3) {
        std::cout << "keystore params error." << std::endl;
        return -1;
    }

    std::string type = param_list[1];
    std::string value = param_list[2];
    std::cout << "type:" << type.c_str() << " value:" << value.c_str() << std::endl;
    bool br = api_method_imp_.key_store(g_userinfo, type, value);
    return br ? 0 : -1;
}

int ApiMethod::Random(const ParamList & param_list) {
    return 0;
}

int ApiMethod::Authorize(const ParamList & param_list) {
    return 0;
}

int ApiMethod::CreateChainAccount(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Create an account on chain.\n";
        std::cout << "\nUSAGE:\n    sendtx create\n";
        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n\n";
        std::cout << "\nEXAMPLE:\n    sendtx create\n";
        return -1;
    }

    return api_method_imp_.create_account(g_userinfo) ? 0 : -1;
}

int ApiMethod::CreateAccountTest(const ParamList & param_list) {
    if (param_list.size() != 2) {
        std::cout << "CreateAccountTest params error." << std::endl;
        return -1;
    }

    std::string count_str = param_list[1];
    uint64_t count = atoi(count_str.c_str());
    for (uint64_t i = 0; i < count; ++i) {
        api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
        api_method_imp_.create_account(g_userinfo);
    }
    return 0;
}

int ApiMethod::getAccount(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Get account info on chain.\n";
        out_str << "\nUSAGE:\n    get account [account_addr]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "account_addr  STRING  The account address to be queried,the default is current account in use.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    get account\n";
        out_str << "    get account T00000LQB6umTo6TZs9UjmZEgwpd7Jt3R7hGhYCS\n";
        return -1;
    }
    if (param_list.size() < 1) {
        out_str << "AccountInfo params error." << std::endl;
        return -1;
    }
    string account = g_userinfo.account;
    if (param_list.size() >= 2) {
        account = param_list[1];
    }
    return api_method_imp_.getAccount(g_userinfo, account, out_str) ? 0 : -1;
}

int ApiMethod::transfer(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Transfer tokens from account to account.\n";
        out_str << "\nUSAGE:\n    sendtx transfer account_addr amount [tx_deposit] [note]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "account_addr  STRING  REQUIRED  The target account address, a regular account or a contract account.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "amount  STRING  REQUIRED  Transfer amount(uTOP).\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-n --note"
                << "Transaction note,characters of any type, not exceeding 128 in length.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-t --tx_deposit"
                << "Transaction deposit,a minimum of 10,0000 uTOP.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    sendtx transfer T00000LgPNkFe6X9DbX1aXzX9SuJhRuhM9bCs5sM 8000 -n haha -t 100000\n";
        return -1;
    }

    if (param_list.size() < 3)
        return -1;

    if (param_list.size() > 3 && param_list.size() != 5 && param_list.size() != 7) {
        out_str << "Wrong param count" << endl;
        return -1;
    }

    std::string from = g_userinfo.account;
    std::string to = param_list[1];
    if (to.empty())
        return -1;

    uint64_t amount = stoull(param_list[2]);

    std::string memo;
    uint64_t tx_deposit = 0;
    if (param_list.size() > 3) {
        string o1 = param_list[3];
        if (o1 == "-n" || o1 == "--note") {
            memo = param_list[4];
        } else if (o1 == "-t" || o1 == "--tx_deposit") {
            tx_deposit = std::stoull(param_list[4]);
        }
        if (param_list.size() > 5) {
            string o2 = param_list[5];
            if (o2 == "-n" || o2 == "--note") {
                memo = param_list[6];
            } else if (o2 == "-t" || o2 == "--tx_deposit") {
                tx_deposit = std::stoull(param_list[6]);
            }
        }
    }

    if (memo.size() > 128) {
        out_str << "memo size: " << memo.size() << " > maximum size 128" << endl;
        return 0;
    }

    if (tx_deposit != 0) {
        api_method_imp_.set_tx_deposit(tx_deposit);
    }
    bool br = api_method_imp_.transfer(g_userinfo, from, to, amount, memo, out_str);
    return br ? 0 : -1;
}

int ApiMethod::stakeGas(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Lock TOP tokens to get gas.\n";
        out_str << "\nUSAGE:\n    sendtx stakeGas account_addr utop_num\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "account_addr  STRING  REQUIRED  The account address about to exchange gas.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "locked_utop  STRING  REQUIRED  Locked TOP token amount(uTOP).\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "sendtx stakeGas T00000LQB6umTo6TZs9UjmZEgwpd7Jt3R7hGhYCS 1000\n";
        return -1;
    }

    if (param_list.size() < 3)
        return -1;

    std::string from = g_userinfo.account;
    std::string to = param_list[1];
    if (to.empty())
        return -1;

    auto amount = stoi(param_list[2]);
    bool br = api_method_imp_.stakeGas(g_userinfo, from, to, amount, out_str);
    return br ? 0 : -1;
}

int ApiMethod::unStakeGas(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Unstake gas from your account and unlock TOP tokens.\n";
        out_str << "\nUSAGE:\n    sendtx unstakeGas account_addr utop_num\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "account_addr  STRING  REQUIRED  The account address which needs to unlock TOP token.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "unlocked_utop  STRING  REQUIRED  Unlocked TOP token amount(uTOP).\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "sendtx unstakeGas T00000LQB6umTo6TZs9UjmZEgwpd7Jt3R7hGhYCS 1000 \n";
        return -1;
    }

    if (param_list.size() < 2)
        return -1;

    std::string from = g_userinfo.account;
    std::string to = param_list[1];
    if (to.empty())
        return -1;

    auto amount = stoi(param_list[2]);
    bool br = api_method_imp_.unStakeGas(g_userinfo, from, to, amount, out_str);
    return br ? 0 : -1;
}

int ApiMethod::getTransaction(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Get transaction details on chain.\n";
        out_str << "\nUSAGE:\n    get transaction [account_addr] [tx_hash]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "account_addr  STRING  REQUIRED  The account address whcih sends transactions or receives transactions.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "tx_hash  STRING  REQUIRED  The last transaction hash of the account which can be queried by 'get account'.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "get transaction T00000LgPNkFe6X9DbX1aXzX9SuJhRuhM9bCs5sM 0x178bc3395de916a422d3f144f3fa27e21ccf7a3eb34d18ba8fa52ecaf6e59f8b\n";
        return -1;
    }

    std::string account{""};
    std::string tx_hash{""};
    if (1 == param_list.size()) {
        account = g_userinfo.account;
        tx_hash = g_userinfo.last_hash;
    } else if (3 == param_list.size()) {
        account = param_list[1];
        tx_hash = param_list[2];
    } else {
        out_str << "Cmd transaction lost params. param_count=0 or param_count=2\n";
        return -1;
    }

    bool br = api_method_imp_.getTransaction(g_userinfo, account, tx_hash, out_str);
    return br ? 0 : -1;
}

int ApiMethod::RequestToken(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Get the chain access identity token.\n";
        std::cout << "\nUSAGE:\n    passport\n";
        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "passport \n";
        return -1;
    }

    if (param_list.size() < 2) {
        api_method_imp_.passport(g_userinfo);
        task_dispatcher::get_instance()->stat_.test_count_ = 1;
    } else if (param_list.size() == 3) {
        int count_per_second = atoi(param_list[1].c_str());
        int second = atoi(param_list[2].c_str());
        task_dispatcher::get_instance()->stat_.test_count_ = count_per_second * second;
        if (count_per_second != 0 && second != 0) {
            uint32_t count = count_per_second * second;
            uint32_t interval = 1000 * 1000 / count_per_second;
            auto tp1 = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < count; ++i) {
                auto tp2 = std::chrono::steady_clock::now();
                int64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count();
                if (interval * i > duration) {
                    utility::usleep(interval * i - duration);
                }

                api_method_imp_.passport(g_userinfo);
            }
        }
    }
    return 0;
}

int ApiMethod::UserInfo(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "print local user info.\n";
        std::cout << "Usage: get userinfo\n\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "get userinfo\n";
        return -1;
    }

    dump_userinfo(g_userinfo);
    return 0;
}

int ApiMethod::ChangeUser(const ParamList & param_list) {
    if (param_list.size() < 2)
        return -1;

    std::string user = param_list[1];
    if (user.empty())
        return -1;

    g_userinfo.clear();
    g_userinfo.account = user;
    std::cout << "Change Account: " << user.c_str() << std::endl;
    return 0;
}

int ApiMethod::Config(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "config rpc client & rpc method.\n";
        std::cout << "Usage: config [OPTIONS] client|method\n\n";
        std::cout << "OPTIONS:\n";
        std::cout << "  -m" << std::setw(HELP_WIDTH) << std::setfill(' ') << " "
                  << "method: 'http'(default) or 'ws'\n";
        std::cout << "  -c" << std::setw(HELP_WIDTH) << std::setfill(' ') << " "
                  << "client format: 'ip:port'\n";
        std::cout << "\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "config -h 127.0.0.1:19081\n";
        std::cout << "    config -m ws\n";
        return -1;
    }

    if (param_list.size() < 3) {
        LOG("Cmd config lost params. param_count=3");
        return -1;
    }

    std::string cmd = param_list[1];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    std::string param = param_list[2];
    std::transform(param.begin(), param.end(), param.begin(), ::tolower);
    if (cmd == "-m") {
        if (param == "http") {
            change_trans_mode(true);
        } else if (param == "ws") {
            change_trans_mode(false);
        } else {
            std::cout << "Change trans mode param error." << std::endl;
        }
    } else if (cmd == "-c") {
        if (!api_method_imp_.ChangeHost(param)) {
            std::cout << "Host param error." << std::endl;
        } else {
            std::cout << "Set host: " << g_server_host_port.c_str() << std::endl;
        }
    } else {
        std::cout << "Param error." << std::endl;
    }

    return 0;
}

std::string ApiMethod::input_hiding() {
    std::system("stty -echo");
    std::string str;
    std::getline(std::cin, str, '\n');
    cin.clear();
    std::system("stty echo");
    return str;
}

std::string ApiMethod::input_no_hiding() {
    std::string str;
    std::getline(std::cin, str, '\n');
    cin.clear();
    return str;
}

bool ApiMethod::check_password() {
    if (is_reset_pw) {
        cout << "Please set a new password. The password must consist of Numbers and Letters, 8 to 16 characters. Pressing Ctrl+C can exit the command." << endl;
    } else if (is_account) {
        std::cout << "Please set a password for the account keystore file. The password must consist of Numbers and Letters, 8 to 16 characters." << std::endl;
    } else {
        std::cout << "Please set a password for the keystore file. The password must consist of Numbers and Letters, 8 to 16 characters." << std::endl;
    }
    g_pw_hint = "";  // set to default empty
    auto pw1 = input_hiding();
    if (pw1.size() < 8 || pw1.size() > 16) {
        std::cout << "Password error!" << std::endl;
        return check_password();
    }
    bool digit_included{false};
    bool letter_included{false};
    for (auto c : pw1) {
        if (c >= '0' && c <= '9')
            digit_included = true;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            letter_included = true;
    }
    if (!(digit_included && letter_included)) {
        std::cout << "Password error!" << std::endl;
        return check_password();
    }

    std::cout << "Please Input Password Again" << std::endl;
    auto pw2 = input_hiding();

    if (pw1 != pw2) {
        std::cout << "Passwords are not the same." << std::endl;
        return check_password();
    } else {
        cache_pw = pw1;
        std::cout << "Please set a password hint! If don't, there will be no hint when you forget your password." << std::endl;
        g_pw_hint = input_no_hiding();
        return true;
    }
}
int ApiMethod::input_pri_key(std::string& pri_str) {
    cout<<"Please input private key."<<endl;
    std::string pri_key = input_hiding();
    if (check_pri_key(pri_key) == false) {
        std::cout << "Private key length error!" << std::endl;
        return 1;
    }
    pri_str = pri_key;
    return 0;
}

int ApiMethod::CreateAccount(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Create account locally.\n";
        std::cout << "\nUSAGE:\n    createAccount [options]\n";
        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-d --dir"
                  << "[the keystore file storage path].\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "wallet createAccount -d /home/ttt\n\n";
        return -1;
    }

    is_account = true;
    if (!check_password()) {
        is_account = false;
        return -1;
    }

    std::string dir = "";
    if (param_list.size() == 3) {
        if (param_list[1] == "-d" || param_list[1] == "--dir") {
            dir = param_list[2];
        }
    }

    auto path = create_new_keystore(cache_pw, dir);
    std::cout << "Successfully create an account locally!\n" << std::endl;
    std::cout << "You can share your public key and account address with anyone.Others need them to interact with you!" << std::endl;
    std::cout << "You must nerver share the private key or account keystore file with anyone!They control access to your funds!" << std::endl;
    std::cout << "You must backup your account keystore file!Without the file, you will not be able to access the funds in your account!" << std::endl;
    std::cout << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    std::cout << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    std::cout << "Account Address: " << g_userinfo.account << std::endl;
    std::cout << "Account Keystore File Path: " << path << std::endl;
    if (copy_g_userinfo.account.size() != 0) {
        g_userinfo = copy_g_userinfo;
        copy_g_userinfo.clear();
    }
    is_account = false;
    return 0;
}
/*
int ApiMethod::CreateAccountKeystore(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Create account keystore file by private key.\n";
        std::cout << "\nUSAGE:\n    createAccountKeystore private_key\n";
        std::cout << "Params:\n     private_key BASE64 STRING REQUIRED The private key of the account.\n";

        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-d --dir"
                  << "[the keystore file storage path].\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "wallet createAccountKeystore ViozcHV2UwMhzmwZRt1LOs05bxTfa+VevqynkjOAxAQ= -d /home/tttt\n";
        return -1;
    }

    std::string str_pri;
    if (param_list.size() > 1) {
        str_pri = param_list[1];
        if (!check_pri_key(str_pri)) {
            cout << "private key error" << endl;
            return -1;
        }
    } else {
        return -1;
    }

    is_account = true;
    if (!check_password()) {
        is_account = false;
        return -1;
    }

    std::string dir = "";
    if (param_list.size() == 4) {
        if (param_list[2] == "-d" || param_list[2] == "--dir") {
            dir = param_list[3];
        }
    }

    auto path = create_new_keystore(cache_pw, dir, str_pri);
    std::cout << "Successfully create an account keystore file!\n" << std::endl;
    std::cout << "You can share your public key and account address with anyone.Others need them to interact with you!" << std::endl;
    std::cout << "You must nerver share the private key or account keystore file with anyone!They control access to your funds!" << std::endl;
    std::cout << "You must backup your account keystore file!Without the file, you will not be able to access the funds in your account!" << std::endl;
    std::cout << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    if (str_pri.size() == BASE64_PRI_KEY_LEN)
        std::cout << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    else
        std::cout << "Public Key: " << top::utl::xcrypto_util::get_hex_public_key(g_userinfo.private_key) << std::endl;
    std::cout << "Account Address: " << g_userinfo.account << std::endl;
    std::cout << "Account Keystore File Path: " << path << std::endl;
    if (copy_g_userinfo.account.size() != 0) {
        g_userinfo = copy_g_userinfo;
        copy_g_userinfo.clear();
    }
    is_account = false;
    return 0;
}*/

int ApiMethod::CreateKey(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Create the public-private key pair.\n";
        std::cout << "\nUSAGE:\n    createKey [options]\n";
        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-d --dir"
                  << "[the keystore file storage path].\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "wallet createKey -d /home/ttt\n\n";
        return -1;
    }

    if (!check_password()) {
        return -1;
    }

    std::string dir = "";
    if (param_list.size() == 3) {
        if (param_list[1] == "-d" || param_list[1] == "--dir") {
            dir = param_list[2];
        }
    }

    auto path = create_new_keystore(cache_pw, dir, true);
    std::cout << "Successfully create an key locally!\n" << std::endl;
    std::cout << "You can share your public key with anyone.Others need it to interact with you!" << std::endl;
    std::cout << "You must nerver share the private key or keystore file with anyone!They can use them to make the node malicious." << std::endl;
    std::cout << "You must backup your keystore file!Without the file,you may not be able to send transactions." << std::endl;
    std::cout << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    std::cout << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    std::cout << "Keystore File Path: " << path << std::endl;
    g_userinfo = copy_g_userinfo;
    copy_g_userinfo.clear();
    return 0;
}

int ApiMethod::CreateKeypairKeystore(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Create public-private key pair keystore file by private key.\n";
        std::cout << "\nUSAGE:\n     createKeypairKeystore private_key [datadir]\n";
        std::cout << "Params:\n     private_key BASE64 STRING REQUIRED The private key of the key pair.\n";

        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-d --dir"
                  << "Storage directory of public-private key pair keystore file.\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "wallet createKeypairKeystore ViozcHV2UwMhzmwZRt1LOs05bxTfa+VevqynkjOAxAQ= -d /home/tttt\n";
        return -1;
    }

    std::string str_pri;
    if (param_list.size() > 1) {
        str_pri = param_list[1];
        if (!check_pri_key(str_pri)) {
            cout << "private key error" << endl;
            return -1;
        }
    } else {
        return -1;
    }

    if (!check_password()) {
        return -1;
    }

    std::string dir = "";
    if (param_list.size() == 4) {
        if (param_list[2] == "-d" || param_list[2] == "--dir") {
            dir = param_list[3];
        }
    }

    auto path = create_new_keystore(cache_pw, dir, str_pri, true);
    std::cout << "Successfully create a publice-private key pair keystore file!\n" << std::endl;
    std::cout << "You can share your public key and account address with anyone.Others need them to interact with you!" << std::endl;
    std::cout << "You must nerver share the private key or account keystore file with anyone!They control access to your funds!" << std::endl;
    std::cout << "You must backup your account keystore file!Without the file, you will not be able to access the funds in your account!" << std::endl;
    std::cout << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    std::cout << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    std::cout << "Keystore File Path: " << path << std::endl;
    g_userinfo = copy_g_userinfo;
    copy_g_userinfo.clear();
    return 0;
}

// just for attach mode
int ApiMethod::attachCreateKey(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Create the public-private key pair.\n";
        out_str << "\nUSAGE:\n    createKey [options]\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ');
        out_str << "-d --dir"
                << "[the keystore file storage path].\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "wallet createKey -d /home/ttt\n\n";
        return -1;
    }

    if (param_list[1] == "empty") {
        cache_pw = empty_pw;
    } else {
        cache_pw = param_list[1];
    }
    std::string dir = "";
    if (param_list.size() == 5) {
        if (param_list[3] == "-d" || param_list[3] == "--dir") {
            dir = param_list[4];
        }
    }

    if (param_list[2] == "empty") {
        g_pw_hint = "";
    } else {
        g_pw_hint = param_list[2];
    }

    auto path = create_new_keystore(cache_pw, dir, true);
    out_str << "Successfully create an key locally!\n" << std::endl;
    out_str << "You can share your public key with anyone.Others need it to interact with you!" << std::endl;
    out_str << "You must nerver share the private key or keystore file with anyone!They can use them to make the node malicious." << std::endl;
    out_str << "You must backup your keystore file!Without the file,you may not be able to send transactions." << std::endl;
    out_str << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    out_str << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    out_str << "Keystore File Path: " << path << std::endl;
    g_userinfo = copy_g_userinfo;
    copy_g_userinfo.clear();
    return 0;
}

// just for attach mode
int ApiMethod::attachCreateAccount(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Create account locally.\n";
        out_str << "\nUSAGE:\n    createAccount [options]\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ');
        out_str << "-d --dir"
                << "[the keystore file storage path].\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "wallet createAccount -d /home/ttt\n\n";
        return -1;
    }

    if (param_list[1] == "empty") {
        cache_pw = empty_pw;
    } else {
        cache_pw = param_list[1];
    }

    std::string dir = "";
    if (param_list.size() == 5) {
        if (param_list[3] == "-d" || param_list[3] == "--dir") {
            dir = param_list[4];
        }
    }

    if (param_list[2] == "empty") {
        g_pw_hint = "";
    } else {
        g_pw_hint = param_list[2];
    }

    auto path = create_new_keystore(cache_pw, dir);
    out_str << "Successfully create an account locally!\n" << std::endl;
    out_str << "You can share your public key and account address with anyone.Others need them to interact with you!" << std::endl;
    out_str << "You must nerver share the private key or keystore file with anyone!They control access to your funds!" << std::endl;
    out_str << "You must backup your keystore file!Without the file, you will not be able to access the funds in your account!" << std::endl;
    out_str << "You must remember your password!Without the password,it’s impossible to use the keystore file!" << std::endl;
    out_str << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << std::endl;
    out_str << "Account Address: " << g_userinfo.account << std::endl;
    out_str << "Account Keystore File Path: " << path << std::endl;
    if (copy_g_userinfo.account.size() != 0) {
        g_userinfo = copy_g_userinfo;
        copy_g_userinfo.clear();
    }
    return 0;
}

int ApiMethod::setDefault(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Import the account keystore file to default account for sending transactions.\n";
        std::cout << "\nUSAGE:\n    wallet setDefault keystore_file_path\n";
        std::cout << "\nParams:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                  << "keystore_file_path  STRING  REQUIRED  The account keystore file path of this account address.\n";
        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "wallet setDefault /home/topcl/keystore/T00000LSyMFKgHfzGBC8sbSEHPXTswzKMfH8soZR\n\n";
        return -1;
    }

    if (param_list.size() < 2) {
        std::cout << "Please input a private key file path." << std::endl;
        return -1;
    }

    std::cout << "Please Input Password. If no password is set when the account is created, similarly, press Ctrl+D to skip the password setting." << std::endl;
    auto pw = input_hiding();
    if (0 == pw.size()) {
        pw = empty_pw;
    }

    auto key_info = parse_keystore(param_list[1]);
    if (key_info.empty()) {
        std::cout << "File error! Can't find the keystore file! " << std::endl;
        return -1;
    }
    if (key_info["address"].isNull()) {
        std::cout << "File error! You are using the keystore file, please use the account keystore file to set the default account. " << std::endl;
        return -1;
    }

    auto base64_pri = import_existing_keystore(pw, param_list[1]);
    if (set_g_userinfo(base64_pri)) {
        std::cout << g_userinfo.account << ": set default account successfully." << std::endl;
    } else {
        std::cout << "Set default account failed." << std::endl;
    }
    return 0;
}

int ApiMethod::attachsetDefault(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Import the keystore file to default account for sending transactions.\n";
        out_str << "\nUSAGE:\n    wallet setDefault keystore_file_path\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "keystore_file_path  STRING  REQUIRED  The account keystore file path of this account address.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "wallet setDefault /home/topcl/keystore/T00000LSyMFKgHfzGBC8sbSEHPXTswzKMfH8soZR\n\n";
        return -1;
    }

    if (param_list[1] == "empty") {
        cache_pw = empty_pw;
    } else {
        cache_pw = param_list[1];
    }

    auto key_info = attach_parse_keystore(param_list[2], out_str);
    if (key_info["address"].isNull()) {
        out_str << "File error! You are using the keystore file, please use the account keystore file to set the default account. " << std::endl;
        return -1;
    }

    auto base64_pri = attach_import_existing_keystore(cache_pw, param_list[2], out_str);
    if (set_g_userinfo(base64_pri)) {
        out_str << g_userinfo.account << ": set default account successfully." << std::endl;
    } else {
        out_str << "Set default account failed." << std::endl;
    }
    return 0;
}

void ApiMethod::outAccountBalance(const std::string & account, std::ostringstream & out_str) {
    std::ostringstream as;
    auto tmp = g_userinfo.account;
    g_userinfo.account = account;
    std::string q_account(account);
    if (top::base::xvaccount_t::get_addrtype_from_account(q_account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        std::transform(q_account.begin()+1, q_account.end(), q_account.begin()+1, ::tolower);
    api_method_imp_.getAccount(g_userinfo, q_account, as);
    g_userinfo.account = tmp;
    xJson::Reader reader;
    xJson::Value root;
    if (reader.parse(as.str(), root)) {
        if (root["data"]["balance"].isUInt64()) {
            auto balance = root["data"]["balance"].asUInt64();
            double top_balance = (double)balance / TOP_UNIT;
            out_str.setf(std::ios::fixed, std::ios::floatfield);
            out_str.precision(6);
            out_str << "balance: " << top_balance << " TOP" << std::endl;
            out_str << "nonce: " << root["data"]["nonce"].asUInt64() << std::endl;
        } else {
            out_str << "balance: -" << std::endl;
            out_str << "nonce: -" << std::endl;
        }
    }
}

int ApiMethod::ListKey(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "List existing accounts and keys.\n";
        out_str << "\nUSAGE:\n    wallet list [options]\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-d --dir"
                << "[target key dir to list].\n";
        out_str << std::left << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-u --using"
                << "only list key in use.\n\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "wallet list -u\n";
        out_str << "    wallet list -d /home/topcl/keystore -u\n";
        return -1;
    }
    std::string dir = "";
    if (param_list.size() >= 3 && (param_list[1] == "-d" || param_list[1] == "--dir")) {
        dir = param_list[2];
    } else {
        dir = g_keystore_dir;
    }

    bool using_option{false};
    if (param_list.size() == 2 && (param_list[1] == "-u" || param_list[1] == "--using")) {
        using_option = true;
    }
    if (param_list.size() == 4) {
        if (param_list[1] == "-u" || param_list[1] == "--using" || param_list[3] == "-u" || param_list[3] == "--using") {
            using_option = true;
        }
    }

    std::vector<std::string> keys = scan_key_dir(dir);
    auto key_account_cmp = [](const std::string & s1, const std::string & s2) {
        if (s1.substr(0, 2) == "T-" && s2.substr(0, 2) != "T-") {
            return true;
        } else {
            return false;
        }
    };
    std::cout << std::endl;
    std::reverse(keys.begin(), keys.end());
    stable_sort(keys.begin(), keys.end(), key_account_cmp);
    for (size_t i = 0; i < keys.size(); ++i) {
        auto path = dir + "/" + keys[i];
        auto key_info = attach_parse_keystore(path, out_str);
        auto account = key_info["address"].asString();
        string status{""};
        if (account != "" && account == g_userinfo.account) {
            status = "Used as default";
        }

        auto public_key = key_info["public_key"].asString();
        if (account == g_userinfo.account || !using_option) {
            if (account != "") {
                out_str << "account #" << i << ": " << account << std::endl;
                outAccountBalance(account, out_str);
                out_str << "account ";
            }
            out_str << "keystore file path: " << path;
            out_str << "\npublic key: " << public_key;
            if (status.size() != 0) {
                out_str << "\nstatus: " << status << std::endl << std::endl;
            } else {
                out_str << std::endl << std::endl;
            }
        }
    }
    if (keys.size() == 0) {
        out_str << "No Key Pair" << std::endl;
    }
    return 0;
}

int ApiMethod::ResetPassword(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "Reset keystore file password.\n";
        std::cout << "\nUSAGE:\n    wallet resetpassword keystore_file_path [options]\n";
        std::cout << "\nParams:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                  << "keystore file path  STRING  REQUIRED  Reset the keystore file password for the account or the key.\n";
        std::cout << "\nOPTIONS:\n";
        std::cout << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                  << "Show help information for one command.\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "wallet resetpassword /root/Topnetwork/keystore/T00000LX2X7fPxZFPuC4iXt7CTXMJzv13KG918Ga\n\n";
        return -1;
    }

    if (param_list.size() < 2) {
        std::cout << "Please Input Keystore Path!!!" << std::endl;
        return -1;
    }

    std::cout << "Please Input Old Password." << std::endl;
    auto pw = input_hiding();
    if (0 == pw.size()) {
        pw = " ";
    }
    auto path = param_list[1];
    reset_keystore_pw(pw, path);
    return 0;
}

int ApiMethod::attachResetPassword(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Reset keystore file password.\n";
        out_str << "\nUSAGE:\n    wallet resetpassword keystore_file_path [options]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "keystore file path  STRING  REQUIRED  Reset the keystore file password for the account or the key.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "wallet resetpassword /root/Topnetwork/keystore/T00000LX2X7fPxZFPuC4iXt7CTXMJzv13KG918Ga\n\n";
        return -1;
    }

    std::string old_pw;
    if (param_list[1] == "empty") {
        old_pw = empty_pw;
    } else {
        old_pw = param_list[1];
    }

    if (param_list[2] == "empty") {
        cache_pw = empty_pw;
    } else {
        cache_pw = param_list[2];
    }

    if (param_list[3] == "empty") {
        g_pw_hint = "";
    } else {
        g_pw_hint = param_list[3];
    }

    auto path = param_list[4];
    attach_reset_keystore_pw(old_pw, cache_pw, path, out_str);
    return 0;
}

int ApiMethod::Key(const ParamList & param_list) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        std::cout << "config local account key.\n";
        std::cout << "\nUSAGE:\n    key [OPTIONS] [value]\n";
        std::cout << "OPTIONS:\n";
        std::cout << std::left << std::setw(HELP_WIDTH) << std::setfill(' ') << "create_key "
                  << " "
                  << "value: empty\n";
        std::cout << std::left << std::setw(HELP_WIDTH) << std::setfill(' ') << "import "
                  << " "
                  << "value: private key file dir\n";
        std::cout << std::left << std::setw(HELP_WIDTH) << std::setfill(' ') << "list "
                  << " "
                  << "value: empty\n";
        std::cout << std::left << std::setw(HELP_WIDTH) << std::setfill(' ') << "resetpassword "
                  << " "
                  << "value: empty\n";
        std::cout << "\nEXAMPLE:\n    ";
        std::cout << "key create_key\n";
        return -1;
    }

    if (param_list.size() < 2) {
        LOG("Cmd key lost params. param_count=2");
        return -1;
    }

    std::string cmd = param_list[1];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    if (cmd == "-s") {
        api_method_imp_.make_child_private_key(g_userinfo.account, g_userinfo.child.private_key, g_userinfo.child.account);
    } else if (cmd == "-t") {
        api_method_imp_.make_private_key(g_userinfo.contract.private_key);
        g_userinfo.contract.account = api_method_imp_.make_account_address(g_userinfo.contract.private_key, top::base::enum_vaccount_addr_type_custom_contract, g_userinfo.account);
    } else if (cmd == "-i") {
        if (param_list.size() < 3) {
            LOG("Cmd key lost params: base64 private key. param_count=3");
            return -1;
        }
        auto base64_pri = param_list[2];
        if (set_g_userinfo(base64_pri)) {
            std::cout << g_userinfo.account << ": import private key successfully." << std::endl;
        } else {
            std::cout << "Import private key failed." << std::endl;
        }
        return 0;
    } else {
        std::cout << "Param error." << std::endl;
    }

    return 0;
}

int ApiMethod::GetListProperty(const ParamList & param_list) {
    auto size = param_list.size();
    if (size != 2 && size != 3) {
        LOG("Cmd GetProperty (gp) lost params. param_count=2 or 3");
        return -1;
    }
    std::string account{""};
    std::string type{"list"};
    std::string data1{""};
    std::string data2{""};
    if (size == 2) {
        account = g_userinfo.account;
        data1 = param_list[1];  //"param1"
    } else {
        account = param_list[1];
        data1 = param_list[2];  //"param1"
    }

    bool br = api_method_imp_.getProperty(g_userinfo, account, type, data1, data2);
    return br ? 0 : -1;
}

int ApiMethod::CreateSubAccount(const ParamList & param_list) {
    //    child:
    //    account: T-nQtHoi5GZ2iKu9e31H29hhqjSHb3duQ4d
    //    private_key : 0x1cde682dd8c55faf18dac699b37c7c796f81c3813cd1519a62286aaf09896ab5
    std::string child_account{g_userinfo.child.account};

    bool br = api_method_imp_.create_sub_account(g_userinfo, child_account, g_userinfo.child.private_key);
    return br ? 0 : -1;
}

int ApiMethod::deployContract(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Create a contract account and publish code on it.\n";
        out_str << "\nUSAGE:\n    sendtx deployContract gas_limit amount contract_path [tx_deposit]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "gas_limit  STRING  REQUIRED  Upper limit of gas fees that the contract is willing to pay for the sender of the transaction.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "amount  STRING  REQUIRED  The amount transferred to the contract account.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "contract_path  STRING  REQUIRED  Contract code file path.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-t --tx_deposit"
                << "Transaction deposit,the default is 10,0000uTOP.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "sendtx deployContract 0 1000 /home/git/TopPyFrame/apitest/lua_script/create_key_rename.lua -t 100000\n";
        return -1;
    }

    if (param_list.size() < 3)
        return -1;

    uint64_t tgas_limit = stoull(param_list[1]);
    uint64_t amount = stoull(param_list[2]);
    std::string code_path = param_list[3];
    ifstream code_file(code_path);
    ostringstream tmp;
    tmp << code_file.rdbuf();
    string code = tmp.str();
    if (param_list.size() == 6) {
        if (param_list[4] == "-t" || param_list[4] == "--tx_deposit") {
            api_method_imp_.set_tx_deposit(std::stoull(param_list[5]));
        }
    }
    bool br = api_method_imp_.deployContract(g_userinfo, g_userinfo.contract.account, tgas_limit, amount, code, out_str);
    out_str << "contract account: " << g_userinfo.contract.account << std::endl;
    return br ? 0 : -1;
}

int ApiMethod::runContract(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Send a transaction to a contract to execute it.\n";
        out_str << "\nUSAGE:\n    sendtx runContract amount contract_addr contract_func param_type,param_value|param_type2,param_value2... [tx_deposit]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "amount UINT64 The amount transferred to the application contract account.The unit is uTOP.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "contract_addr  STRING  REQUIRED  Contract account address, beginning with the symbol \"T-3\".\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "contract_func  STRING  REQUIRED  The name of the contract function.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "param_type  OBJECT  REQUIRED  Parameter type:1--Uint64;2--String;3--Bool.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "param_value OBJECT  REQUIRED  Parameter value.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-t --tx_deposit"
                << "Transaction deposit,the default is 10,0000uTOP.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "sendtx runContract 100 T-3-MbEfwERMNw9c4oKbLtfkWBkL2KFRsahEDe test_func 1,3243|2,abcd|3,true -t 100000\n";
        return -1;
    }

    if (param_list.size() < 3)
        return -1;

    uint64_t amount = stoull(param_list[1]);
    std::string contract = param_list[2];
    std::string func = param_list[3];
    std::string para = "";
    if (param_list.size() > 4) {
        if (param_list[4] == "-t" || param_list[4] == "--tx_deposit") {
            api_method_imp_.set_tx_deposit(std::stoull(param_list[5]));
        } else {
            para = param_list[4];
            if (param_list.size() == 7) {
                if (param_list[5] == "-t" || param_list[5] == "--tx_deposit") {
                    api_method_imp_.set_tx_deposit(std::stoull(param_list[6]));
                }
            }
        }
    }
    bool br = api_method_imp_.runContract(g_userinfo, amount, contract, func, para, out_str);
    return br ? 0 : -1;
}

int ApiMethod::registerNode(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Register as node.\n";
        out_str << "\nUSAGE:\n    system registerNode register_deposit node_type nodename [node_sign_key]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "register_deposit  Uint64  REQUIRED  Node register deposit.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_type  STRING  REQUIRED  Node type: edge、validator、advance.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "nodename  STRING  REQUIRED  Node nickname.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "--dividend_ratio"
                << "dividend_ratio  UINT32  For advance nodes to set dividend ratio for voters who support you. Value∈[0,100]\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "--node_sign_key"
                << "Public key of the account or the asset-free public-private key pair.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system registerNode 40000 edge nick --node_sign_key BADKIy4IqZz8S3Zj0+8WAOy666bWfxqFGe4WzE70CKGtXN47COWT1KGRxGX1a02H4m1Kbd5lgpE9hT/+Lg8gnao=\n";
        return -1;
    }

    CLI::App system_app;
    auto app = system_app.add_subcommand("registerNode", "Register as node.");
    uint64_t mortgage;
    app->add_option("register_deposit", mortgage, "Node register deposit.")->required();
    std::string role;
    app->add_option("node_type", role, "Node type: edge、validator、advance.")->required();
    std::string nickname;
    app->add_option("nodename", nickname, "Node nickname.")->required();
    std::string signing_key;
    app->add_option("--node_sign_key", signing_key, "Public key of the account or the asset-free public-private key pair.");
    uint32_t dividend_rate = 0;
    app->add_option("--dividend_ratio", dividend_rate, "For advance nodes to set dividend ratio for voters who support you. Value∈[0,100]")->check(CLI::Range(0, 100));

    // redirect output of CLI:App
    ArgParser arg_parser(param_list, "system");
    auto rtn = redirect_cli_out(system_app, arg_parser, out_str);
    if (rtn != 0) {
        return rtn;
    }

    if (signing_key == "") {
        auto path = g_keystore_dir + "/" + g_userinfo.account;
        auto key_info = attach_parse_keystore(path, out_str);
        signing_key = key_info["public_key"].asString();
    }

    bool br = api_method_imp_.registerNode(g_userinfo, mortgage, role, nickname, signing_key, dividend_rate, out_str);
    return br ? 0 : -1;
}

int ApiMethod::redirect_cli_out(CLI::App & system_app, ArgParser & arg_parser, std::ostringstream & out_str) {
    std::streambuf * const buffer = cout.rdbuf();
    auto osb = out_str.rdbuf();
    cout.rdbuf(osb);
    try {
        (system_app).parse((arg_parser.get_argc()), (arg_parser.get_argv()));
    } catch (const CLI::ParseError & e) {
        (system_app).exit(e);
        cout.rdbuf(buffer);
        return -1;
    }
    cout.rdbuf(buffer);
    return 0;
}

int ApiMethod::parse_cmd(CLI::App & app, const ParamList & param_list, std::ostringstream & out_str, const std::string & cmd_type, const std::string & example) {
    ArgParser arg_parser(param_list, cmd_type);
    auto rtn = redirect_cli_out(app, arg_parser, out_str);
    if (rtn != 0) {
        auto size = param_list.size();
        if (size > 0 && (param_list[size - 1] == COMMAND_HELP_STRING[0] || param_list[size - 1] == COMMAND_HELP_STRING[1])) {
            out_str << "EXAMPLE:\n    ";
            out_str << example;
        }
        return rtn;
    }
    return 0;
}

int ApiMethod::updateNodeType(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Update node type.\n";
        out_str << "\nUSAGE:\n    system updateNodeType node_type\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_type  OBJECT  REQUIRED  The new node type.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system updateNodeType validator\n";
        return -1;
    }

    if (param_list.size() != 2) {
        out_str << "Cmd updateNodeType, lost params. param_count>=2\n";
        return -1;
    }

    std::string role = param_list[1];
    bool br = api_method_imp_.updateNodeType(g_userinfo, role, out_str);
    return br ? 0 : -1;
}

int ApiMethod::stakeNodeDeposit(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Stake node deposit.\n";
        out_str << "\nUSAGE:\n    system stakeDeposit deposit\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "deposit  NUMBER  REQUIRED  Increased deposit(uTOP).\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system stakeDeposit 40000\n";
        return -1;
    }

    if (param_list.size() != 2) {
        out_str << "Cmd stakeDeposit, lost params. param_count=2\n";
        return -1;
    }

    std::string mortgage_str = param_list[1];
    uint64_t mortgage = atoll(mortgage_str.c_str());

    out_str << "stake node deposit " << mortgage << endl;
    bool br = api_method_imp_.stake_node_deposit(g_userinfo, mortgage, out_str);
    return br ? 0 : -1;
}

int ApiMethod::updateNodeInfo(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Update node type, node name, node deposit, dividend ratio and sign key at the same time.\n";
        out_str << "\nUSAGE:\n    system updateNodeInfo node_type node_name updated_deposit_type node_deposit dividend_raito node_sign_key\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_type  STRING REQUIRED Updated node type: edge, validator and advance.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_name  STRING REQUIRED Updated node name.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "updated_deposit_type  STRING REQUIRED 1--Increase node deposit. 2--Decrease node deposit.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_deposit  UINT64 REQUIRED Node deposit\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "dividend_ratio  UINT32 REQUIRED Updated dividend ratio. Value∈[0,100]\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_sign_key  STRING REQUIRED Updated node sign key. Value∈[0,100]\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system updateNodeInfo validator haha 1 100000 50 BADKIy4IqZz8S3Zj0+8WAOy666bWfxqFGe4WzE70CKGtXN47COWT1KGRxGX1a02H4m1Kbd5lgpE9hT/+Lg8gnao=\n";
        return -1;
    }

    if (param_list.size() != 7) {
        out_str << "Cmd updateNodeInfo, lost params. param_count==7\n";
        return -1;
    }

    std::string role = param_list[1];
    std::string name = param_list[2];
    uint32_t updated_deposit_type = std::stoul(param_list[3]);
    uint64_t node_deposit = std::stoull(param_list[4]);
    uint32_t dividend_rate = std::stoul(param_list[5]);
    std::string node_sign_key = param_list[6];
    bool br = api_method_imp_.updateNodeInfo(g_userinfo, role, name, updated_deposit_type, node_deposit, dividend_rate, node_sign_key, out_str);
    return br ? 0 : -1;
}

int ApiMethod::unstakeNodeDeposit(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Unstake node deposit.\n";
        out_str << "\nUSAGE:\n    system unstakeDeposit deposit\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "deposit  NUMBER  REQUIRED  Reduced deposit(uTOP).\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system unstakeDeposit 40000\n";
        return -1;
    }

    if (param_list.size() != 2) {
        out_str << "Cmd unstakeNodeDeposit, lost params. param_count=2\n";
        return -1;
    }

    std::string mortgage_str = param_list[1];
    uint64_t mortgage = atoll(mortgage_str.c_str());

    out_str << "unstake node deposit " << mortgage << endl;
    bool br = api_method_imp_.unstake_node_deposit(g_userinfo, mortgage, out_str);
    return br ? 0 : -1;
}

int ApiMethod::setNodeName(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "setNodeName.\n";
        out_str << "\nUSAGE:\n    system setNodeName nodename\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "nodename  STRING  REQUIRED  New node nickname.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system setNodeName nick1\n";
        return -1;
    }

    if (param_list.size() != 2) {
        LOG("Cmd setNodeName params, lost params. param_count=2");
        return -1;
    }

    std::string nickname = param_list[1];

    out_str << "set nickname " << nickname << endl;
    bool br = api_method_imp_.setNodeName(g_userinfo, nickname, out_str);
    return br ? 0 : -1;
}

int ApiMethod::updateNodeSignKey(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "updateNodeSignKey.\n";
        out_str << "\nUSAGE:\n    system updateNodeSignKey node_sign_key\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_sign_key  STRING  REQUIRED  New node sign_key.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system updateNodeSignKey BADKIy4IqZz8S3Zj0+8WAOy666bWfxqFGe4WzE70CKGtXN47COWT1KGRxGX1a02H4m1Kbd5lgpE9hT/+Lg8gnao=\n";
        return -1;
    }

    if (param_list.size() != 2) {
        LOG("Cmd updateNodeSignKey params, lost params. param_count=2");
        return -1;
    }

    std::string node_sign_key = param_list[1];

    out_str << "update node_sign_key " << node_sign_key << endl;
    bool br = api_method_imp_.updateNodeSignKey(g_userinfo, node_sign_key, out_str);
    return br ? 0 : -1;
}

int ApiMethod::unRegisterNode(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Node unregister.\n";
        out_str << "\nUSAGE:\n    system unregisterNode\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system unregisterNode\n";
        return -1;
    }

    if (param_list.size() != 1) {
        out_str << "Cmd Node deregister (deregister) lost params. param_count=1\n";
        return -1;
    }

    bool br = api_method_imp_.unRegisterNode(g_userinfo, out_str);
    return br ? 0 : -1;
}

int ApiMethod::redeemNodeDeposit(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Redeem node deposit when node register.\n";
        out_str << "\nUSAGE:\n    system redeemNodeDeposit\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system redeemNodeDeposit\n";
        return -1;
    }
    if (param_list.size() != 1) {
        out_str << "Cmd redeemNodeDeposit deposit lost params. param_count=1\n";
        return -1;
    }

    bool br = api_method_imp_.redeemNodeDeposit(g_userinfo, out_str);
    return br ? 0 : -1;
}

int ApiMethod::setDividendRatio(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Set devidend ratio for voters who support you.\n";
        out_str << "\nUSAGE:\n    system setDividendRatio [options] percent\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "percent  INTEGER  REQUIRED   Dividend ratio(≥0，≤100).\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "--tx_deposit"
                << "Transaction deposit,the default is 10,0000uTOP.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system setDividendRatio 80\n";
        return -1;
    }

    if (param_list.size() < 2) {
        out_str << "Cmd Set Dividend Rate lost params. param_count=2\n";
        return -1;
    }
    std::string dividend_rate_str = param_list[1];
    uint32_t dividend_rate = (uint32_t)std::stoul(dividend_rate_str);
    if (param_list.size() == 3) {
        api_method_imp_.set_tx_deposit(std::stoull(param_list[2]));
    }
    bool br = api_method_imp_.setDividendRatio(g_userinfo, dividend_rate, out_str);
    return br ? 0 : -1;
}

int ApiMethod::stakeVote(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Lock TOP tokens to get votes.\n";
        out_str << "\nUSAGE:\n    sendtx stakeVote vote_amount lock_duration\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "vote_amount  STRING  REQUIRED  Amount of votes to be exchanged.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "lock_duration  STRING  REQUIRED  TOP token lock duration,minimum 30 days.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "sendtx stakeVote 1000 30\n";
        return -1;
    }

    if (param_list.size() < 3) {
        out_str << "Cmd buy Vote lost params. param_count<4\n";
        return -1;
    }

    uint64_t mortgage = (uint64_t)atoll(param_list[1].c_str());
    uint16_t lock_duration = (uint16_t)atoi(param_list[2].c_str());

    bool br = api_method_imp_.stakeVote(g_userinfo, mortgage, lock_duration, out_str);
    return br ? 0 : -1;
}

int ApiMethod::unStakeVote(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Unstake votes from your account and unlock TOP tokens.\n";
        out_str << "\nUSAGE:\n    sendtx unstakeVote votes_num\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "votes_num  STRING  REQUIRED  Votes amount,unlock the corresponding TOP token.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "sendtx unstakeVote 200\n";
        return -1;
    }

    if (param_list.size() < 2) {
        out_str << "Cmd sell Vote lost params. param_count<2\n";
        return -1;
    }

    uint64_t amount = (uint64_t)atoll(param_list[1].c_str());

    bool br = api_method_imp_.unStakeVote(g_userinfo, amount, out_str);
    return br ? 0 : -1;
}

int ApiMethod::voteNode(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Vote on nodes.\n";
        out_str << "\nUSAGE:\n    system voteNode node_num node_account_addr vote_num [node_account_addr2] [vote_num2]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_num  INTEGER  REQUIRED  Number of nodes to be voted on.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_account_addr  STRING  REQUIRED  The target account address for the vote.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "vote_num  UINT64  REQUIRED  Number of votes.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system voteNode 1 T00000LN3q57Ek5fBL5xEXk64GH8ruXh2q64PpHS 80000\n";
        out_str << "    system voteNode 2 T00000LN3q57Ek5fBL5xEXk64GH8ruXh2q64PpHS 80000 T00000LXjmqqMjLee8gRv2zQaLpUSdH8L3ewmAV9 10000\n";
        return -1;
    }

    if (param_list.size() < 4) {
        out_str << "Cmd Set Vote lost params. param_count<4\n";
        return -1;
    }

    std::map<std::string, int64_t> vote_infos;
    std::string count_str = param_list[1];
    uint32_t count = atoi(count_str.c_str());
    for (uint32_t i = 0; i < count; ++i) {
        std::string account = param_list[i * 2 + 2];
        std::string amount_str = param_list[i * 2 + 3];
        int64_t amount = atoll(amount_str.c_str());

        vote_infos.insert(std::map<std::string, int64_t>::value_type(account, amount));
    }

    bool br = api_method_imp_.voteNode(g_userinfo, vote_infos, out_str);
    return br ? 0 : -1;
}

int ApiMethod::unVoteNode(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Unvote on nodes.\n";
        out_str << "\nUSAGE:\n    system unvoteNode node_num node_account_addr vote_num [node_account_addr2] [vote_num2]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_num  INTEGER  REQUIRED  Number of nodes to be voted on.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "node_account_addr  STRING  REQUIRED  Account address to be cancelled vote.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "vote_num  UINT64  REQUIRED  Number of cancelled votes.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system unvoteNode 1 T00000LN3q57Ek5fBL5xEXk64GH8ruXh2q64PpHS 80000\n";
        out_str << "    system unvoteNode 2 T00000LN3q57Ek5fBL5xEXk64GH8ruXh2q64PpHS 80000 T00000LXjmqqMjLee8gRv2zQaLpUSdH8L3ewmAV9 10000\n";
        return -1;
    }

    if (param_list.size() < 4) {
        out_str << "Cmd Set Vote (avote) lost params. param_count<4\n";
        return -1;
    }

    std::map<std::string, int64_t> vote_infos;
    std::string count_str = param_list[1];
    uint32_t count = atoi(count_str.c_str());
    for (uint32_t i = 0; i < count; ++i) {
        std::string account = param_list[i * 2 + 2];
        std::string amount_str = param_list[i * 2 + 3];
        int64_t amount = atoll(amount_str.c_str());

        vote_infos.insert(std::map<std::string, int64_t>::value_type(account, amount));
    }

    bool br = api_method_imp_.unVoteNode(g_userinfo, vote_infos, out_str);
    return br ? 0 : -1;
}

int ApiMethod::getBlock(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Get block information by account address and block height.\n";
        out_str << "\nUSAGE:\n    get block account_addr height\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "account_addr String REQUIRED Use general account address to query unit block and table block account address to query table block.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "height Unit64/String REQUIRED Integer of a block number, or the String \"latest\".\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "get block T00000LgPNkFe6X9DbX1aXzX9SuJhRuhM9bCs5sM latest\n";
        out_str << "    get block T00000LgPNkFe6X9DbX1aXzX9SuJhRuhM9bCs5sM@0 1\n";
        return -1;
    }

    if (param_list.size() < 3) {
        out_str << "Cmd getBlock lost params. param_count < 3\n";
        return -1;
    }

    std::string owner = param_list[1];
    std::string height = param_list[2];
    bool br = api_method_imp_.getBlock(g_userinfo, owner, height, out_str);
    return br ? 0 : -1;
}

int ApiMethod::claimNodeReward(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Claim the node rewards.\n";
        out_str << "\nUSAGE:\n    system claimNodeReward\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system claimNodeReward\n";
        return -1;
    }

    if (param_list.size() != 1) {
        out_str << "Cmd ClaimNodeReward lost params. param_count=1\n";
        return -1;
    }

    bool br = api_method_imp_.claimNodeReward(g_userinfo, out_str);
    return br ? 0 : -1;
}

int ApiMethod::claimVoterDividend(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Claim the voter dividend.\n";
        out_str << "\nUSAGE:\n    system claimVoterDividend\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system claimVoterDividend\n";
        return -1;
    }
    if (param_list.size() != 1) {
        out_str << "Cmd Return ClaimReward (claim) lost params. param_count=2\n";
        return -1;
    }

    bool br = api_method_imp_.claimVoterDividend(g_userinfo, out_str);
    return br ? 0 : -1;
}

int ApiMethod::GenerateKeys(const ParamList & param_list) {
    if (param_list.size() < 2) {
        LOG("Cmd Generate Keys(gk) lost params. param_count < 2 ");
        return -1;
    }

    std::string count_str = param_list[1];
    uint32_t count = atoi(count_str.c_str());

    // not store to file
    std::string priv_key;
    std::string pub_key;
    if (param_list.size() == 2) {
        for (uint32_t i = 0; i < count; ++i)
            api_method_imp_.make_keys_base64(std::cout);
    } else {
        std::string filename = param_list[2];

        std::fstream fs;
        fs.open(filename, std::ios_base::out);
        if (!fs.is_open()) {
            fs.clear();
            LOG("Generate Keys(gk) open file failed.");
            return -1;
        }
        for (uint32_t i = 0; i < count; ++i)
            api_method_imp_.make_keys_base64(fs);
        fs.close();
    }

    return 0;
}

int ApiMethod::CheckKeys(const ParamList & param_list) {
    if (param_list.size() != 2) {
        LOG("Cmd Check Keys(ck) lost params. param_count != 2 ");
        return -1;
    }

    std::string filename = param_list[1];
    std::fstream fs;
    fs.open(filename, std::ios_base::in);
    if (!fs.is_open()) {
        fs.clear();
        LOG("Check Keys(ck) open file failed.");
        return -1;
    }

    std::string account;
    std::string priv_key, pub_key, addr;
    while (fs >> account) {
        auto first_delm_pos = account.find_first_of(',');
        auto last_delm_pos = account.find_last_of(',');
        priv_key = account.substr(0, first_delm_pos);
        pub_key = account.substr(first_delm_pos + 1, last_delm_pos - first_delm_pos - 1);
        addr = account.substr(last_delm_pos + 1);
        auto main_addr = addr;
        if (!api_method_imp_.validate_key(utility::base64_decode(priv_key), pub_key, addr)) {
            LOG(priv_key);
            LOG(pub_key);
            LOG(addr);
            LOG("Check Keys(ck) failed: validate main key");
            return -1;
        }

        int sub_keys = 2;
        for (auto i = 0; i < sub_keys; ++i) {
            fs >> account;
            first_delm_pos = account.find_first_of(',');
            last_delm_pos = account.find_last_of(',');
            priv_key = account.substr(0, first_delm_pos);
            pub_key = account.substr(first_delm_pos + 1, last_delm_pos - first_delm_pos - 1);
            addr = account.substr(last_delm_pos + 1);
            if (!api_method_imp_.validate_key(utility::base64_decode(priv_key), pub_key, addr, main_addr)) {
                LOG(priv_key);
                LOG(pub_key);
                LOG(addr);
                LOG("Check Keys(ck) failed: validate sub key");
                return -1;
            }
        }
    }

    LOG("All key is correct!");
    return 0;
}

// int ApiMethod::ActivateAccounts(const ParamList& param_list) {
//     if (param_list.size() != 2) {
//         LOG("Cmd Activate Accounts(aa) lost params. param_count != 2 ");
//         return -1;
//     }

//     std::string filename = param_list[1];
//     std::fstream fs;
//     fs.open(filename, std::ios_base::in);
//     if (!fs.is_open()) {
//         fs.clear();
//         LOG(" Activate Accounts (aa) open file failed.");
//         return -1;
//     }

//     std::string addr;
//     std::string privkey_base64;
//     fs >> addr >> privkey_base64;
//     g_userinfo.account = addr;
//     std::string priv = utility::base64_decode(privkey_base64);
//     std::vector<uint8_t> keys = hex_to_uint(priv);
//     if (keys.size() != 32)  {
//         LOG(" Activate Accounts (aa) decode private key error.");
//         return -1;
//     }
//     memcpy(g_userinfo.private_key, keys.data(), keys.size());

//     uint64_t token;
//     int count = 0;
//     while (fs >> addr >> token) {
//          count++;
//          LOG(count, "\taddr: ", addr);
//          LOG("--------------------------------------------");
//          api_method_imp_.getAccount(g_userinfo, g_userinfo.account);
//          utility::usleep(1000000);
//          std::string memo;
//          api_method_imp_.transfer(g_userinfo, g_userinfo.account, addr, token, memo);
//          utility::usleep(1000000);
//          LOG("--------------------------------------------");
//     }

//     return 0;
// }

// int ApiMethod::CheckActivateAccounts(const ParamList& param_list) {

//     if (param_list.size() != 3) {
//         LOG("Cmd Check Activate Accounts(caa) lost params. param_count != 3 ");
//         return -1;
//     }

//     std::string filename = param_list[1];
//     std::fstream fs;
//     fs.open(filename, std::ios_base::in);
//     if (!fs.is_open()) {
//         fs.clear();
//         LOG("Check Activate Accounts(caa) open file failed.");
//         return -1;
//     }

//     user_info  query_user;
//     filename = param_list[2];
//     std::fstream result;
//     result.open(filename, std::ios_base::out);
//     if (!result.is_open()) {
//         result.clear();
//         LOG("Check Activate Accounts(caa) open file failed.");
//         return -1;
//     }

//     std::string addr;
//     int count = 0;
//     int error_count = 0;
//     while(fs >> addr) {
//         query_user.account = addr;
//         count++;
//         LOG(count, "\t check addr: ", addr);
//         LOG("--------------------------------------------");
//         api_method_imp_.getAccount(query_user, query_user.account, [&addr, &result, &error_count](AccountInfoResult* res){
//             if (res->error != static_cast<int>(ErrorCode::Success)) {
//                 LOG("FOUND ERROR");
//                 result << addr << '\n';
//                 error_count++;
//             } else {
//                 LOG("SUCCESS");
//             }
//         });
//         utility::usleep(1000000);
//         LOG("--------------------------------------------");
//     }

//     result << "total: " << error_count << '\n';
//     fs.close();
//     result.close();

//     return 0;
// }100

int ApiMethod::submitProposal(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Submit the on-chain governance proposal.\n";
        out_str << "\nUSAGE:\n    system submitProposal  proposal_type target value proposal_deposit effective_timer_height\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "proposal_type  UINT8  REQUIRED  Proposal Type：1/3/4/5/6--on-chain governance parameter modification proposal；2--community fund management proposal.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "target  STRING  REQUIRED  When proposal_type is \"1/3/4/5/6\": target is on-chain governance parameter. When proposal_type is \"2\": target is burn account address "
                   "\"T-!-Ebj8hBvoLdvcEEUwNZ423zM3Kh9d4nL1Ug\".\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "value  STRING  REQUIRED   When target is on-chain governance parameter, value=new parameter value. When target is burn account address, value=transfered "
                   "amount, the unit is uTOP.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "proposal_deposit  UINT64  REQUIRED  Proposal deposit(on-chain governance parameter).\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "effective_timer_height  UINT64  REQUIRED   Clock height for proposal to take effect.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system submitProposal 1 min_archive_deposit 20000 100000000 300\n";
        out_str << "    system submitProposal 2 T-!-Ebj8hBvoLdvcEEUwNZ423zM3Kh9d4nL1Ug 10000 100000000 300\n";
        return -1;
    }

    if (param_list.size() < 6) {
        LOG("Cmd Add Proposal lost params. param_count<5");
        return -1;
    }

    uint8_t type = (uint8_t)top::base::xstring_utl::toint32(param_list[1]);
    std::string target = param_list[2];
    std::string value = param_list[3];
    uint64_t deposit = top::base::xstring_utl::touint64(param_list[4]);
    uint64_t effective_timer_height = top::base::xstring_utl::touint64(param_list[5]);

    bool br = api_method_imp_.submitProposal(g_userinfo, type, target, value, deposit, effective_timer_height, out_str);
    return br ? 0 : -1;
}

int ApiMethod::withdrawProposal(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Withdraw proposal.\n";
        out_str << "\nUSAGE:\n    system withdrawProposal proposal_id\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "proposal_id  STRING  REQUIRED  Proposal ID.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system withdrawProposal 10\n";
        return -1;
    }

    if (param_list.size() < 2) {
        out_str << "Cmd Withdraw Proposal lost params. param_count<1\n";
        return -1;
    }
    std::string proposal_id = param_list[1];

    bool br = api_method_imp_.withdrawProposal(g_userinfo, proposal_id, out_str);
    return br ? 0 : -1;
}

int ApiMethod::tccVote(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "TCC(TOP Network Community Council)  vote on proposal.\n";
        out_str << "\nUSAGE:\n    system tccVote proposal_id opinion\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "proposal_id  STRING  REQUIRED   Proposal ID.\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "opinion  STRING  REQUIRED   Vote:true or false.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system tccVote 10 true\n";
        return -1;
    }

    if (param_list.size() < 3) {
        out_str << "Cmd Add Proposal (avote) lost params. param_count<2\n";
        return -1;
    }

    // int i = 0;
    // for (const auto& entry : param_list) {
    //     std::cout << "i = " << i++ << ", vote proposal parameter: " << entry << std::endl;
    // }

    std::string proposal_id = param_list[1];
    bool option = false;
    if (param_list[2] == "true" || param_list[2] == "TRUE") {
        option = true;
    }

    bool br = api_method_imp_.tccVote(g_userinfo, proposal_id, option, out_str);
    return br ? 0 : -1;
}

int ApiMethod::getChainInfo(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Get chain info.\n";
        out_str << "\nUSAGE:\n    get chaininfo\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "get chaininfo\n";
        return -1;
    }

    if (param_list.size() < 1) {
        out_str << "Cmd Chain Info lost params. param_count<2\n";
        return -1;
    }

    bool br = api_method_imp_.getChainInfo(g_userinfo, out_str);
    return br ? 0 : -1;
}

int ApiMethod::queryNodeInfo(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Query node information.\n";
        out_str << "\nUSAGE:\n    system queryNodeInfo [target_account_addr]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "target_account_addr  STRING  REQUIRED  Node account address.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system queryNodeInfo  //get all node info\n";
        out_str << "    system queryNodeInfo T00000LLdWiAhUMyiXq39pUbSSRUdjNN6gQHb9bm  //get target node info\n";
        return -1;
    }

    if (param_list.size() < 1) {
        out_str << "Cmd Node Info lost params. param_count<1\n";
        return -1;
    }

    // int i = 0;
    // for (const auto& entry : param_list) {
    //     std::cout << "i = " << i++ << ", node info parameter: " << entry << std::endl;
    // }

    std::string target = param_list.size() == 2 ? param_list[1] : "";
    bool br = api_method_imp_.queryNodeInfo(g_userinfo, target, out_str);
    return br ? 0 : -1;
}

int ApiMethod::queryNodeReward(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Query specific node rewards.\n";
        out_str << "\nUSAGE:\n    system queryNodeReward [target_account_addr]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "target_account_addr  STRING  REQUIRED  Target node account address.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system queryNodeReward T00000LLdWiAhUMyiXq39pUbSSRUdjNN6gQHb9bm  //get target node reward\n";
        return -1;
    }
    if (param_list.size() < 1) {
        out_str << "Cmd Node Reward lost params. param_count<2\n";
        return -1;
    }

    // int i = 0;
    // for (const auto& entry : param_list) {
    //     std::cout << "i = " << i++ << ", node reward parameter: " << entry << std::endl;
    // }

    std::string target = param_list.size() == 2 ? param_list[1] : "";
    bool br = api_method_imp_.queryNodeReward(g_userinfo, target, out_str);
    return br ? 0 : -1;
}

int ApiMethod::listVoteUsed(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Query vote-used distribution.\n";
        out_str << "\nUSAGE:\n    system listVoteUsed target_account_addr\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "target_account_addr  STRING  REQUIRED  Voter account address.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system listVoteUsed T00000LLdWiAhUMyiXq39pUbSSRUdjNN6gQHb9bm\n";
        return -1;
    }

    if (param_list.size() < 1) {
        out_str << "Cmd Vote Distribution lost params. param_count<2\n";
        return -1;
    }

    // int i = 0;
    // for (const auto& entry : param_list) {
    //     std::cout << "i = " << i++ << ", Vote Distribution parameter: " << entry << std::endl;
    // }

    std::string target = param_list.size() == 2 ? param_list[1] : "";
    bool br = api_method_imp_.listVoteUsed(g_userinfo, target, out_str);
    return br ? 0 : -1;
}

int ApiMethod::queryVoterDividend(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Query specific voter dividend.\n";
        out_str << "\nUSAGE:\n    system queryVoterDividend target_account_addr\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "target_account_addr  STRING  REQUIRED  Voter account address.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system queryVoterDividend T00000LLdWiAhUMyiXq39pUbSSRUdjNN6gQHb9bm\n";
        return -1;
    }

    if (param_list.size() < 1) {
        out_str << "Cmd Voter Reward lost params. param_count<2\n";
        return -1;
    }

    // int i = 0;
    // for (const auto& entry : param_list) {
    //     std::cout << "i = " << i++ << ", voter reward parameter: " << entry << std::endl;
    // }

    std::string target = param_list.size() == 2 ? param_list[1] : "";
    bool br = api_method_imp_.queryVoterDividend(g_userinfo, target, out_str);
    return br ? 0 : -1;
}

int ApiMethod::queryProposal(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Query specific proposal details.\n";
        out_str << "\nUSAGE:\n    system queryProposal [proposal_id]\n";
        out_str << "\nParams:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ')
                << "proposal_id  STRING  Proposal ID, if null, queries all proposals by default.\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "system queryProposal //get all proposal\n";
        out_str << "    system queryProposal aaa //get target proposal\n";
        return -1;
    }

    if (param_list.size() < 1) {
        out_str << "Cmd Query Proposal lost params. param_count<2\n";
        return -1;
    }

    // int i = 0;
    // for (const auto& entry : param_list) {
    //     std::cout << "i = " << i++ << ", query proposal parameter: " << entry << std::endl;
    // }

    std::string target = param_list.size() == 2 ? param_list[1] : "";
    bool br = api_method_imp_.queryProposal(g_userinfo, target, out_str);
    return br ? 0 : -1;
}

int ApiMethod::getCGP(const ParamList & param_list, std::ostringstream & out_str) {
    if (param_list.size() > 1 && (COMMAND_HELP_STRING[0] == param_list[1] || COMMAND_HELP_STRING[1] == param_list[1])) {
        out_str << "Get on-chain governance parameters.\n";
        out_str << "\nUSAGE:\n    get CGP\n";
        out_str << "\nOPTIONS:\n";
        out_str << std::setw(INDENT_WIDTH) << std::setfill(' ') << ' ' << std::left << std::setw(HELP_WIDTH - INDENT_WIDTH) << std::setfill(' ') << "-h --help"
                << "Show help information for one command.\n";
        out_str << "\nEXAMPLE:\n    ";
        out_str << "get CGP\n";
        return -1;
    }

    if (param_list.size() < 1) {
        out_str << "Cmd Query OnchainParams lost params. param_count<2\n";
        return -1;
    }
    std::string target;
    bool br = api_method_imp_.getCGP(g_userinfo, target, out_str);
    return br ? 0 : -1;
}

ApiMethod::CommandFunc ApiMethod::get_method(const std::string & method, std::ostringstream & out_str, Command_type type) {
    if (method.empty()) {
        out_str << "ERROR: Unknown Command." << std::endl;
        out_str << "Try 'help' for more information." << std::endl;
    }

    MethodFuncMap func_map = methods_;
    switch (type) {
    case Command_type::toplevel:
        break;
    case Command_type::subcommands:
        func_map = level_methods_;
        break;
    case Command_type::get:
        func_map = get_methods_;
        break;
    case Command_type::system:
        func_map = system_methods_;
        break;
    case Command_type::sendtransaction:
        func_map = sendtransaction_methods_;
        break;
    case Command_type::wallet:
        func_map = wallet_methods_;
        break;
    case Command_type::debug:
        func_map = debug_methods_;
        break;
    }

    auto it = func_map.find(method);
    if (it != func_map.end()) {
        auto p = it->second;
        return p.second;
    }
    return nullptr;
}

void ApiMethod::regist_method(const std::string & method, CommandFunc func, const std::string & info, Command_type type) {
    std::string lower_name = method;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    cmd_name[lower_name] = method;
    switch (type) {
    case Command_type::toplevel:
        methods_[lower_name] = std::make_pair(info, func);
        break;
    case Command_type::subcommands:
        level_methods_[lower_name] = std::make_pair(info, func);
        break;
    case Command_type::get:
        get_methods_[lower_name] = std::make_pair(info, func);
        break;
    case Command_type::system:
        system_methods_[lower_name] = std::make_pair(info, func);
        break;
    case Command_type::sendtransaction:
        sendtransaction_methods_[lower_name] = std::make_pair(info, func);
        break;
    case Command_type::wallet:
        wallet_methods_[lower_name] = std::make_pair(info, func);
        break;
    case Command_type::debug:
        debug_methods_[lower_name] = std::make_pair(info, func);
        break;
    }
}

void ApiMethod::dump_userinfo(const user_info & info) {
    // constexpr static auto key_len = 32;

    std::cout.width(16);
    std::cout.setf(std::ios::left);

    std::cout << "identity_token: " << info.identity_token << std::endl;
    std::cout << "account: " << info.account << std::endl;
    std::cout << "last_hash: " << info.last_hash << std::endl;
    std::cout << "balance: " << info.balance << std::endl;
    std::cout << "nonce: " << info.nonce << std::endl;
#ifdef DEBUG
// std::cout << "secret_key: " << info.secret_key << std::endl;
// std::cout << "sign_method: " << info.sign_method << std::endl;
// std::cout << "sign_version: " << info.sign_version << std::endl;
// std::string private_key = uint_to_str(info.private_key, uinfo::key_len);
// std::cout << "private_key: " << private_key.c_str() << std::endl;
// if (private_key.length() > 2) {
//     std::cout << "public_key: " << api_method_imp_.get_public_key(info.private_key) << std::endl;
// }

// std::cout << "child_account: " << info.child.account << std::endl;
// private_key = uint_to_str(info.child.private_key, uinfo::key_len);
// std::cout << "child_private_key: " << private_key.c_str() << std::endl;

// std::cout << "contract_account: " << info.contract.account << std::endl;
// private_key = uint_to_str(info.contract.private_key, uinfo::key_len);
// std::cout << "contract_private_key: " << private_key.c_str() << std::endl;
// auto table_id = top::data::xaccount_mapping::account_to_table_id(info.account);
// std::stringstream ss;
// ss << "(0x" << std::hex << std::setw(4) << std::setfill('0') << table_id << ")";
// std::cout << "table id: " << table_id << ss.str() << std::endl;
#endif
}

int ApiMethod::set_userinfo() {
    api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
    return 0;
}

void ApiMethod::set_keystore_path(const std::string & data_dir) {
    g_data_dir = data_dir;
    g_keystore_dir = data_dir + "/keystore";
}

void ApiMethod::change_trans_mode(bool use_http) {
    if (use_http) {
        g_server_host_port = config_file::instance()->get_string(net_setting, http_host);
        if (g_server_host_port.empty()) {
            g_server_host_port = SERVER_HOST_PORT_HTTP;
        }
        trans_base::s_defaule_mode = TransMode::HTTP;
        // std::cout << "Using trans mode - HTTP: " << g_server_host_port << std::endl;
    } else {
        g_server_host_port = config_file::instance()->get_string(net_setting, ws_host);
        if (g_server_host_port.empty()) {
            g_server_host_port = SERVER_HOST_PORT_WS;
        }
        trans_base::s_defaule_mode = TransMode::WS;
        // std::cout << "Using trans mode - WS: " << g_server_host_port  << std::endl;
    }
#if  defined(DEBUG) || defined(RELEASEDEBINFO)
    // std::cout << "[debug]edge_domain_name old: " << g_edge_domain << std::endl;
    char* topio_home = getenv("TOPIO_HOME");
//    std::cout <<"data_dir:" << topio_home << std::endl;
    if (topio_home) {
        g_data_dir = topio_home;
//        std::cout <<"data_dir:" << g_data_dir << std::endl;
    }
    auto edge_config_path = g_data_dir + "/.edge_config.json";
    std::ifstream edge_config_file(edge_config_path, std::ios::in);
    if (!edge_config_file) {
        std::cout << "Edge domain name file: " << edge_config_path << " Not Exist" << std::endl;
    } else {
        std::stringstream buffer;
        buffer << edge_config_file.rdbuf();
        string edge_info = buffer.str();
        xJson::Reader reader;
        xJson::Value edge_info_js;
        if (!reader.parse(edge_info, edge_info_js)) {
            std::cout << "Edge domain name file: " << edge_config_path << " parse error" << endl;
        } else {
            if (!edge_info_js["edge_ip"].asString().empty()) {
                g_server_host_port = edge_info_js["edge_ip"].asString();
            }
            if (!edge_info_js["edge_domain_name"].asString().empty()) {
                g_edge_domain = edge_info_js["edge_domain_name"].asString();
            }
        }
    }

    std::cout << "[debug]edge_domain_name: " << g_edge_domain << std::endl;
    std::cout << "[debug]edge_ip: " << g_server_host_port << std::endl;
#endif
}

void ApiMethod::get_token() {
    api_method_imp_.passport(g_userinfo);
    sleep(1);
}

int ApiMethod::get_account_info(std::ostringstream & out_str, xJson::Value & root) {
    api_method_imp_.getAccount(g_userinfo, g_userinfo.account, out_str);
    auto result = out_str.str();
    xJson::Reader reader;
    if (reader.parse(result, root)) {
        if (root["data"].empty()) {
            cout << g_userinfo.account << " not found on chain!" << endl;
            return 1;
        }
    } else {
        cout << result << endl;
        return 1;
    }
    sleep(1);
    return 0;
}

void ApiMethod::block_prune(std::string & prune_enable, std::ostringstream & out_str) {
    top::base::xstring_utl::tolower_string(prune_enable);
    if (prune_enable != "off" && prune_enable != "on") {
        out_str << "Please set auto prune data to On or Off." << std::endl;
        return;
    }
    std::string extra_config = g_data_dir + "/.extra_conf.json";
    xJson::Value key_info_js;
    std::ifstream keyfile(extra_config, std::ios::in);
    if (keyfile) {
        std::stringstream buffer;
        buffer << keyfile.rdbuf();
        keyfile.close();
        std::string key_info = buffer.str();
        xJson::Reader reader;
        // ignore any error when parse
        reader.parse(key_info, key_info_js);
    }

    key_info_js["auto_prune_data"] = prune_enable;

    // dump new json to file
    xJson::StyledWriter new_sw;
    std::ofstream os;
    os.open(extra_config);
    if (!os.is_open()) {
        out_str << "Dump file failed" << std::endl;
        return;
    }
    os << new_sw.write(key_info_js);
    os.close();
    if (prune_enable == "off")
        out_str << "Set auto prune data Off successfully." << std::endl;
    else if (prune_enable == "on")
        out_str << "Set auto prune data On successfully." << std::endl;
}
}  // namespace xChainSDK
