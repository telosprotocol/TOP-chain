#include "api_method.h"

#include "base/utility.h"
#include "base/config_file.h"
#include "task/request_task.h"
#include "task/task_dispatcher.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xcrypto.h"
#include "xcrypto/xckey.h"
#include "xcrypto_util.h"
#include "xdata/xnative_contract_address.h"
#include "xpbase/base/top_utils.h"

#include "console_log.h"

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

// #[deprecated]
static const std::string DEPRECATED_OLD_DEFAULT_KEY = " ";

// local test
// #define NEXT_VERSION
#ifdef NEXT_VERSION
#    define __compatibility_begin(...) if (false) {
#    define __compatibility_end(...) }
#else
#    define __compatibility_begin(...)
#    define __compatibility_end(...)
#endif

bool check_pri_key(const std::string & str_pri) {
    return BASE64_PRI_KEY_LEN == str_pri.size() || HEX_PRI_KEY_LEN == str_pri.size();
}
bool check_account_address(const std::string& account)
{
    top::base::xvaccount_t _vaccount(account);
    return _vaccount.is_unit_address();
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
    top::base::xvaccount_t _vaccount(account);
    if (_vaccount.is_eth_address())
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
        string pri_key;
        if (!hex_ed_key.empty()) {
            top::base::xvaccount_t _vaccount(account);
            if (_vaccount.is_eth_address())
                get_eth_file(account);
            std::string path = g_keystore_dir + '/' + account;
            decrypt_keystore_file_by_kdf_key(hex_ed_key, path, pri_key);
        }
        return pri_key;
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
    top::base::xvaccount_t _vaccount(account);
    if (_vaccount.is_eth_address())
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

bool ApiMethod::set_default_prikey(std::ostringstream & out_str) {
    if (g_userinfo.account.empty()) {
        // if daemon already have default account. get prikey:
        std::string str_pri = get_prikey_from_daemon(out_str);
        if (!str_pri.empty()) {
            set_g_userinfo(str_pri);
            return true;
        } else {
            CONSOLE_INFO("Please set a default account by command `topio wallet setDefaultAccount`. ");
            return false;
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

void ApiMethod::create_account(const string & pw_path, std::ostringstream & out_str) {
    std::string new_pw{""};

    auto pswd_type = pw_path.empty() ? password_type::interactive : password_type::file_path;
    auto result = get_password(keystore_type::account_key, pswd_type, pw_path);
    if (result.first == false) {
        return;
    } else {
        new_pw = result.second;
    }

    std::string dir;
    auto path = create_new_keystore(new_pw, dir);
    out_str << "Successfully create an account locally!\n" << std::endl;

    out_str << "Account Address: " << g_userinfo.account << std::endl;
    out_str << "Owner public-Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << "\n\n";
    out_str << "You can share your public key and account address with anyone.Others need them to interact with you!" << std::endl;
    out_str << "You must nerver share the private key or account keystore file with anyone!They control access to your funds!" << std::endl;
    out_str << "You must backup your account keystore file!Without the file, you will not be able to access the funds in your account!" << std::endl;
    out_str << "You must remember your password!Without the password,it's impossible to use the keystore file!" << std::endl;

    return;
}

void ApiMethod::create_key(std::string & owner_account, const string & pw_path, std::ostringstream & out_str) {
    std::string new_pw{""};

    auto pswd_type = pw_path.empty() ? password_type::interactive : password_type::file_path;
    auto result = get_password(keystore_type::worker_key, pswd_type, pw_path);
    if (result.first == false) {
        return;
    } else {
        new_pw = result.second;
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
    auto path = create_new_keystore(new_pw, dir, true, owner_account);
    out_str << "Successfully create an worker keystore file!\n" << std::endl;
    out_str << "Account Address: " << owner_account << std::endl;
    out_str << "Public Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << "\n\n";
    out_str << "You can share your public key with anyone.Others need it to interact with you!" << std::endl;
    out_str << "You must nerver share the private key or keystore file with anyone!They can use them to make the node malicious." << std::endl;
    out_str << "You must backup your keystore file!Without the file,you may not be able to send transactions." << std::endl;
    out_str << "You must remember your password!Without the password,it's impossible to use the keystore file!" << std::endl;
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
        top::base::xvaccount_t _vaccount(account);
        if (_vaccount.is_eth_address())
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
    top::base::xvaccount_t _vaccount(account);
    if (_vaccount.is_eth_address())
        get_eth_file(account_file);
    const std::string store_path = g_keystore_dir + "/" + account_file;
    std::fstream store_file;
    store_file.open(store_path, std::ios::in);
    if (!store_file) {
        out_str << "The account's owner keystore file does not exist in wallet." << std::endl;
        return;
    }

    xJson::Value keystore_info;
    if (parse_keystore(store_path, keystore_info) == false) {
        CONSOLE_ERROR("keystore parse error, check keystore file %s", store_path.c_str());
        return;
    }

    std::string pri_key;
    std::string kdf_key;
    __compatibility_begin("try old empty_pw \" \" ");
    if (decrypt_keystore_by_password(DEPRECATED_OLD_DEFAULT_KEY, keystore_info, pri_key) == false) {
        // " " password not right.
    } else {
        xassert(!pri_key.empty());
        std::cout << "It is recommended to set a password to protect the keystore file!" << std::endl;
        // continue to set default.
        if (decrypt_get_kdf_key(DEPRECATED_OLD_DEFAULT_KEY, keystore_info, kdf_key) == false) {
            xassert(false);  // not possible.
            return;
        }
    }
    if (pri_key.empty()) {
        __compatibility_end();
        std::string pw;
        if (pw_path.empty()) {
            std::cout << "Please input password." << std::endl;
            pw = input_hiding();
        } else {
            auto result = get_password(keystore_type::account_key, password_type::file_path, pw_path);
            if (result.first == false) {
                return;
            } else {
                pw = result.second;
            }
        }
        if (decrypt_keystore_by_password(pw, keystore_info, pri_key) == false) {
            // password not right.
            out_str << "Wrong password, set default account failed." << get_keystore_hint(keystore_info) << std::endl;
            return;
        }
        if (decrypt_get_kdf_key(pw, keystore_info, kdf_key) == false) {
            xassert(false);  // not possible.
            return;
        }

        __compatibility_begin("try old empty_pw \" \" ");
    }
    __compatibility_end();

    if (!pri_key.empty() && set_prikey_to_daemon(account, kdf_key, out_str) == 0) {
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
        out_str << "No file with public_key " << public_key << endl;
        return;
    }
    // todo ------------------↑ refactor
    xJson::Value keystore_info;
    if (parse_keystore(path, keystore_info) == false) {
        CONSOLE_ERROR("keystore parse error, check keystore file %s", path.c_str());
        return;
    }

    std::string pri_key;

    std::cout << "Please input old password. [If the keystore has no password, press Enter directly.(empty password will be deprecated soon)]" << std::endl;
    // COMPATIBILITY:
    auto pw = input_hiding();
    __compatibility_begin("temporarily allow empty password to reset. if old_pw == empty, try \" \" ");
    if (pw.empty()) {
        if (decrypt_keystore_by_password(DEPRECATED_OLD_DEFAULT_KEY, keystore_info, pri_key) == false) {
            // " " password not right.
        } else {
            std::cout << "It is recommended to set a password to protect the keystore file!" << std::endl;
            xassert(!pri_key.empty());
            // continue to reset password.
        }
    }
    if (pri_key.empty()) {
        __compatibility_end();
        // TODO NEXT_NEXT_VERSION can uncomments: or not , not a big deal.
        // auto pw = input_hiding_no_empty("Password not allow to be empty!");
        if (decrypt_keystore_by_password(pw, keystore_info, pri_key) == false) {
            // password not right.
            out_str << "Old password wrong," << get_keystore_hint(keystore_info) << std::endl;
            // out_str << 
            out_str << "Reset password failed." << std::endl;
            return;
        }
        __compatibility_begin("temporarily allow empty password to reset. if old_pw == empty, try \" \" ");
    }
    __compatibility_end();

    std::string new_password;
    // keystore type is not import here. whatever.
    auto result = get_password(keystore_type::account_key, password_type::interactive, "", true);
    if (result.first == false) {
        xassert(false); // not possible since not allow to use pswd file when reset.
        return;
    } else {
        new_password = result.second;
    }

    std::ofstream key_file(path, std::ios::out | std::ios::trunc);
    update_keystore_file(new_password, pri_key, key_file, keystore_info);
    out_str << "Reset password successfully!." << std::endl;
    return;

    // todo reset default account.
}

void ApiMethod::import_account(std::string const & pw_path, std::ostringstream & out_str) {
    std::string new_pw{""};

    auto pswd_type = pw_path.empty() ? password_type::interactive : password_type::file_path;
    auto result = get_password(keystore_type::account_key, pswd_type, pw_path);
    if (result.first == false) {
        return;
    } else {
        new_pw = result.second;
    }

    std::string pri_str;
    if (input_pri_key(pri_str) != 0)
        return;

    std::string dir;
    std::string path = create_new_keystore(new_pw, dir, pri_str);
    if (path.empty())
        return;

    out_str << "Import successfully.\n" << std::endl;
    out_str << "Account Address: " << g_userinfo.account << std::endl;
    out_str << "Public-Key: " << top::utl::xcrypto_util::get_base64_public_key(g_userinfo.private_key) << "\n\n";
    return;
}

void ApiMethod::export_account(const std::string & account, std::ostringstream & out_str) {
    if (account.empty()) {
        CONSOLE_INFO("You need to identify the account you want to export.");
        list_accounts(out_str);
        return;
    }

    std::vector<std::string> keys = scan_key_dir(g_keystore_dir);
    if (keys.size() == 0) {
        out_str << "There is no account in wallet." << std::endl;
        return;
    }
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] != account)
            continue;

        std::string keystore_file = g_keystore_dir + "/" + keys[i];

        // todo ------------------↑ refactor
        xJson::Value keystore_info;
        if (parse_keystore(keystore_file, keystore_info) == false) {
            CONSOLE_ERROR("keystore parse error, check keystore file %s", keystore_file.c_str());
            return;
        }

        std::string pri_key;
        __compatibility_begin("try old empty_pw \" \" ");
        if (decrypt_keystore_by_password(DEPRECATED_OLD_DEFAULT_KEY, keystore_info, pri_key) == false) {
            // " " password not right.
        } else {
            std::cout << "It is recommended to set a password to protect the keystore file!" << std::endl;
            xassert(!pri_key.empty());
        }
        if (pri_key.empty()) {
            __compatibility_end();
            std::string pw;
            std::cout << "Please input password." << std::endl;
            pw = input_hiding();
            if (decrypt_keystore_by_password(pw, keystore_info, pri_key) == false) {
                // password not right.
                out_str << "Wrong password, export account failed." << get_keystore_hint(keystore_info) << std::endl;
                return;
            }
            __compatibility_begin("try old empty_pw \" \" ");
        }
        __compatibility_end();

        out_str << "Export successfully.\n" << std::endl;
        out_str << "Keystore file: " << keystore_file << std::endl;
        out_str << "Account Address: " << account << std::endl;
        out_str << "Private-Key: " << pri_key << "\n\n";

        std::ifstream keyfile(keystore_file, std::ios::in);
        if (!keyfile) {
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
                top::base::xvaccount_t _vaccount(target_node_id);
                if (_vaccount.is_eth_address())
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

    // todo ------------------↑ refactor
    xJson::Value keystore_info;
    if (parse_keystore(target_kf, keystore_info) == false) {
        CONSOLE_ERROR("keystore parse error, check keystore file %s", target_kf.c_str());
        return -1;
    }

    std::string pri_key;
    std::string kdf_key;
    __compatibility_begin("try old empty_pw \" \" ");
    if (decrypt_keystore_by_password(DEPRECATED_OLD_DEFAULT_KEY, keystore_info, pri_key) == false) {
        // " " password not right.
    } else {
        std::cout << "It is recommended to set a password to protect the keystore file!" << std::endl;
        xassert(!pri_key.empty());
        // continue to set default.
        if (decrypt_get_kdf_key(DEPRECATED_OLD_DEFAULT_KEY, keystore_info, kdf_key) == false) {
            xassert(false);  // not possible.
            return -1;
        }
    }
    if (pri_key.empty()) {
        __compatibility_end();
        std::string pw;
        if (pw_path.empty()) {
            std::cout << "Please input password." << std::endl;
            pw = input_hiding();
        } else {
            auto result = get_password(keystore_type::account_key, password_type::file_path, pw_path);
            if (result.first == false) {
                return -1;
            } else {
                pw = result.second;
            }
        }
        if (decrypt_keystore_by_password(pw, keystore_info, pri_key) == false) {
            // password not right.
            out_str << "Wrong password, set default miner failed." << get_keystore_hint(keystore_info) << std::endl;
            return -1;
        }
        if (decrypt_get_kdf_key(pw, keystore_info, kdf_key) == false) {
            xassert(false);  // not possible.
            return -1;
        }

        __compatibility_begin("try old empty_pw \" \" ");
    }
    __compatibility_end();

    if (pri_key.empty() || kdf_key.empty()) {
        out_str << "decrypt private token failed" << std::endl;
        out_str << "Set miner key failed." << std::endl;
        return -1;
    }

    // todo ------------------↓ refactor

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
    if (set_prikey_to_daemon(pub_key, kdf_key, out_str, 0) != 0) {
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
    top::base::xvaccount_t _vaccount(to);
    if (_vaccount.is_eth_address())
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

void ApiMethod::estimategas(std::string & to, std::string & amount_d, std::string & note, std::string & tx_deposit_d, std::ostringstream & out_str) {
    std::ostringstream res;
    if (update_account(res) != 0) {
        return;
    }
    if (!check_account_address(to)) {
        cout << "Invalid transfer account address." << endl;
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
    api_method_imp_.estimategas(g_userinfo, from, to, amount, note, out_str);
    tackle_null_query(out_str);
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
    top::base::xvaccount_t _vaccount(g_userinfo.account);
    if (_vaccount.is_eth_address())
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
                top::base::xvaccount_t _vaccount(account);
                if (_vaccount.is_eth_address())
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
        if (!set_default_prikey(out_str)) {
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
        top::base::xvaccount_t _vaccount(target);
        if (_vaccount.is_eth_address())
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

void ApiMethod::getBlocksByHeight(std::string & target, std::string & height, std::ostringstream & out_str) {
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
    api_method_imp_.getBlocksByHeight(g_userinfo, g_userinfo.account, height, out_str);
    tackle_null_query(out_str);
}

void ApiMethod::chain_info(std::ostringstream & out_str) {
    api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
    api_method_imp_.getChainInfo(g_userinfo, out_str);
}

void ApiMethod::general_info(std::ostringstream & out_str) {
    api_method_imp_.make_private_key(g_userinfo.private_key, g_userinfo.account);
    api_method_imp_.getGeneralInfo(g_userinfo, out_str);
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
        top::base::xvaccount_t _vaccount(account);
        if (_vaccount.is_eth_address())
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
}

ApiMethod::~ApiMethod() {
}

std::string ApiMethod::get_keystore_hint(xJson::Value const & keystore_info) {
    std::string password_hint = keystore_info["hint"].asString();
    if (password_hint.empty()) {
        return "";
    } else {
        return "Hint: " + password_hint;
    }
}

std::string ApiMethod::input_hiding() {
    /// add `< /dev/tty` to work around to support handle password from stdin
    /// or it will error: `stty: standard input: Inappropriate ioctl for device`
    /// more infomation reference https://unix.stackexchange.com/questions/157852/echo-test-stty-echo-stty-standard-input-inappropriate-ioctl-for-device
    std::system("stty -echo < /dev/tty");
    std::string str;
    std::getline(std::cin, str, '\n');
    cin.clear();
    std::system("stty echo < /dev/tty");
    return str;
}

std::string ApiMethod::input_hiding_no_empty(std::string const & empty_msg) {
    auto input = input_hiding();
    if (input.empty()) {
        CONSOLE_ERROR(empty_msg);
        return input_hiding_no_empty(empty_msg);
    }
    return input;
}

std::string ApiMethod::input_no_hiding() {
    std::string str;
    std::getline(std::cin, str, '\n');
    cin.clear();
    return str;
}

std::string ApiMethod::input_same_pswd_twice() {
    std::string pw1 = input_hiding_no_empty("empty password not allowed!");

    CONSOLE_INFO("Please input password again");
    std::string pw2 = input_hiding();

    if (pw1 == pw2) {
        return pw1;
    } else {
        CONSOLE_INFO("Passwords are not the same.");
        CONSOLE_INFO("\nPlease input password:");
        return input_same_pswd_twice();
    }
}

std::string ApiMethod::input_pswd_hint() {
    CONSOLE_INFO("Please set a password hint! If don't, there will be no hint when you forget your password.");
    return input_no_hiding();
}

std::pair<bool, std::string> ApiMethod::get_password(keystore_type const & keys_type, password_type const & pswd_type, std::string const & file_path, bool is_reset_pw) {
    g_pw_hint = "";
    std::string get_pw = "";

    if (pswd_type == password_type::file_path) {
        std::ifstream pw_file(file_path, std::ios::in);
        if (!pw_file) {
            CONSOLE_ERROR(" Fail to open file: ", file_path);
            return std::make_pair(false, get_pw);
        }
        std::getline(pw_file, get_pw);  //? Without check format
    } else if (pswd_type == password_type::interactive) {
        if (is_reset_pw) {
            CONSOLE_INFO("Please set a new password. Pressing Ctrl+C can exit the command.");
        } else if (keys_type == keystore_type::account_key) {
            CONSOLE_INFO("Please set a password for the account keystore file.");
        } else {
            CONSOLE_INFO("Please set a password for the keystore file.");
        }
        get_pw = input_same_pswd_twice();

        g_pw_hint = input_pswd_hint();
    } else {
        xassert(false);
    }
    return std::make_pair(true, get_pw);
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

void ApiMethod::outAccountBalance(const std::string & account, std::ostringstream & out_str) {
    std::ostringstream as;
    auto tmp = g_userinfo.account;
    g_userinfo.account = account;
    std::string q_account(account);
    top::base::xvaccount_t _vaccount(q_account);
    if (_vaccount.is_eth_address())
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
