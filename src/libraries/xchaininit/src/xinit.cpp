#include <string>
#include <unistd.h>
#include <unordered_map>
#include <fstream>
#include <signal.h>

#include "xdb/xdb_factory.h"
#include "xdbstore/xstore.h"
#include "xbase/xcontext.h"
#include "xcommon/xnode_type.h"
#include "xrpc/xrpc_init.h"
#include "xchaininit/xinit.h"
#include "xchaininit/xconfig.h"
#include "xchaininit/xchain_options.h"
#include "xchaininit/xchain_params.h"
#include "xchaininit/xchain_command_http_server.h"
#include "xversion/version.h"
#include "xbase/xutl.h"
#include "xbase/xhash.h"
#include "xpbase/base/top_utils.h"
#include "xdata/xchain_param.h"
#include "xdata/xmemcheck_dbg.h"
#include "xconfig/xconfig_register.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xmetrics/xmetrics.h"
#include "xapplication/xapplication.h"
#include "xtopcl/include/global_definition.h"
#include "xtopcl/include/topcl.h"
#include "xdata/xverifier/xverifier_utl.h"
#include "xtopcl/include/api_method.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcheckpoint.h"
#include "xsync/xsync_object.h"
#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"
#include "xelect/client/xelect_client.h"
#include "xgenesis/xgenesis_manager.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xchain_fork/xfork_points.h"

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;


namespace top{

bool g_topchain_init_finish_flag = false;

class xtop_hash_t : public base::xhashplugin_t
{
public:
    xtop_hash_t()
        :base::xhashplugin_t(-1) //-1 = support every hash types
    {
    }
private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator = (const xtop_hash_t &) = delete;
public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input,enum_xhash_type type) override
    {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char*>(hash.data()), hash.size());
    }
};

static bool create_rootblock(const std::string & config_file) {
    auto genesis_loader = std::make_shared<loader::xconfig_genesis_loader_t>(config_file);
    // config::xconfig_register_t::get_instance().add_loader(genesis_loader);
    // config::xconfig_register_t::get_instance().load();
    data::xrootblock_para_t rootblock_para;
    if (false == genesis_loader->extract_genesis_para(rootblock_para)) {
        xerror("create_rootblock extract genesis para fail");
        return false;
    }
    if (false == data::xrootblock_t::init(rootblock_para)) {
        xerror("create_rootblock rootblock init fail");
        return false;
    }
    xinfo("create_rootblock success");
    return true;
}

bool set_auto_prune_switch(const std::string& prune)
{
    std::string prune_enable = prune;
    top::base::xstring_utl::tolower_string(prune_enable);
    if (prune_enable == "on")
        base::xvchain_t::instance().enable_auto_prune(true);
    else
        base::xvchain_t::instance().enable_auto_prune(false);
    return true;
}

bool db_migrate(const std::string & src_db_path)
{    
    // base::xvblockstore_t* _blockstore = base::xvchain_t::instance().get_xblockstore();
    // xassert(_blockstore != nullptr);
    // return base::db_delta_migrate_v2_to_v3(src_db_path, _blockstore);
    // XTODO no old version db need migrate
    return true;
}

void on_sys_signal_callback(int signum, siginfo_t *info, void *ptr)
{
    switch(signum)
    {
        //signals to generate core dump
    case SIGSEGV:
    case SIGILL:
    case SIGFPE:
    case SIGABRT:
        {
            printf("on_sys_signal_callback:capture core_signal(%d)\n",signum);
            xwarn("on_sys_signal_callback:capture core_signal(%d)",signum);
            
            //trigger save data before coredump
            top::base::xvchain_t::instance().on_process_close();
            top::sync::xtop_sync_out_object::instance().save_span();
            //forward to default handler
            signal(signum, SIG_DFL);//restore to default handler
            kill(getpid(), signum); //send signal again to genereate core dump by default hander
        }
        break;
        
        //signal to terminate process
    case SIGHUP:
    case SIGINT:
    case SIGTERM:
        {
            printf("on_sys_signal_callback:capture terminate_signal(%d) \n",signum);
            xwarn("on_sys_signal_callback:capture terminate_signal(%d)",signum);
            
            //trigger save data before terminate
            top::base::xvchain_t::instance().on_process_close();
            top::sync::xtop_sync_out_object::instance().save_span();
            //forward to default handler
            signal(signum, SIG_DFL);//restore to default handler
            kill(getpid(), signum); //send signal again to default handler
        }
        break;
        
    case SIGUSR1:
    case SIGUSR2:
        {
            printf("on_sys_signal_callback:capture user_signal(%d)\n",signum);
            xwarn("on_sys_signal_callback:capture user_signal(%d)",signum);
            
            //trigger save data
            top::base::xvchain_t::instance().save_all();
            top::sync::xtop_sync_out_object::instance().save_span();
        }
        break;
        
    default:
        {
            printf("on_sys_signal_callback:capture other_signal(%d) \n",signum);
            xwarn("on_sys_signal_callback:capture other_signal(%d)",signum);
        }
    }
}

void catch_system_signals()
{
// XTODO always not enable signal capture feature for dead-lock issue
#if 0  // #ifndef DISABLE_SIGNAL_CAPTURE
    static struct sigaction _sys_sigact;
    memset(&_sys_sigact, 0, sizeof(_sys_sigact));
    
    _sys_sigact.sa_sigaction = on_sys_signal_callback;
    _sys_sigact.sa_flags = SA_SIGINFO;
    
    //config signal of termine
    sigaction(SIGTERM, &_sys_sigact, NULL);
    signal(SIGINT, SIG_IGN); //disable INT signal
    signal(SIGHUP, SIG_IGN); //disable HUP signal
    
#ifndef DISABLE_CORE_SIGNAL_CAPTURE
    //config signal of cores
    sigaction(SIGSEGV, &_sys_sigact, NULL);
    sigaction(SIGILL, &_sys_sigact, NULL);
    sigaction(SIGFPE, &_sys_sigact, NULL);
    sigaction(SIGABRT, &_sys_sigact, NULL);
#endif        

    //config user 'signal
    sigaction(SIGUSR1, &_sys_sigact, NULL);
    sigaction(SIGUSR2, &_sys_sigact, NULL);
    std::cout << "catch_system_signals" << std::endl;
#endif
}

int topchain_init(const std::string& config_file, const std::string& config_extra) {
    using namespace std;
    using namespace base;
    using namespace network;
    using namespace vnetwork;
    using namespace store;
    using namespace rpc;

    catch_system_signals();//setup and hook system signals

    // init up
    setup_options();
    g_topchain_init_finish_flag = false;

    auto& config_center = top::config::xconfig_register_t::get_instance();
    auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>(config_file, config_extra);
    config_center.add_loader(offchain_loader);
    if (!config_center.load()) {
        xwarn("parse config %s failed!", config_file.c_str());
        return -1;
    }
    config_center.remove_loader(offchain_loader);
    config_center.init_static_config();

    std::string v3_db_path = XGET_CONFIG(db_path) + DB_PATH;
    config_center.set(config::xdb_path_configuration_t::name, v3_db_path);

    return topchain_start("", config_file);
}

void load_bwlist_config(const std::string& datadir, config::xconfig_register_t& config_center) {
    std::string bwlist_path;
    std::string dirpath;
    if (datadir.empty()) {
        dirpath = ".";  //current dir
    } else {
        dirpath = datadir;
    }
#ifdef _WIN32
    bwlist_path = dirpath + "\\bwlist.json"
#else
    bwlist_path = dirpath + "/bwlist.json";
#endif
    std::map<std::string, std::string> bwlist;
    auto ret = top::xverifier::xtx_utl::load_bwlist_content(bwlist_path, bwlist);
    if (ret) {
        for (auto const& item: bwlist) {
            xinfo("load_bwlist_config load: key %s, value %s", item.first.c_str(), item.second.c_str());
            config_center.set(item.first, item.second);
        }
    } else {
        xwarn("load_bwlist_config failed! path=%s", bwlist_path.c_str());  // maybe has no file, it is ok.
    }
}

int topchain_start(const std::string& datadir, const std::string& config_file) {
    auto hash_plugin = new xtop_hash_t();
    if (false == create_rootblock(config_file)) {
        return 1;
    }

    auto& config_center = top::config::xconfig_register_t::get_instance();

    xchain_params chain_params;
    // attention: put chain_params.initconfig_using_configcenter behind config_center
    chain_params.initconfig_using_configcenter();
    auto& user_params = data::xuser_params::get_instance();
    global_node_id = user_params.account.to_string();
    global_node_pubkey = base::xstring_utl::base64_decode(user_params.publickey);

    std::string sign_key;
    if (!config_center.get("sign_key", sign_key)) {
        assert(false);
    }

    auto node_signkey = DecodePrivateString(sign_key);  // will be moved

    // clear cache.
    sign_key.clear();
    if (!config_center.set("sign_key", std::string{"***"})) {
        assert(false);
    }

    config_center.dump();

    auto const log_level = XGET_CONFIG(log_level);
    auto const log_path = XGET_CONFIG(log_path);
    std::cout << "account: " << global_node_id << std::endl;
    xinit_log(log_path.c_str(), true, true);
    xset_log_level((enum_xlog_level)log_level);
    auto xbase_info = base::xcontext_t::get_xbase_info();
    xkinfo("=== topio start here ===");
    xkinfo("=== topio start info: forkpoints: %s", fork_points::dump_fork_points().c_str());
    xkinfo("=== topio start info: xbase info: %s", xbase_info.c_str());
    xkinfo("=== topio start info: version info: %s", dump_version().c_str());

    // load bwlist
    load_bwlist_config(datadir, config_center);

    config_center.log_dump();
    std::cout << "=== topio start here ===" << std::endl;
    std::cout << "=== xbase info:" << xbase_info << " ===" << std::endl;

    //wait log path created,and init metrics
    XMETRICS_INIT2(log_path);

    // init checkpoint
    data::xchain_checkpoint_t::load();

    //init data_path into xvchain instance
    //init auto_prune feature
    set_auto_prune_switch(XGET_CONFIG(auto_prune_data));
    xkinfo("topchain_init init auto_prune_switch= %d", base::xvchain_t::instance().is_auto_prune_enable());

    MEMCHECK_INIT();

    // start chain command http server
    {
        uint16_t admin_http_port = 0;
        std::string admin_http_local_ip = "127.0.0.1";
        config_center.get("admin_http_addr", admin_http_local_ip);
        config_center.get<uint16_t>("admin_http_port", admin_http_port);

        std::cout << "will start chain command http server, local_ip:" << admin_http_local_ip << " port:" << admin_http_port << std::endl;
        xinfo("will start chain command http server, local_ip:%s port:%d", admin_http_local_ip.c_str(), admin_http_port);
        auto chain_command_server = std::make_shared<chain_command::ChainCommandServer>(admin_http_local_ip, admin_http_port);
        chain_command_server->Start();
    }

    application::xapplication_t app{user_params.account, xpublic_key_t{global_node_pubkey}, std::move(node_signkey)};
    // std::string v2_db_path = XGET_CONFIG(db_path) + "/db";  // TODO(jimmy) delete in v1.2.8
    // if (false == db_migrate(v2_db_path)) {
    //     return 1;
    // }
    try {
        app.start();
    } catch (top::error::xtop_error_t const & eh) {
        assert(false);
        xerror("application exit with xtop_error_t %d msg %s", eh.code().value(), eh.what());
    } catch (std::exception const & eh) {
        assert(false);
        xerror("application exit with std::exception msg %s", eh.what());
    }

    xinfo("==== app start done ===");

    std::cout << std::endl
        << "#####################################################################" << std::endl
        << "topio start ok" << std::endl
        << "#####################################################################" << std::endl;

    xinfo("#####################################################################");
    xinfo("topio start ok");
    xinfo("#####################################################################");

    // make block here
    while (true) {
        // TODO(jimmy) total 27 threads need optimize
        xinfo("total xbase threads number %d", base::xcontext_t::instance().get_total_threads());
        base::xvchain_t::instance().get_xdbstore()->GetDBMemStatus();
#ifdef DEBUG        
        std::this_thread::sleep_for(std::chrono::seconds(120));
#else 
        std::this_thread::sleep_for(std::chrono::seconds(300));
#endif
    }
    // todo adapter to vnode type

    app.stop();
    config_center.clear_loaders();
    delete hash_plugin;
    return 0;
}

// Commented out by Charles.Liu 2020.05.19
// node_type has been deleted in config.json
// void
// set_node_type(const uint32_t role_type) {
//     auto& user_params = data::xuser_params::get_instance();
//     user_params.node_role_type = static_cast<top::common::xminer_type_t>(role_type);
// }

bool check_miner_info(const std::string &pub_key, const std::string &node_id, std::string& miner_type) {
    g_userinfo.account = node_id;
    top::base::xvaccount_t _vaccount(node_id);
    if (_vaccount.is_eth_address()) {
        std::transform(g_userinfo.account.begin() + 1, g_userinfo.account.end(), g_userinfo.account.begin() + 1, ::tolower);
    }

    top::xtopcl::xtopcl xtop_cl;
    std::ostringstream out_str;
    xtop_cl.api.change_trans_mode(true);

    xtop_cl.api.query_miner_info(g_userinfo.account, out_str);
    std::string result = out_str.str();
    if (result.find("account_addr") == std::string::npos) {
        std::cout << g_userinfo.account << " account has not registered miner." << std::endl;
        // std::cout << "result:"<<result<< std::endl;
        return false;
    }

    // registered
    json response;
    try {
        response = json::parse(result);
        if (response.contains("data")) {
            auto data = response["data"];
            auto node_sign_key = data["node_sign_key"].get<std::string>();
            miner_type = data["registered_node_type"].get<std::string>();
            // std::cout << "miner type: " <<miner_type <<std::endl;
            if (node_sign_key != pub_key) {
                std::cout << "The minerkey does not match miner account." << std::endl
                    << g_userinfo.account << " account's miner key is "
                    << node_sign_key << std::endl;
                return false;
            }

            // account registered and pub_key matched
            return true;
        }
    } catch (json::parse_error& e) {
        std::cout << "json parse error:" << result << std::endl;
        return false;
    }

    return false;
}

/*
void check_miner(const std::string &pub_key, const std::string &node_id) {
    bool status = check_miner_info(pub_key, node_id);
    if (!status || father_pid <= 0) {
        std::cout << "Start node failed." << std::endl;
        kill(father_pid, SIGTERM);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ::_Exit(1);
    }
}
*/

int topchain_noparams_init(const std::string& pub_key, const std::string& pri_key, const std::string& node_id, const std::string& datadir, const std::string& config_extra) {
    using namespace std;
    using namespace base;
    using namespace network;
    using namespace vnetwork;
    using namespace store;
    using namespace rpc;
    //using top::elect::xbeacon_xelect_imp;

    catch_system_signals();//setup and hook system signals

    std::string chain_db_path = datadir + DB_PATH;
    std::string log_path;
    
#ifdef _WIN32
    log_path = datadir + "\\log";
    // TODO(smaug) mkdir in windows
#else
    log_path = datadir + "/log";

    std::string mk_cmd("mkdir -m 0755 -p ");
    mk_cmd += log_path;

    FILE *output = popen(mk_cmd.c_str(), "r");
    if (!output) {
        printf("popen(%s) failed ", mk_cmd.c_str());
        return 1;
    }
    pclose(output);
#endif


    XMETRICS_INIT2(log_path);


    std::cout << "topio config initializing..."  << std::endl;
    xinfo("topio config initializing...") ;
    auto& config_center = top::config::xconfig_register_t::get_instance();
    // only for cofig extra
    auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>("", config_extra);
    config_center.add_loader(offchain_loader);
    config_center.load();
    config_center.remove_loader(offchain_loader);
    config_center.init_static_config();

    config_center.set("node_id",    std::string(node_id));
    config_center.set("public_key", pub_key);
    config_center.set("sign_key",   pri_key);
    config_center.set("datadir", datadir);
    config_center.set(config::xdb_path_configuration_t::name, chain_db_path);
    config_center.set(config::xlog_path_configuration_t::name, log_path);

    std::string bootnodes;
    config_center.get("bootnodes", bootnodes);
    if (!bootnodes.empty()) {
        std::string default_bootnodes = XGET_CONFIG(p2p_endpoints);
        if (!default_bootnodes.empty()) {
            if (bootnodes.back() != ',') {
                bootnodes += ",";
            }
            bootnodes += default_bootnodes;
        }
        XSET_CONFIG(p2p_endpoints, bootnodes);
    }

    //init data_path into xvchain instance
    base::xvchain_t::instance().set_data_dir_path(datadir);

    return topchain_start(datadir, "");
}

}
