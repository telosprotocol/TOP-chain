//  xchain_command.cc
//  user commands
//
//  Created by Smaugx on 04/23/2020.
//  Copyright (c) 2017-2020 Telos Foundation & contributors
//

#include "xchaininit/xchain_command.h"

#if defined(XCXX20)
#    include "CLI/CLI.hpp"
#else
#    include "CLI11.hpp"
#endif
#include "db_tool/db_prune.h"
#include "db_tool/db_tool.h"
#include "xversion/version.h"
#include "xchaininit/xchain_command_http_client.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xtopcl/include/global_definition.h"
#include "xtopcl/include/topcl.h"
#include "xtopcl/include/xcrypto.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

using json = nlohmann::json;

namespace top {

static const uint32_t MAX_PASSWORD_SIZE = 1024;

ChainCommands::ChainCommands(elect::MultilayerNetworkInterfacePtr net_module, sync::xsync_face_t * sync) : net_module_{net_module}, sync_module(sync) {
    std::string datadir;
    top::config::xconfig_register_t::get_instance().get<std::string>("datadir", datadir);
    AddNetModuleCommands();
    AddSyncModuleCommands();
}

ChainCommands::ChainCommands(const std::string & datadir, elect::MultilayerNetworkInterfacePtr net_module, sync::xsync_face_t * sync) : net_module_{net_module}, sync_module(sync) {
    AddNetModuleCommands();
    AddSyncModuleCommands();
}

// cmdline: node isjoined; node peers; chain syncstatus; ...
bool ChainCommands::ProcessCommand(const std::string & cmdline, std::string & result) {
    std::string module_name;
    std::string cmd_name;
    XchainArguments args;
    try {
        top::base::LineParser line_split(cmdline.c_str(), ' ', cmdline.size());
        module_name.clear();
        cmd_name.clear();
        for (uint32_t i = 0; i < line_split.Count(); ++i) {
            if (strlen(line_split[i]) == 0) {
                continue;
            }

            if (module_name.empty())
                // the first sp is module name: node, chain
                module_name = line_split[i];
            else if (cmd_name.empty()) {
                // the second sp is cmd_name
                cmd_name = line_split[i];  // PeerCount, Joined, Account, uinfo
            } else {
                args.push_back(line_split[i]);
            }
        }
    } catch (const std::exception & e) {
        TOP_WARN("Error processing command: %s", e.what());
    }

    if (module_name.empty() || cmd_name.empty()) {
        result = "command:" + cmdline + " not support";
        return false;
    }

    std::transform(module_name.begin(), module_name.end(), module_name.begin(), ::tolower);
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::tolower);
    std::string module_cmd_name = module_name + "." + cmd_name;

    std::unique_lock<std::mutex> lock(map_commands_mutex_);
    auto it = map_commands_.find(module_cmd_name);
    if (it == map_commands_.end()) {
        result = "command:" + cmdline + " not support";
        return false;
    }
    (it->second)(args, "", result);  // call command procedure
    return true;
}

void ChainCommands::AddNetModuleCommands() try {
    std::string module_name = "node";

    /*
    AddCommand(module_name, "help", [this](const XchainArguments& args, const std::string& cmdline, std::string& result){
        // call net help
        result = net_module_->HelpInfo();
        //std::cout << result << std::endl;
    });
    */

    AddCommand(module_name, "isjoined", [this](const XchainArguments & args, const std::string & cmdline, std::string & result) {
        auto ret = net_module_->Joined();
        if (ret) {
            result = "YES";
        } else {
            result = "NO, " + global_node_id + " account has not joined network.";
        }
        // std::cout << result << std::endl;
    });

#ifdef DEBUG

    AddCommand(module_name, "broadcast", [this](const XchainArguments & args, const std::string & cmdline, std::string & result) {
        uint32_t msg_size = 300;
        if (args.size() >= 1) {
            msg_size = check_cast<uint32_t, const char *>(args[0].c_str());
        }
        uint32_t count = 100;
        if (args.size() >= 2) {
            count = check_cast<uint32_t, const char *>(args[1].c_str());
        }
        //auto sus = net_module_->Broadcast(msg_size, count);
        //result = std::to_string(sus);
        result = net_module_->Broadcast(msg_size, count);
        // std::cout << result << std::endl;
    });
#endif

    AddCommand(module_name, "peers", [this](const XchainArguments & args, const std::string & cmdline, std::string & result) {
        result = net_module_->Peers();
        // std::cout << result << std::endl;
    });

    AddCommand(module_name, "nodep2paddr", [this](const XchainArguments & args, const std::string & cmdline, std::string & result) {
        result = net_module_->P2pAddr();
        // std::cout << result << std::endl;
    });

} catch (std::exception & e) {
    std::cout << "catch error: (" << e.what() << ") check_cast failed" << std::endl;
}

void ChainCommands::AddSyncModuleCommands() {
    std::string module_name = "chain";
    try {
        /*
        AddCommand("sync", "help", [this](const XchainArguments& args, const std::string& cmdline, std::string& result) {
            result = sync_module->help();
            //std::cout << result << std::endl;
        });
        */

        AddCommand(module_name, "syncstatus", [this](const XchainArguments & args, const std::string & cmdline, std::string & result) {
            result = sync_module->status();
            // std::cout << result << std::endl;
        });
        AddCommand(module_name, "autoprunedata", [this](const XchainArguments & args, const std::string & cmdline, std::string & result) {
            if (args.size() < 1)
                return;
            std::string prune = args[0];
            xinfo("enter set prune: %s", prune.c_str());
            result = sync_module->auto_prune_data(prune);
            // std::cout << result << std::endl;
        });
    } catch (std::exception & e) {
        std::cout << "catch error: (" << e.what() << ") check_cast failed" << std::endl;
    }
}

void ChainCommands::AddCommand(const std::string & module_name, const std::string & cmd_name, XchainCommandProc cmd_proc) {
    assert(cmd_proc);

    std::unique_lock<std::mutex> lock(map_commands_mutex_);

    // speciall for topcl module, cmd_name is always  topcl.
    auto module_cmd_name = module_name + "." + cmd_name;
    auto it = map_commands_.find(module_cmd_name);
    if (it != map_commands_.end()) {
        TOP_WARN("command(%s) exist and ignore new one", module_cmd_name.c_str());
        return;
    }

    map_commands_[module_cmd_name] = cmd_proc;
    TOP_INFO("add command(%s)", module_cmd_name.c_str());
}

// begin handle chain command

// get current dir
std::string get_working_path() {
    char temp[256];
    return (getcwd(temp, sizeof(temp)) ? std::string(temp) : std::string(""));
}

int db_backup(const std::string & from, const std::string & to) {
    std::string errormsg{""};
    // backup db directory
    std::string from_db_dir = from + DB_PATH;
    std::string output_file_db = to + DB_PATH;

    if (!isDirExist(from_db_dir)) {
        printf("Backup failed\nError: the %s  does not exist.\n", from_db_dir.c_str());
        return -1;
    }

    makedir(output_file_db.c_str());

    if (0 != backup(from_db_dir, output_file_db, errormsg)) {
        fprintf(stderr, "Backup db err: %s", errormsg.c_str());
        return -1;
    } else {
        printf("Backup db successfully.\n");
        return 0;
    }
}

int db_restore(const std::string & from, const std::string & to, const int backupid) {
    std::string errormsg{""};
    if (from.empty()) {
        // if not specify the refuse
        printf("please input the correct backup directory\n");
        return -1;
    }

    std::string from_db_dir = from + DB_PATH;

    // user specify the data recover target directory
    std::string target = to;

    if (!isDirExist(from_db_dir) || !isDirExist(to)) {
        printf("Restore failed\nError: the %s or %s does not exist.\n", from_db_dir.c_str(), to.c_str());
        return -1;
    }

    // parameter verify
    // to kown if pid file is exist
    auto pidFile = to + "/.xnode.pid";
    if (isFileExist(pidFile)) {
        printf("please make sure there is no any topio proccess running on current directory.\n");
        return -1;
    }
    // create two restore user directory, one for db, the other for pdb
    multiplatform_mkdir(target.c_str());
    auto db_target = target + DB_PATH;
    // check if the target directory specified by user is exist
    if (!IsDirEmpty(db_target.c_str())) {
        printf("Restore failed: %s\nError: The target dir for restore is not empty, please input a empty one.\n", db_target.c_str());
        return -1;
    }

    multiplatform_mkdir(db_target.c_str());
    // printf("resetore from:%s to %s\n", from_db_dir.c_str(),db_target.c_str());
    if (-1 == restore(backupid, from_db_dir, db_target, errormsg)) {
        fprintf(stderr, "Restore failed error: %s.\n", errormsg.c_str());
        return -1;
    }
    printf("Database restore operating successfully.\n");
    return 0;
}

// use to check if a directory exists
bool isDirExist(std::string dirPath) {
    struct stat fileInfo;
    stat(dirPath.c_str(), &fileInfo);
    if (S_ISDIR(fileInfo.st_mode)) {
        return true;
    } else {
        return false;
    }
}

// use to check if a file exists
bool isFileExist(const std::string & name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool IsDirEmpty(const char * dirname) {
    int n = 0;
    dirent * d;
    DIR * dir = opendir(dirname);
    if (dir == NULL)
        return true;
    while ((d = readdir(dir)) != NULL) {
        if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) {
            continue;
        } else {
            n++;
        }

        if (n > 0) {
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
    return true;
}

int multiplatform_mkdir(const std::string & path) {
    // Skip if directory is not exist
    if (isDirExist(path)) {
        return 0;
    }

#if defined(__LINUX_PLATFORM__) || defined(__MAC_PLATFORM__)
    return makedir(path.c_str());
#elif defined(__WIN_PLATFORM__)
    std::string tpath = path;
    std::string from = "/", to = "\\";
    size_t pos;
    while ((pos = tpath.find(from)) != std::string::npos) {
        tpath.replace(pos, 1, to);
    }
    _mkdir(tpath.c_str());
#endif
    return 0;
}

int parse_execute_command(const char * config_file_extra, int argc, char * argv[]) {
    std::ifstream config_in(config_file_extra);
    if (!config_in.is_open()) {
        std::cout << "open config file failed!" << std::endl;
        return -1;
    }

    json config_extra_json;
    try {
        config_in >> config_extra_json;
    } catch (json::parse_error & e) {
        std::cout << "parse config file failed!" << std::endl;
        config_in.close();
        return -1;
    }
    config_in.close();

    CLI::App app{"TOPIO is a tool that integrates command line client , wallet and node program based on TOP Network."};
    app.ignore_case();
    // require at least one command
    // app.require_subcommand(1);
    auto version_func = [&]() -> void {
        print_version();
        return;
    };
    app.add_flag_callback("-v,--version", version_func, "topio version");

    using xChainSDK::ApiMethod;
    top::xtopcl::xtopcl topcl;
    // set default datadir for topcl
    topcl.api.set_keystore_path(config_extra_json["datadir"].get<std::string>());
    topcl.api.change_trans_mode(true);
    std::ostringstream out_str;

    /*
     * wallet
     */
    auto wallet_app = app.add_subcommand("wallet", "Create and manage accounts and public-private key pairs.");
    // create new account
    auto createAccount = wallet_app->add_subcommand("createAccount", "Create an account.");
    std::string pw_path;
    createAccount->callback(std::bind(&ApiMethod::create_account, &topcl.api, std::ref(pw_path), std::ref(out_str)));
    createAccount->add_option("-f,--pwd_file_path", pw_path, "The path of file that contains password string.");

    // create new key
    auto createKey = wallet_app->add_subcommand("createKey", "Create a worker key for specific account.");
    std::string createKey_pw_path;
    std::string owner_account = g_userinfo.account;
    createKey->callback(std::bind(&ApiMethod::create_key, &topcl.api, std::ref(owner_account), std::ref(createKey_pw_path), std::ref(out_str)));
    createKey->add_option(
        "account_addr", owner_account, "The account address new key belong to.If you do not add this parameter, a worker key will be created for your default account.");
    createKey->add_option("-f,--pwd_file_path", createKey_pw_path, "The path of file that contains password string.");

    // list all accounts
    auto listAccounts_app = wallet_app->add_subcommand("listAccounts", "List all accounts in wallet.");
    listAccounts_app->callback(std::bind(&ApiMethod::list_accounts, &topcl.api, std::ref(out_str)));

    // set default account
    auto setDefaultAccount_app = wallet_app->add_subcommand("setDefaultAccount", "Set a default account for sending transactions.");
    std::string setDefaultAccount_account;
    std::string setDefaultAccount_pw_path;
    setDefaultAccount_app->callback(
        std::bind(&ApiMethod::set_default_account, &topcl.api, std::ref(setDefaultAccount_account), std::ref(setDefaultAccount_pw_path), std::ref(out_str)));
    setDefaultAccount_app->add_option("account_addr", setDefaultAccount_account, "Account address.")->required();
    setDefaultAccount_app->add_option("-f,--pwd_file_path", setDefaultAccount_pw_path, "The path of file that contains password string.");

    // reset keystore password
    auto resetPw_app = wallet_app->add_subcommand("resetKeystorePwd", "Reset the password for a keystore file.");
    std::string resetPw_public_key;
    resetPw_app->callback(std::bind(&ApiMethod::reset_keystore_password, &topcl.api, std::ref(resetPw_public_key), std::ref(out_str)));
    resetPw_app->add_option("public_key", resetPw_public_key, "The public key.")->required();

    // import account
    std::string importAccount_pw_path;
    auto importAccount_app = wallet_app->add_subcommand("importAccount", "Import private key into wallet.");
    importAccount_app->callback(std::bind(&ApiMethod::import_account, &topcl.api, std::ref(importAccount_pw_path), std::ref(out_str)));
    importAccount_app->add_option("-f,--pwd_file_path", importAccount_pw_path, "The path of file that contains password string.");

    // export account
    auto exportAccount_app = wallet_app->add_subcommand("exportAccount", "Export private key and keystore json file.");
    exportAccount_app->callback(std::bind(&ApiMethod::export_account, &topcl.api, std::ref(owner_account), std::ref(out_str)));
    exportAccount_app->add_option("account_addr", owner_account, "The account address")->required();
    // todo add option use -f --pwd_file_path to avoid interactive password asked.

    /*
     * mining
     */
    auto mining_app = app.add_subcommand("mining", "Register as a miner and manage a miner.");
    mining_app->require_subcommand(1);

    // register miner
    auto registerMiner_app = mining_app->add_subcommand("registerMiner", "Register to the mining pool. ");
    std::string registerMiner_amount("0");
    std::string miner_type;
    std::string miner_name;
    registerMiner_app->add_option("top_num", registerMiner_amount, "Miner register deposit,unit is TOP.")->required();
    registerMiner_app->add_option("miner_type", miner_type, "Miner type: edge, validator, advance, exchange.")->required();
    registerMiner_app->add_option("miner_name", miner_name, "Miner nickname. 4-16 characters, supporting letters, numbers or underscores.")->required();
    uint32_t dividend_ratio = 0;
    std::string node_sign_key;
    registerMiner_app->add_option("-d,--dividend_ratio", dividend_ratio, "For advance miner to set dividend ratio for voters who support you. Value∈[0,100].Default is 0.")
        ->check(CLI::Range((uint16_t)0, (uint16_t)100))
        ->default_val(0);
    registerMiner_app->add_option(
        "-m,--miner_key", node_sign_key, "Use your default account's owner key or worker key for mining. The default is the owner key. Please pass in the public key here.");
    registerMiner_app->callback(std::bind(&ApiMethod::register_node,
                                          &topcl.api,
                                          std::ref(registerMiner_amount),
                                          std::ref(miner_type),
                                          std::ref(miner_name),
                                          std::ref(dividend_ratio),
                                          std::ref(node_sign_key),
                                          std::ref(out_str)));

    auto setminerkey = mining_app->add_subcommand("setMinerKey", "Set the key for mining.");
    std::string miner_default_pub_key;
    std::string miner_pw_path;
    setminerkey->add_option("publickey", miner_default_pub_key, "Public key")->required();
    setminerkey->add_option("-f,--pwd_file_path", miner_pw_path, "The path of file that contains password string");
    setminerkey->callback(std::bind(&ApiMethod::set_default_miner, &topcl.api, std::ref(miner_default_pub_key), std::ref(miner_pw_path), std::ref(out_str)));

    // query miner info
    auto queryMinerInfo_app = mining_app->add_subcommand("getMinerInfo", "Query miner information");
    std::string queryMinerInfo_account;
    queryMinerInfo_app->add_option("account_addr", queryMinerInfo_account, "Account address, if you do not add this parameters, your default miner account will be queried.");
    queryMinerInfo_app->callback(std::bind(&ApiMethod::query_miner_info, &topcl.api, std::ref(queryMinerInfo_account), std::ref(out_str)));

    // query miner reward
    auto queryMinerReward_app = mining_app->add_subcommand("queryMinerReward", "Query specific miner rewards.");
    std::string queryMinerReward_account;
    queryMinerReward_app->add_option("account_addr", queryMinerReward_account, "Miner account address. If you do not add this parameter, your default account will be queried.");
    queryMinerReward_app->callback(std::bind(&ApiMethod::query_miner_reward, &topcl.api, std::ref(queryMinerReward_account), std::ref(out_str)));

    // claim miner reward
    auto claimMinerReward_app = mining_app->add_subcommand("claimMinerReward", "Claim the miner rewards.");
    claimMinerReward_app->callback(std::bind(&ApiMethod::claim_miner_reward, &topcl.api, std::ref(out_str)));

    // update miner info
    auto updateMinerInfo_app = mining_app->add_subcommand("updateMinerInfo", "Update miner type, miner name, miner deposit, dividend ratio and miner key at the same time.");
    std::string updateMinerInfo_type;
    std::string updateMinerInfo_name;
    uint32_t updateMinerInfo_deposit_type = 0;
    std::string updateMinerInfo_deposit("0");
    uint32_t updateMinerInfo_rate = 0;
    std::string updateMinerInfo_sign_key;
    updateMinerInfo_app->add_option("miner_type", updateMinerInfo_type, "New miner type: edge, validator, advance, exchange.")->required();
    updateMinerInfo_app->add_option("miner_name", updateMinerInfo_name, "New miner name. 4-16 characters, supporting letters, numbers or underscores.")->required();
    updateMinerInfo_app->add_option("increase_or_decrease", updateMinerInfo_deposit_type, "1--Increase miner deposit. 2--Decrease miner deposit.")->required();
    updateMinerInfo_app->add_option("top_num", updateMinerInfo_deposit, "Amounts of miner deposit will be increased or decreased，unit is TOP")->required();
    updateMinerInfo_app->add_option("dividend_ratio", updateMinerInfo_rate, "Updated dividend ratio.")->required()->check(CLI::Range((uint16_t)0, (uint16_t)100));
    updateMinerInfo_app->add_option("miner_key", updateMinerInfo_sign_key, "Updated miner sign key.")->required();
    updateMinerInfo_app->callback(std::bind(&ApiMethod::update_miner_info,
                                            &topcl.api,
                                            std::ref(updateMinerInfo_type),
                                            std::ref(updateMinerInfo_name),
                                            std::ref(updateMinerInfo_deposit_type),
                                            std::ref(updateMinerInfo_deposit),
                                            std::ref(updateMinerInfo_rate),
                                            std::ref(updateMinerInfo_sign_key),
                                            std::ref(out_str)));

    // set dividend ratio
    auto setDividendRatio_app = mining_app->add_subcommand("setDividendRatio", "Set devidend ratio for voters who support you.");
    uint32_t setDividendRatio_ratio = 0;
    std::string setDividendRatio_deposit("0");
    setDividendRatio_app->add_option("percent", setDividendRatio_ratio, "Dividend ratio(≥0，≤100).")->required()->check(CLI::Range((uint16_t)0, (uint16_t)100));
    setDividendRatio_app->add_option("-t,--tx_deposit", setDividendRatio_deposit, "Transaction deposit, a minimum of 0.1 TOP.");
    setDividendRatio_app->callback(std::bind(&ApiMethod::set_dividend_ratio, &topcl.api, std::ref(setDividendRatio_ratio), std::ref(setDividendRatio_deposit), std::ref(out_str)));

    // set miner name
    auto setMinerName_app = mining_app->add_subcommand("setMinerName", "Set miner name.");
    std::string setMinerName_name;
    setMinerName_app->add_option("miner_name", setMinerName_name, "New miner nickname.")->required();
    setMinerName_app->callback(std::bind(&ApiMethod::set_miner_name, &topcl.api, std::ref(setMinerName_name), std::ref(out_str)));

    // add deposit
    auto addDeposit_app = mining_app->add_subcommand("addDeposit", "Pledge more deposit for the miner. ");
    std::string addDeposit_deposit("0");
    addDeposit_app->add_option("top_num", addDeposit_deposit, "Amounts of miner deposit will be increased, unit is TOP.")->required();
    addDeposit_app->callback(std::bind(&ApiMethod::add_deposit, &topcl.api, std::ref(addDeposit_deposit), std::ref(out_str)));

    // reduce deposit
    auto reduceDeposit_app = mining_app->add_subcommand("reduceDeposit", "Reduce the deposit of the miner. ");
    std::string reduceDeposit_deposit("0");
    reduceDeposit_app->add_option("top_num", reduceDeposit_deposit, "Amounts of miner deposit will be decreased, unit is TOP.")->required();
    reduceDeposit_app->callback(std::bind(&ApiMethod::reduce_deposit, &topcl.api, std::ref(reduceDeposit_deposit), std::ref(out_str)));

    // change miner type
    auto changeMinerType_app = mining_app->add_subcommand("changeMinerType", "Update miner type.");
    std::string changeMinerType_type;
    changeMinerType_app->add_option("miner_type", changeMinerType_type, "New miner type: edge, validator, advance, exchange.")->required();
    changeMinerType_app->callback(std::bind(&ApiMethod::change_miner_type, &topcl.api, std::ref(changeMinerType_type), std::ref(out_str)));

    // terminate
    auto terminate_app = mining_app->add_subcommand("terminate", "Stop mining and leave the network.");
    terminate_app->callback(std::bind(&ApiMethod::unregister_node, &topcl.api, std::ref(out_str)));

    // withdraw deposit
    auto withdrawDeposit_app = mining_app->add_subcommand("withdrawDeposit", "Redeem miner deposit when miner register.");
    withdrawDeposit_app->callback(std::bind(&ApiMethod::withdraw_deposit, &topcl.api, std::ref(out_str)));

    /*
     * node
     */
    auto node = app.add_subcommand("node", "Start and manage node(s).");
    node->require_subcommand(1);

    // just put here for help info(particular implementation in topio main(xmain.cpp))
    // startnode
    auto startnode = node->add_subcommand("startNode", "Launch the node.");
    {
        std::string const description{"start topio as single_process(default master + worker mode)."};
        startnode->add_flag(std::string{"-S,--single_process"}, description);
    }
    startnode->add_option("--admin_http_addr", "admin http server addr(default: 127.0.0.1).");
    startnode->add_option("--admin_http_port", "admin http server port(default: 8000).");
    startnode->add_option("--bootnodes", "Comma separated endpoints(ip:port) for P2P  discovery bootstrap.");
    startnode->add_option("--net_port", "p2p network listening port (default: 9000).");
    startnode->add_option("--db_compress", "set db compress option:(default:default_compress, high_compress, bottom_compress, no_compress).");
    // startnode->add_option("-c,--config", "start with config file.");
    {
        std::string const description{"start as no daemon."};
        startnode->add_flag("--nodaemon", description);
    }
    startnode->callback([&]() {});
    // stopnode
    auto stopnode = node->add_subcommand("stopNode", "stop topio.");
    stopnode->callback([&]() {});
    // reloadnode
    auto reloadnode = node->add_subcommand("reloadNode", "reload topio.");
    reloadnode->callback([&]() {});
    auto safeboxnode = node->add_subcommand("safebox", "safebox process, manager accounts and token.")->group("");
    safeboxnode->add_option("--http_addr", "safebox http server addr(default: 127.0.0.1).");
    safeboxnode->add_option("--http_port", "safebox http server port(default: 7000).");
    safeboxnode->callback([&]() {});

    auto node_call = [&](const std::string & admin_http_addr, uint16_t admin_http_port) -> int {
        std::pair<std::string, uint16_t> admin_http_pair = std::make_pair(admin_http_addr, admin_http_port);
        std::string cmd;
        for (int i = 1; i < argc; ++i) {
            if (strlen(argv[i]) == 0) {
                continue;
            }
            cmd += argv[i];
            cmd += " ";
        }
        std::string result;
        handle_node_command(admin_http_pair, cmd, result);
        std::cout << result << std::endl;
        return 0;
    };

    std::string admin_http_addr("127.0.0.1");
    uint16_t admin_http_port(8000);
    // isjoined
    auto isjoined = node->add_subcommand("isJoined", "Query whether the node has joined the network.");
    isjoined->add_option("--admin_http_addr", admin_http_addr, "admin http server addr(default: 127.0.0.1).");
    isjoined->add_option("--admin_http_port", admin_http_port, "admin http server port(default: 8000).");
    isjoined->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // peers
    auto peers = node->add_subcommand("peers", "Get the peers.");
    peers->add_option("--admin_http_addr", admin_http_addr, "admin http server addr(default: 127.0.0.1).");
    peers->add_option("--admin_http_port", admin_http_port, "admin http server port(default: 8000).");
    peers->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // netID
    auto netid = node->add_subcommand("netID", "Print network IDs which the node joined in.")->group("");
    netid->add_option("--admin_http_addr", admin_http_addr, "admin http server addr(default: 127.0.0.1).");
    netid->add_option("--admin_http_port", admin_http_port, "admin http server port(default: 8000).");
    netid->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // peercount
    auto peercount = node->add_subcommand("peerCount", "Print root routing table size.")->group("");
    peercount->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // broadcast
    auto broadcast = node->add_subcommand("broadcast", "Broadcast a message.")->group("");
    uint32_t bd_msg_size;
    uint32_t bd_count;
    broadcast->add_option("msg_size", bd_msg_size, "Broadcast msg size.")->required();
    broadcast->add_option("count", bd_count, "Broadcast msg count.")->required();
    broadcast->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // allpeers
    auto allpeers = node->add_subcommand("allpeers", "Print All peers.")->group("");
    allpeers->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // all nodes
    auto allnodes = node->add_subcommand("allnodes", "Print All nodes.")->group("");
    allnodes->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // gid
    auto gid = node->add_subcommand("gid", "Print Root P2p id.")->group("");
    gid->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // accountaddr
    auto accountaddr = node->add_subcommand("accountaddr", "Print accountaddr.")->group("");
    accountaddr->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // osInfo
    auto osinfo = node->add_subcommand("osInfo", "Print OS information.")->group("");
    osinfo->add_option("--admin_http_addr", admin_http_addr, "admin http server addr(default: 127.0.0.1).");
    osinfo->add_option("--admin_http_port", admin_http_port, "admin http server port(default: 8000).");
    osinfo->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));
    // nodeP2PAddr
    auto nodep2paddr = node->add_subcommand("nodeP2PAddr", "Print the nodes's P2P ID with IP:port.");
    nodep2paddr->add_option("--admin_http_addr", admin_http_addr, "admin http server addr(default: 127.0.0.1).");
    nodep2paddr->add_option("--admin_http_port", admin_http_port, "admin http server port(default: 8000).");
    nodep2paddr->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));

#if !defined(XBUILD_CONSORTIUM)
    /*
     * staking
     */
    auto staking_app = app.add_subcommand("staking", "Stake fund to vote miners and manage vote tickets.");

    // stake fund
    auto stakeFund_app = staking_app->add_subcommand("stakeFund", "Stake TOP tokens to receive vote tickets.");
    uint64_t stakeFund_amount = 0;
    uint16_t stakeFund_lock_duration = 0;
    stakeFund_app->add_option("vote_amount", stakeFund_amount, "Amount of votes to be exchanged.")->required();
    stakeFund_app->add_option("lock_duration", stakeFund_lock_duration, "TOP token lock duration,minimum 30 days.")->required();
    stakeFund_app->callback(std::bind(&ApiMethod::stake_fund, &topcl.api, std::ref(stakeFund_amount), std::ref(stakeFund_lock_duration), std::ref(out_str)));

    // stake withdraw fund
    auto stake_withdrawFund_app = staking_app->add_subcommand("withdrawFund", "Withdraw pledged TOP tokens and tickets.");
    uint64_t stake_withdrawFund_amount = 0;
    std::string stake_withdrawFund_deposit("0");
    stake_withdrawFund_app->add_option("votes_num", stake_withdrawFund_amount, "Votes amount, unlock the corresponding TOP token.")->required();
    stake_withdrawFund_app->add_option("-t,--tx_deposit", stake_withdrawFund_deposit, "Transaction deposit, a minimum of 0.1 TOP.");
    stake_withdrawFund_app->callback(
        std::bind(&ApiMethod::stake_withdraw_fund, &topcl.api, std::ref(stake_withdrawFund_amount), std::ref(stake_withdrawFund_deposit), std::ref(out_str)));

    // vote miner
    auto voteMiner_app = staking_app->add_subcommand("voteMiner", "Assign tickets to miners.");
    std::vector<std::pair<std::string, int64_t>> vote_infos;
    voteMiner_app
        ->add_option("miner_and_votes",
                     vote_infos,
                     "Miner account address(es) and votes. For example, if you want to vote 2 miners, you can execute command as: topio staking voteMiner miner_addr1 80000 "
                     "miner_addr2 10000")
        ->required();
    voteMiner_app->callback(std::bind(&ApiMethod::vote_miner, &topcl.api, std::ref(vote_infos), std::ref(out_str)));

    // withdraw votes
    auto withdrawVotes_app = staking_app->add_subcommand("withdrawVotes", "Withdraw vote tickets from miners.");
    std::vector<std::pair<std::string, int64_t>> withdrawVotes_vote_infos;
    withdrawVotes_app
        ->add_option("miner_and_votes",
                     withdrawVotes_vote_infos,
                     "Miner account address(es) and votes. For example, if you want to withdraw votes on 2 miners, you can execute command as: topio staking withdrawVotes "
                     "miner_addr1 80000 miner_addr2 10000")
        ->required();
    withdrawVotes_app->callback(std::bind(&ApiMethod::withdraw_votes, &topcl.api, std::ref(withdrawVotes_vote_infos), std::ref(out_str)));

    // query votes
    auto queryVotes_app = staking_app->add_subcommand("queryVotes", "Query allocation information of vote tickets.");
    std::string queryVotes_account;
    queryVotes_app->add_option("account_addr", queryVotes_account, "Account address. If you do not add this parameter, your default account will be queried.");
    queryVotes_app->callback(std::bind(&ApiMethod::query_votes, &topcl.api, std::ref(queryVotes_account), std::ref(out_str)));

    // query reward
    auto queryReward_app = staking_app->add_subcommand("queryReward", "Query reward amount.");
    std::string queryReward_account;
    queryReward_app->add_option("account_addr", queryReward_account, "Account address. If you do not add this parameters, your default miner account will be queried.");
    queryReward_app->callback(std::bind(&ApiMethod::query_reward, &topcl.api, std::ref(queryReward_account), std::ref(out_str)));

    // claim reward
    auto claimReward_app = staking_app->add_subcommand("claimReward", "Claim reward.");
    claimReward_app->callback(std::bind(&ApiMethod::claim_reward, &topcl.api, std::ref(out_str)));
#endif

    /*
     * transfer
     */
    std::string to;
    std::string amount("0");
    std::string note;
    std::string tx_deposit("0");
    auto transfer = app.add_subcommand("transfer", "Send TOP token to an account.");
    transfer->callback(std::bind(&ApiMethod::transfer1, &topcl.api, std::ref(to), std::ref(amount), std::ref(note), std::ref(tx_deposit), std::ref(out_str)));
    transfer->add_option("account_addr", to, "The receipt account address.")->required();
    transfer->add_option("top_num", amount, "Locked TOP token amount.")->required();
    transfer->add_option("note", note, "The note for the transfer,characters of any type, not exceeding 128 in length.");
    transfer->add_option("-t,--tx_deposit", tx_deposit, "Transaction deposit,a minimum of 0.1 TOP.");

    /*
     * estimategas
     */
    auto estimategas = app.add_subcommand("estimategas", "estimate transaction gas.");
    estimategas->callback(std::bind(&ApiMethod::estimategas, &topcl.api, std::ref(to), std::ref(amount), std::ref(note), std::ref(tx_deposit), std::ref(out_str)));
    estimategas->add_option("account_addr", to, "The receipt account address.")->required();
    estimategas->add_option("top_num", amount, "Locked TOP token amount.")->required();
    estimategas->add_option("note", note, "The note for the transfer,characters of any type, not exceeding 128 in length.");
    estimategas->add_option("-t,--tx_deposit", tx_deposit, "Transaction deposit,a minimum of 0.1 TOP.");

    /*
     * querytx
     */
    auto querytx_app = app.add_subcommand("querytx", "Get detail information of a transaction.");
    std::string querytx_account;
    std::string hash;
    querytx_app->callback(std::bind(&ApiMethod::query_tx, &topcl.api, std::ref(querytx_account), std::ref(hash), std::ref(out_str)));
    querytx_app->add_option("hash", hash, "Transaction hash.")->required();
    querytx_app->add_option(
        "account_addr", querytx_account, "The sender or reciever account address. If you do not add this parameter, your default account's transaction will be queried.");

    /*
     * chain
     */
    auto chain_app = app.add_subcommand("chain", "Interact with chain, including block, account, contract ,etc.");

    // query account
    auto queryAccount_app = chain_app->add_subcommand("queryAccount", "Query account information for a specific account.");
    std::string queryAccount_account;
    queryAccount_app->add_option("account_addr", queryAccount_account, "Account address，if you do not add this parameters, your default account will be queried.");
    queryAccount_app->callback(std::bind(&ApiMethod::query_account, &topcl.api, std::ref(queryAccount_account), std::ref(out_str)));

    // query block
    auto queryBlock_app = chain_app->add_subcommand("queryBlock", "Query information of a block.");
    std::string queryBlock_account;
    std::string queryBlock_height;
    queryBlock_app->add_option("height", queryBlock_height, "Integer of a block number, or the String \"latest\".")->required();
    queryBlock_app->add_option(
        "account_addr", queryBlock_account, "Account address to query its block block information. if you do not add this parameters, your default account will be queried.");
    queryBlock_app->callback(std::bind(&ApiMethod::query_block, &topcl.api, std::ref(queryBlock_account), std::ref(queryBlock_height), std::ref(out_str)));
#ifdef DEBUG
    // query blocks by height
    auto queryBlocksByHeight_app = chain_app->add_subcommand("queryBlocksByHeight", "Query information of blocks By Height.");
    std::string queryBlocksByHeight_account;
    std::string queryBlocksByHeight_height;
    queryBlocksByHeight_app->add_option("height", queryBlocksByHeight_height, "Integer of a block number, or the String \"latest\".")->required();
    queryBlocksByHeight_app->add_option("account_addr",
                                      queryBlocksByHeight_account,
                                      "Account address to query its block block information. if you do not add this parameters, your default account will be queried.");
    queryBlocksByHeight_app->callback(
        std::bind(&ApiMethod::getBlocksByHeight, &topcl.api, std::ref(queryBlocksByHeight_account), std::ref(queryBlocksByHeight_height), std::ref(out_str)));
 #endif
    // query General Info
    auto generalInfo_app = chain_app->add_subcommand("generalInfo", "Get General informaion.");
    generalInfo_app->callback(std::bind(&ApiMethod::general_info, &topcl.api, std::ref(out_str)));
    // query chain info
    auto chainInfo_app = chain_app->add_subcommand("chainInfo", "Get chain informaion.");
    chainInfo_app->callback(std::bind(&ApiMethod::chain_info, &topcl.api, std::ref(out_str)));

/* disable deploy & run contract temporarily for 6.15 version
    // deploy contract
    auto deployContract_app = chain_app->add_subcommand("deployContract", "Create a contract account and deploy a code to the contract.");
    uint64_t deployContract_gas_limit = 0;
    std::string deployContract_amount("0");
    std::string deployContract_path;
    std::string deployContract_tx_deposit("0");
    deployContract_app->add_option("gas_limit", deployContract_gas_limit, "Upper limit of gas fees that the contract is willing to pay for the sender of the transaction.")
        ->required();
    deployContract_app->add_option("top_num", deployContract_amount, "The TOP token amounts transferred to the contract account.The unit is TOP.")->required();
    deployContract_app->add_option("code_path", deployContract_path, "Contract code file path.")->required();
    deployContract_app->add_option("-t,--tx_deposit", deployContract_tx_deposit, "Transaction deposit,the default is 0.1TOP.");
    deployContract_app->callback(std::bind(&ApiMethod::deploy_contract,
                                           &topcl.api,
                                           std::ref(deployContract_gas_limit),
                                           std::ref(deployContract_amount),
                                           std::ref(deployContract_path),
                                           std::ref(deployContract_tx_deposit),
                                           std::ref(out_str)));

    // call contract
    auto callContract_app = chain_app->add_subcommand("callContract", "Send a transaction to a contract.");
    std::string callContract_amount("0");
    std::string callContract_addr;
    std::string callContract_func;
    std::string callContract_params;
    std::string callContract_tx_deposit("0");
    callContract_app->add_option("contract_addr", callContract_addr, "Contract account address, beginning with the symbol \"T30000\".")->required();
    callContract_app->add_option("contract_func", callContract_func, "The name of the contract function.")->required();
    callContract_app->add_option("top_num", callContract_amount, "The TOP token amounts transferred to the application contract account.The unit is TOP.");
    callContract_app->add_option(
        "func_params",
        callContract_params,
        "Parameter type:1--Uint64;2--String;3–Bool. If you need to add more than one parameter,  use '|'  to separate them. For example: '1,3243|2,abcd|3,true'.");
    callContract_app->add_option("-t,--tx_deposit", callContract_tx_deposit, "Transaction deposit,the default is 0.1TOP.");
    callContract_app->callback(std::bind(&ApiMethod::call_contract,
                                         &topcl.api,
                                         std::ref(callContract_amount),
                                         std::ref(callContract_addr),
                                         std::ref(callContract_func),
                                         std::ref(callContract_params),
                                         std::ref(callContract_tx_deposit),
                                         std::ref(out_str)));
*/
    // syncStatus
    auto syncstatus_app = chain_app->add_subcommand("syncStatus", "Get block sync status of the node.");
    syncstatus_app->callback(std::bind(node_call, std::ref(admin_http_addr), std::ref(admin_http_port)));

    auto block_prune = [&](std::string& prune_enable, std::ostringstream& out_str, const std::string & admin_http_addr, uint16_t admin_http_port) -> int {
        topcl.api.block_prune(prune_enable, out_str);
        std::pair<std::string, uint16_t> admin_http_pair = std::make_pair(admin_http_addr, admin_http_port);
        std::string cmd;
        for (int i = 1; i < argc; ++i) {
            if (strlen(argv[i]) == 0) {
                continue;
            }
            cmd += argv[i];
            cmd += " ";
        }
        std::string result;
        handle_node_command(admin_http_pair, cmd, result);
        std::cout << result << std::endl;
        return 0;
    };

    std::string prune_enable("off");
    auto block_prune_app = chain_app->add_subcommand("autoPruneData", "Set auto prune data.");
    block_prune_app->callback(std::bind(block_prune, std::ref(prune_enable), std::ref(out_str),
        std::ref(admin_http_addr), std::ref(admin_http_port)));
    block_prune_app->add_option("on|off", prune_enable, "auto prune data on or off.")->required();
   
#if !defined(XBUILD_CONSORTIUM) 
    /*
    * resource
    */ 
    auto resource_app = app.add_subcommand("resource", "Manage the resource of account.");

    // stake for gas
    auto stakeForGas_app = resource_app->add_subcommand("stakeForGas", "Stake TOP tokens to receive free gas.");
    std::string stakeForGas_amount("0");
    stakeForGas_app->add_option("top_num", stakeForGas_amount, "Amounts of deposit for gas will be withdrawed, unit is TOP.")->required();
    stakeForGas_app->callback(std::bind(&ApiMethod::stake_for_gas, &topcl.api, std::ref(stakeForGas_amount), std::ref(out_str)));

    // withdraw staked token for gas
    auto withdrawFund_app = resource_app->add_subcommand("withdrawFund", "Withdraw TOP tokens staked for resources (free gas).");
    std::string withdrawFund_amount("0");
    withdrawFund_app->add_option("top_num", withdrawFund_amount, "Amounts of deposit for gas will be withdrawed, unit is TOP.")->required();
    withdrawFund_app->callback(std::bind(&ApiMethod::withdraw_fund, &topcl.api, std::ref(withdrawFund_amount), std::ref(out_str)));
#endif
    /*
     * govern
     */
    auto govern_app = app.add_subcommand("govern", "Govern propasal on chain.");

    // get proposal
    auto getProposal_app = govern_app->add_subcommand("getProposal", "Query specific proposal details.");
    std::string getProposal_id;
    getProposal_app->add_option("proposal_id", getProposal_id, "Proposal ID. If you do not add this parameter, all proposals will be queried by default.");
    getProposal_app->callback(std::bind(&ApiMethod::get_proposal, &topcl.api, std::ref(getProposal_id), std::ref(out_str)));

    // get cgp
    auto CGP_app = govern_app->add_subcommand("CGP", "Get on-chain governance parameters.");
    CGP_app->callback(std::bind(&ApiMethod::cgp, &topcl.api, std::ref(out_str)));

    // submit proposal
    auto submitProposal_app = govern_app->add_subcommand("submitProposal", "Submit the on-chain governance proposal.");
    uint8_t submitProposal_type = 0;
    std::string submitProposal_target;
    std::string submitProposal_value;
    std::string submitProposal_deposit("0");
    uint64_t submitProposal_effective_timer_height = 0;
    submitProposal_app
        ->add_option("proposal_type", submitProposal_type, "Proposal Type：1--on-chain governance parameter modification proposal；2--community fund management proposal.")
        ->required();
    submitProposal_app
        ->add_option(
            "target", submitProposal_target, "When proposal_type is \"1\": target is on-chain governance parameter. When proposal_type is \"2\": target is burn account address.")
        ->required();
    submitProposal_app
        ->add_option(
            "value", submitProposal_value, "When target is on-chain governance parameter, value=new parameter value. When target is burn account address, value=transfered.")
        ->required();
    submitProposal_app->add_option("proposal_deposit", submitProposal_deposit, "Proposal deposit(on-chain governance parameter).")->required();
    submitProposal_app->add_option("effective_timer_height", submitProposal_effective_timer_height, "Clock height for proposal to take effect.")->required();
    submitProposal_app->callback(std::bind(&ApiMethod::submit_proposal,
                                           &topcl.api,
                                           std::ref(submitProposal_type),
                                           std::ref(submitProposal_target),
                                           std::ref(submitProposal_value),
                                           std::ref(submitProposal_deposit),
                                           std::ref(submitProposal_effective_timer_height),
                                           std::ref(out_str)));

    // withdraw proposal
    auto withdrawProposal_app = govern_app->add_subcommand("withdrawProposal", "Withdraw proposal.");
    std::string withdrawProposal_id;
    withdrawProposal_app->add_option("proposal_id", withdrawProposal_id, "Proposal ID.")->required();
    withdrawProposal_app->callback(std::bind(&ApiMethod::withdraw_proposal, &topcl.api, std::ref(withdrawProposal_id), std::ref(out_str)));

    // tcc vote
    auto tccVote_app = govern_app->add_subcommand("tccVote", "TCC(TOP Network Community Council) vote on proposal.");
    std::string tccVote_id;
    std::string tccVote_opinion;
    tccVote_app->add_option("proposal_id", tccVote_id, "Proposal ID.")->required();
    tccVote_app->add_option("opinion", tccVote_opinion, "Vote:true or false.")->required();
    tccVote_app->callback(std::bind(&ApiMethod::tcc_vote, &topcl.api, std::ref(tccVote_id), std::ref(tccVote_opinion), std::ref(out_str)));

    auto db = app.add_subcommand("db", "Manage the chain database.");
    db->require_subcommand();
    auto backup = db->add_subcommand("backup", "Backup the chain database to a specified directory.");
    std::string backupFromDir, backupToDir;
    backup->add_option("-d,--dir", backupFromDir, "Database-source directory,the default is current directory.");
    // backup->add_flag("-l,--list_DBversion",listflag,"List all the available backup version for restore.");
    backup->add_option("backupdir", backupToDir, "Target directory.")->mandatory();

    backup->callback([&]() {
        // backup directory can not fill in and that means current directory
        auto dbdir = backupToDir + DB_PATH;
        if (backupFromDir.empty()) {
            // get currrent directory
            backupFromDir = get_working_path();
            if (backupFromDir.empty()) {
                out_str << "Backup failed\n,Error: faield to get the current directory." << std::endl;
                return;
            }
        }

        if (0 == db_backup(backupFromDir, backupToDir)) {
            auto backup_info = db_backup_list_info(dbdir);
            out_str << "DBversion: " << backup_info.latest_backup_id() << std::endl;
        }
    });

    std::string listbackupToDir{};
    auto listversion = db->add_subcommand("listBackups", "List all verions of backup database.");
    listversion->add_option("backupdir", listbackupToDir, "Target database backup directory.")->mandatory();
    listversion->callback([&]() {
        auto dbdir = listbackupToDir + DB_PATH;
        auto backup_info = db_backup_list_info(dbdir);
        if (backup_info.empty()) {
            out_str << "No data." << std::endl;
        } else {
            auto backup_ids = backup_info.backup_ids_and_timestamps();
            char backup_date[100];
            for (auto id_ts_pair : backup_ids) {
                time_t rawtime(id_ts_pair.second);
                struct tm * p = gmtime(&rawtime);
                strftime(backup_date, sizeof(backup_date), "%Y-%m-%d %H:%M:%S", p);
                out_str << "DBversion:" << id_ts_pair.first << ",timestamp:" << backup_date << "." << std::endl;
            }
        }
    });

    auto restore = db->add_subcommand("restore", "Restore the database from the backup directory.");
    std::string restoreFromDir, restoreToDir;
    int backupid = 0;
    restore->add_option("-d,--dir", restoreToDir, "The target diectory to restore(be empty),the default is current directory.");
    restore->add_option("-D,--DBversion", backupid, "Database backup version ID, the default is the lastest version.");
    restore->add_option("backupdir", restoreFromDir, "Target directory.")->mandatory();
    restore->callback([&]() {
        if (restoreToDir.empty()) {
            // get currrent directory
            restoreToDir = get_working_path();
            if (restoreToDir.empty()) {
                out_str << "Restore failed\nError: faield to get the current directory." << std::endl;
                return;
            }
        }

        if (backupid == 0) {
            auto dbdir = restoreFromDir + DB_PATH;
            auto backup_info = db_backup_list_info(dbdir);
            if (backup_info.empty()) {
                out_str << "Restore failed\nError: cannot find the dbversion." << std::endl;
                return;
            }
            backupid = backup_info.latest_backup_id();
        }

        db_restore(restoreFromDir, restoreToDir, backupid);
    });

    auto cmd_db_download = db->add_subcommand("download", "download database.");
    std::string download_addr;
    cmd_db_download->add_option("download_addr", download_addr, "Download address.")->mandatory();
    cmd_db_download->callback([&]() {
        db_download(config_extra_json["datadir"].get<std::string>(), download_addr, out_str);
    });

    auto cmd_db_prune = db->add_subcommand("prune", "prune database.");
    cmd_db_prune->callback([&]() {
        std::string account = topcl.api.get_account_from_daemon();
        db_prune::DbPrune::instance().db_prune(account, config_extra_json["datadir"].get<std::string>(), out_str);
    });
    auto cmd_db_compact = db->add_subcommand("compact", "compact database.");
    std::string compact_dir{};
    cmd_db_compact->add_option("-d,--dir", compact_dir, "Database directory which to compact."); 
    cmd_db_compact->callback([&]() {
        std::string dbdir;
        if (!compact_dir.empty()) {
            dbdir = compact_dir;
        } else {
            dbdir = config_extra_json["datadir"].get<std::string>();
        }
        db_prune::DbPrune::instance().compact_db(dbdir, out_str);
    });
    auto cmd_db_convert = db->add_subcommand("convert", "convert database.");
    std::string convert_dir;
    cmd_db_convert->add_option("-d,--dir", convert_dir, "Database directory which to convert.")->mandatory();
    cmd_db_convert->add_option("miner_type", miner_type, "Miner type.")->required();   
    cmd_db_convert->callback([&]() {
        auto dbdir = convert_dir;
        db_prune::DbPrune::instance().db_convert(std::ref(miner_type), dbdir, out_str);
    });

    int app_ret = 0;
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError & e) {
        return app.exit(e);
    }

    std::cout << out_str.str() << std::endl;
    return app_ret;
}

// cmdline: node help
bool handle_node_command(const std::pair<std::string, uint16_t> & admin_http, const std::string & cmdline, std::string & result) {
    chain_command::ChainCommandFetcher http_fetcher{admin_http};
    std::string http_json_result;
    http_fetcher.Request(cmdline,http_json_result);
    if (http_json_result.empty()) {
        return false;
    }

    json response;
    try {
        /*
        {
            "status": "",
            "error": "",
            "data": {
                "node help": "",
            }
        }
        */

        response = json::parse(http_json_result);
        std::string status = response["status"].get<std::string>();
        auto data = response["data"];
        if (status != "ok") {
            result = response["error"].get<std::string>();
            return false;
        }

        for (json::iterator it = data.begin(); it != data.end(); ++it) {
            result += it.value();
        }
    } catch (json::parse_error & e) {
        result = http_json_result;
        TOP_WARN("%s", result.c_str());
        return false;
    }

    return true;
}

std::string decrypt_keystore_by_key(const std::string & keystore_path, const std::string & token) {
    std::string private_key;
    xChainSDK::xcrypto::decrypt_keystore_file_by_kdf_key(token, keystore_path, private_key);
    return private_key;
}

uint64_t GetAvailableSpace(const char * path) {
    struct statvfs stat;

    if (statvfs(path, &stat) != 0) {
        return -1;
    }
    // the available size is f_bsize * f_bavail
    return stat.f_bsize * stat.f_bavail;
}
uint64_t get_db_file_length(const std::string& str_out) {
    uint64_t len = 0;
    std::string strtemp = str_out;
    std::string::size_type found = str_out.find("Length: ");
    if (found == std::string::npos)
        return 0;
    strtemp = strtemp.substr(found + 8);
    //printf("find2: %s\n", strtemp.c_str());

    found = strtemp.find(" ");
    if (found == std::string::npos)
        return 0;
    strtemp = strtemp.substr(0, found);
    //printf("find3: %s\n", strtemp.c_str());
    len = std::strtoull(strtemp.c_str(), NULL, 0);
    return len;
}
int db_download(const std::string datadir, const std::string& download_addr, std::ostringstream& out_str) {
    std::string local_db_dir = datadir + DB_PATH;
    if (!IsDirEmpty(local_db_dir.c_str())) {
        out_str << "The target dir for restore is not empty: " << local_db_dir << std::endl;;
        return 1;
    }
    std::string::size_type found = download_addr.find_last_of('/');
    if (found == std::string::npos) {
        out_str << "download address error." << std::endl;
        return 1;
    }
    std::string db_filename = download_addr.substr(found + 1);
    if (db_filename.empty())
        return 1;
    if (db_filename.substr(db_filename.size()-7) != ".tar.gz") {
        out_str<<"only support tar.gz file format."<<std::endl;
        return 1;
    }

    std::string down_cmd;
    char tmp[4096] = {0};
    uint64_t file_length = 0;
    FILE * output = NULL;
    // get file length
    {
        down_cmd = std::string("wget --spider ") + download_addr + " 2>&1";
        printf("%s\n", down_cmd.c_str());
        output = popen(down_cmd.c_str(), "r");
        if (!output) {
            printf("popen failed\n");
            return 1;
        }
        while (fgets(tmp, sizeof(tmp), output) != NULL) {
            printf("%s", tmp);
            std::string strtemp = tmp;
            if (strtemp.substr(0, 8).compare("Length: ") == 0) {
                file_length = get_db_file_length(strtemp);
                break;
            }
        }
        pclose(output);
        printf("file length: %llu\n", static_cast<unsigned long long>(file_length));
        if (file_length == 0) {
            return 0;
        }
    }

    uint64_t free_space = GetAvailableSpace(datadir.c_str());
    if (free_space < 0) {
        printf("get free space fail: %s\n", datadir.c_str());
        return 0;
    }
    printf("free space: %llu\n", static_cast<unsigned long long>(free_space));
    if (free_space < file_length / 2 * 3) {
        printf("No enough free disk space.\n");
        return 0;
    }

    // wget download and extract file with one command
    if (free_space < file_length / 2 * 5) {
        down_cmd = std::string("wget -c ") + download_addr + " -O - | tar -xz -C " + datadir;
        printf("%s\n", down_cmd.c_str());
        output = popen(down_cmd.c_str(), "r");
        if (!output) {
            printf("popen failed\n");
            return 1;
        }
        while (fgets(tmp, sizeof(tmp), output) != NULL) {
            printf("%s", tmp);
        }
        pclose(output);
        out_str << "download and extract db ok." << std::endl;;
        return 0;
    }

    // download file and check md5sum
    std::string md5sum;
    {
        std::string tmp_filename = db_filename.substr(0, db_filename.size() - 7);  // remove ".tar.gz"
        found = tmp_filename.find_last_of('_');
        if (found == std::string::npos) {
            out_str << "database file name format error." << std::endl;
            return 1;
        }
        md5sum = tmp_filename.substr(found + 1);  // get md5sum

        tmp_filename = tmp_filename.substr(found);
        found = tmp_filename.find_last_of('_');
        if (found == std::string::npos) {
            out_str << "database file name format error." << std::endl;
            return 1;
        }
        std::string db_version = std::string("/db_") + tmp_filename.substr(found + 1);  // get db version
    }
    // wget download file
    {
        down_cmd = std::string("wget ") + download_addr;
        output = popen(down_cmd.c_str(), "r");
        if (!output) {
            printf("popen failed\n");
            return 1;
        }
        while (fgets(tmp, sizeof(tmp), output) != NULL) {
            printf("%s", tmp);
        }
        pclose(output);
        printf("download database ok.\n");
    }
    // check md5sum
    {
        down_cmd = std::string("md5sum ./") + db_filename;
        output = popen(down_cmd.c_str(), "r");
        if (!output) {
            printf("popen failed\n");
            return 1;
        }
        std::string md5_tmp;
        while (fgets(tmp, sizeof(tmp), output) != NULL) {
            printf("%s", tmp);
            md5_tmp = tmp;
        }
        pclose(output);
        found = md5_tmp.find(' ');
        if (md5_tmp.substr(0, found) != md5sum) {
            printf("md5 check failed: %s, %s\n", md5_tmp.substr(0, found).c_str(), md5sum.c_str());
            return 1;
        }
        printf("md5sum check ok.\n");
    }
    // extract file
    {
        down_cmd = std::string("tar zxvf ./") + db_filename + " -C " + datadir;
        printf("extract database: %s\n", down_cmd.c_str());
        output = popen(down_cmd.c_str(), "r");
        if (!output) {
            printf("popen failed\n");
            return 1;
        }
        std::string out;
        while (fgets(tmp, sizeof(tmp), output) != NULL) {
            if (out.empty())
                out = tmp;
            printf("%s", tmp);
        }
        found = out.find('/');
        if (found == std::string::npos) {
            printf("extract database error.\n");
            pclose(output);
            return 1;
        }
        pclose(output);
        printf("extract database to '%s' ok.\n", out.substr(0, found).c_str());
    }
    return 0;
}

}  //  namespace top
