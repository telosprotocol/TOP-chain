#include <string>
#include <unistd.h>
#include <unordered_map>
#include <fstream>

#include "xdb/xdb_factory.h"
#include "xstore/xstore.h"
#include "xbase/xcontext.h"
#include "xcommon/xnode_type.h"
#include "xrpc/xrpc_init.h"
#include "xchaininit/xinit.h"
#include "xchaininit/xconfig.h"
#include "xchaininit/xchain_options.h"
#include "xchaininit/xchain_params.h"
#include "xbase/xutl.h"
#include "xbase/xhash.h"
#include "xpbase/base/top_utils.h"
#include "xdata/xchain_param.h"
#include "xdata/xmemcheck_dbg.h"
#include "xconfig/xconfig_register.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xmutisig/xmutisig.h"
#include "xmetrics/xmetrics.h"
#include "xapplication/xapplication.h"
#include "xchaininit/admin_http.h"
#include "xtopcl/include/topcl.h"
#include "xverifier/xverifier_utl.h"
#include "xtopcl/include/api_method.h"
#include "xconfig/xpredefined_configurations.h"
#include "xmigrate/xvmigrate.h"

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
    xrootblock_para_t rootblock_para;
    if (false == genesis_loader->extract_genesis_para(rootblock_para)) {
        xerror("create_rootblock extract genesis para fail");
        return false;
    }
    if (false == xrootblock_t::init(rootblock_para)) {
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
    base::xvblockstore_t* _blockstore = base::xvchain_t::instance().get_xblockstore();
    xassert(_blockstore != nullptr);
    return base::db_delta_migrate_v2_to_v3(src_db_path, _blockstore);
}

int topchain_init(const std::string& config_file, const std::string& config_extra) {
    using namespace std;
    using namespace base;
    using namespace network;
    using namespace vnetwork;
    using namespace store;
    using namespace rpc;

    // init up
    setup_options();

    //using top::elect::xbeacon_xelect_imp;
    auto hash_plugin = new xtop_hash_t();


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

    // XTODO from v3, v3_db_path = config_path+DB_PATH
    std::string v2_db_path = XGET_CONFIG(db_path) + "/db";  // TODO(jimmy) delete in v1.2.8
    std::string v3_db_path = XGET_CONFIG(db_path) + DB_PATH;
    config_center.set(config::xdb_path_configuration_t::name, v3_db_path);

    xchain_params chain_params;
    // attention: put chain_params.initconfig_using_configcenter behind config_center
    chain_params.initconfig_using_configcenter();
    auto& user_params = data::xuser_params::get_instance();
    global_node_id = user_params.account.value();
    global_node_signkey = DecodePrivateString(user_params.signkey);

#ifdef CONFIG_CHECK
    // config check
    if (!user_params.is_valid()) return 1;
#endif

    config_center.dump();

    auto const log_level = XGET_CONFIG(log_level);
    auto const log_path = XGET_CONFIG(log_path);
    std::cout << "account: " << global_node_id << std::endl;
    xinit_log(log_path.c_str(), true, true);
    xset_log_level((enum_xlog_level)log_level);
    auto xbase_info = base::xcontext_t::get_xbase_info();
    xwarn("=== xtopchain start here ===");
    xwarn("=== xbase info: %s ===", xbase_info.c_str());
    config_center.log_dump();
    std::cout << "=== xtopchain start here ===" << std::endl;
    std::cout << "=== xbase info:" << xbase_info << " ===" << std::endl;

    //wait log path created,and init metrics
    XMETRICS_INIT2(log_path);

    //init data_path into xvchain instance
    //init auto_prune feature
    set_auto_prune_switch(XGET_CONFIG(auto_prune_data));
    xkinfo("topchain_init init auto_prune_switch= %d", base::xvchain_t::instance().is_auto_prune_enable());

    MEMCHECK_INIT();
    if (false == create_rootblock(config_file)) {
        return 1;
    }

    // start admin http service
    {
        uint16_t admin_http_port = 0;
        std::string admin_http_local_ip = "127.0.0.1";
        config_center.get("admin_http_addr", admin_http_local_ip);
        config_center.get<uint16_t>("admin_http_port", admin_http_port);

        std::string webroot;
#ifdef _WIN32
        webroot = "C:\\";
#elif __APPLE__
        webroot = "/var";
#elif __linux__
        webroot = "/var";
#elif __unix__
        webroot = "/var";
#else
#error  "Unknown compiler"
#endif

        std::cout << "will start admin http, local_ip:" << admin_http_local_ip << " port:" << admin_http_port << " webroot:" << webroot << std::endl;
        xinfo("will start admin http, local_ip:%s  port:%d webroot:%s", admin_http_local_ip.c_str(), admin_http_port, webroot.c_str());
        auto admin_http = std::make_shared<admin::AdminHttpServer>(admin_http_local_ip, admin_http_port, webroot);
        admin_http->Start();
    }
    // end admin http service

    //xmutisig::xmutisig::init_openssl();
    application::xapplication_t app{
        user_params.account,
        xpublic_key_t{ user_params.publickey },
        user_params.signkey
    };

    if (false == db_migrate(v2_db_path)) {
        return 1;
    }

    app.start();
    xinfo("==== app start done ===");

    std::cout << std::endl
        << "#####################################################################" << std::endl
        << "xtopchain start ok" << std::endl
        << "#####################################################################" << std::endl;

    xinfo("#####################################################################");
    xinfo("xtopchain start ok");
    xinfo("#####################################################################");

    // make block here
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // todo adapter to vnode type

    config_center.clear_loaders();
    app.stop();
    delete hash_plugin;
    return 0;
}

// Commented out by Charles.Liu 2020.05.19
// node_type has been deleted in config.json
// void
// set_node_type(const uint32_t role_type) {
//     auto& user_params = data::xuser_params::get_instance();
//     user_params.node_role_type = static_cast<top::common::xrole_type_t>(role_type);
// }

bool load_bwlist_content(std::string const& config_file, std::map<std::string, std::string>& result) {
    if (config_file.empty()) {
        xwarn("load local black/white list error!");
        return false;
    }

    std::ifstream in(config_file);
    if (!in.is_open()) {
        xwarn("open local black/white list file %s error", config_file.c_str());
        return false;
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    in.close();
    auto json_content = std::move(oss.str());
    if (json_content.empty()) {
        xwarn("black/white list config file %s empty", config_file.c_str());
        return false;
    }


    xJson::Value  json_root;
    xJson::Reader  reader;
    bool ret = reader.parse(json_content, json_root);
    if (!ret) {
        xwarn("parse config file %s failed", config_file.c_str());
        return false;
    }


    auto const members = json_root.getMemberNames();
    for(auto const& member: members) {
        if (json_root[member].isArray()) {
            for (unsigned int i = 0; i < json_root[member].size(); ++i) {
                try {
                    auto const& addr = json_root[member][i].asString();
                    if (addr.size() <= top::base::xvaccount_t::enum_vaccount_address_prefix_size) return false;
                    auto const addr_type = top::base::xvaccount_t::get_addrtype_from_account(addr);
                    if (addr_type != top::base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_eth_user_account && addr_type != top::base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_user_account) return false;
                    if ( top::xverifier::xverifier_success != top::xverifier::xtx_utl::address_is_valid(addr)) return false;
                } catch (...) {
                    xwarn("parse config file %s failed", config_file.c_str());
                    return false;
                }

                result[member] += json_root[member][i].asString() + ",";
            }
        } else {
            xwarn("parse config file %s failed", config_file.c_str());
            return false;
        }

    }

    xdbg("load black/whitelist successfully!");
    return true;

}

bool check_miner_info(const std::string &pub_key, const std::string &node_id) {
    g_userinfo.account = node_id;
    if (top::base::xvaccount_t::get_addrtype_from_account(g_userinfo.account) == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        std::transform(g_userinfo.account.begin() + 1, g_userinfo.account.end(), g_userinfo.account.begin() + 1, ::tolower);
    top::xtopcl::xtopcl xtop_cl;
    std::string result;
    xtop_cl.api.change_trans_mode(true);
    std::vector<std::string> param_list;
    std::string query_cmdline    = "system queryNodeInfo " + g_userinfo.account;
    xtop_cl.parser_command(query_cmdline, param_list);
    xtop_cl.do_command(param_list, result);
    auto query_find = result.find("account_addr");
    if (query_find == std::string::npos) {
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

    auto hash_plugin = new xtop_hash_t();
    std::string chain_db_path = datadir + DB_PATH;
    std::string log_path;
    std::string bwlist_path;
#ifdef _WIN32
    log_path = datadir + "\\log";
    bwlist_path = datadir + "\\bwlist.json"
    // TODO(smaug) mkdir in windows
#else
    log_path = datadir + "/log";
    bwlist_path = datadir + "/bwlist.json";

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


    std::cout << "xnode config initializing..."  << std::endl;
    xinfo("xnode config initializing...") ;
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
    config_center.set(config::xplatform_db_path_configuration_t::name, chain_db_path);

    uint16_t net_port = 0;
    config_center.get<uint16_t>("net_port", net_port);
    if (net_port != 0) {
        config_center.set(config::xplatform_business_port_configuration_t::name, net_port);
    }
    std::string bootnodes;
    config_center.get("bootnodes", bootnodes);
    if (!bootnodes.empty()) {
        std::string default_bootnodes;
        config_center.get(config::xplatform_public_endpoints_configuration_t::name, default_bootnodes);
        if (!default_bootnodes.empty()) {
            if (bootnodes.back() != ',') {
                bootnodes += ",";
            }
            bootnodes += default_bootnodes;
        }
        config_center.set(config::xplatform_public_endpoints_configuration_t::name, bootnodes);
    }

#ifdef DEBUG
    config_center.dump();
#endif

    xchain_params chain_params;
    // attention: put chain_params.initconfig_using_configcenter behind config_center
    chain_params.initconfig_using_configcenter();
    auto& user_params = data::xuser_params::get_instance();
    global_node_id = user_params.account.value();

    global_node_signkey = DecodePrivateString(user_params.signkey);

#ifdef CONFIG_CHECK
    // config check
    if (!user_params.is_valid()) {
        return 1;
    }
#endif
    auto const log_level = XGET_CONFIG(log_level);
    /*
    std::string log_path(data::xlog_path_configuration_t::value);
    if (!config_center.get(std::string(data::xlog_path_configuration_t::name), log_path)) {
        assert(0);
    }
    */
    std::cout << "account: " << global_node_id << std::endl;
    xinfo("account:%s", global_node_id.c_str());
    xinit_log(log_path.c_str(), true, true);
    xset_log_level((enum_xlog_level)log_level);
    xinfo("=== xtopchain start here with noparams ===");
    std::cout << "xnode start begin..." << std::endl;

    //init data_path into xvchain instance
    base::xvchain_t::instance().set_data_dir_path(datadir);
    //init auto_prune feature
    set_auto_prune_switch(XGET_CONFIG(auto_prune_data));
    xkinfo("topchain_noparams_init auto_prune_switch= %d", base::xvchain_t::instance().is_auto_prune_enable());

    // load bwlist
    std::map<std::string, std::string> bwlist;
    auto ret = load_bwlist_content(bwlist_path, bwlist);
    if (ret) {
        for (auto const& item: bwlist) {
            xdbg("key %s, value %s", item.first.c_str(), item.second.c_str());
            config_center.set(item.first, item.second);
        }
    } else {
        xdbg("load_bwlist_content failed!");
    }

    MEMCHECK_INIT();
    if (false == create_rootblock("")) {
        return 1;
    }
    // start admin http service
    {
        xinfo("==== start admin http server ===");
        uint16_t admin_http_port = 0;
        std::string admin_http_local_ip = "127.0.0.1";
        config_center.get("admin_http_addr", admin_http_local_ip);
        config_center.get<uint16_t>("admin_http_port", admin_http_port);

        if (admin_http_port == 0) {
            std::cout << "http port:" << admin_http_port << " invalid" << std::endl;
            return 1;
        }

        std::string webroot;

#ifdef _WIN32
        webroot = "C:\\";
#elif __APPLE__
        webroot = "/var";
#elif __linux__
        webroot = "/var";
#elif __unix__
        webroot = "/var";
#else
#error  "Unknown compiler"
#endif


        std::cout << "start admin http, local_ip:" << admin_http_local_ip << " port:" << admin_http_port << " webroot:" << webroot << std::endl;
        xinfo("start admin http, local_ip:%s port:%d webroot:%s", admin_http_local_ip.c_str(), admin_http_port, webroot.c_str());
        auto admin_http = std::make_shared<admin::AdminHttpServer>(admin_http_local_ip, admin_http_port, webroot);
        admin_http->Start();
    }
    // end admin http service

    //xmutisig::xmutisig::init_openssl();
    application::xapplication_t app{
        user_params.account,
        xpublic_key_t{ user_params.publickey },
        user_params.signkey
    };

    std::string v2_db_path = datadir + OLD_DB_PATH;
    if (false == db_migrate(v2_db_path)) {
        return 1;
    }

    app.start();

    std::cout << std::endl
        << "================================================" << std::endl
        << "topio start ok" << std::endl
        << "================================================" << std::endl;

    xinfo("================================================");
    xinfo("topio start ok");
    xinfo("================================================");
  
    // make block here
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    config_center.clear_loaders();
    app.stop();
    delete hash_plugin;
    return 0;
}

}
