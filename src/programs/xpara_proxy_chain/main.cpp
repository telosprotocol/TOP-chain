#include "xbase/xbase.h"
#include "xbase/xhash.h"
#include "xbase/xutl.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xrootblock.h"
#include "xdata/xchain_param.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xloader/xconfig_genesis_loader.h"
#include "xmetrics/xmetrics.h"
#include "xconfig/xchain_names.h"

#include "xpara_proxy/xpara_proxy.h"


#include "xpbase/base/top_utils.h"

#include <string.h>

#include <iostream>

using namespace top;

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

int para_proxy_chain_init(const std::string& config_file, const std::string& config_extra) {
    using namespace std;
    using namespace base;
    // using namespace network;
    // using namespace vnetwork;
    // using namespace store;
    // using namespace rpc;

    // init up
    // setup_options();

    auto hash_plugin = new xtop_hash_t();

    XMETRICS_INIT();

    // offchain_loader must be the first!
    auto& config_center = top::config::xconfig_register_t::get_instance();
    auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>(config_file, config_extra);
    config_center.add_loader(offchain_loader);
    config_center.load();
    config_center.remove_loader(offchain_loader);
    config_center.init_static_config();

    auto& user_params = data::xuser_params::get_instance();
    global_node_id = user_params.account.value();
    global_node_signkey = DecodePrivateString(user_params.signkey);

#ifdef CONFIG_CHECK
    // config check
    if (!user_params.is_valid()) return 1;
#endif

    config_center.dump();


    auto const chain_name = XGET_CONFIG(chain_name);
    uint32_t chain_id;
    if (!config_center.get("chain_id", chain_id)) {
        assert(0);
    }
    // uint32_t chain_id = std::atoi(chain_id_str.c_str());
    assert(chain_id != base::enum_main_chain_id && chain_id != base::enum_test_chain_id);
    config::set_chain_name_id(chain_name, chain_id);

    auto const log_level = XGET_CONFIG(log_level);
    auto const log_path = XGET_CONFIG(log_path);
    std::cout << "account: " << global_node_id << std::endl;
    xinit_log(log_path.c_str(), true, true);
    xset_log_level((enum_xlog_level)log_level);
    auto xbase_info = base::xcontext_t::get_xbase_info();
    xwarn("=== xpara_proxy_chain start here ===");
    xwarn("=== xbase info: %s ===", xbase_info.c_str());
    std::cout << "=== xpara_proxy_chain start here ===" << std::endl;
    std::cout << "=== xbase info:" << xbase_info << " ===" << std::endl;

    if (false == create_rootblock(config_file)) {
        return 1;
    }

    //xmutisig::xmutisig::init_openssl();

    para_proxy::xpara_proxy_t para_proxy{
    // }
    // application::xapplication_t app{
        user_params.account,
        xpublic_key_t{ user_params.publickey },
        user_params.signkey
    };
    para_proxy.start();
    xinfo("==== app start done ===");

    std::cout << std::endl
        << "#####################################################################" << std::endl
        << "xpara_proxy_chain start ok" << std::endl
        << "#####################################################################" << std::endl;

    xinfo("#####################################################################");
    xinfo("xpara_proxy_chain start ok");
    xinfo("#####################################################################");

    // make block here
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // todo adapter to vnode type

    config_center.clear_loaders();
    para_proxy.stop();
    delete hash_plugin;
    return 0;
}

int main(int argc, char * argv[]) {
    // use this to join or quit network, config show_cmd = false will come here
    // auto elect_mgr = elect_main.elect_manager();
    try {
        if (argc < 2) {
            std::cout << "usage: xpara_proxy_chain config_file1 [config_file2]" << std::endl;
            _Exit(0);
        }

        if (strcmp(argv[1], "-v") == 0) {
            // print_version();
            _Exit(0);
        } else {
            // print_version();
            std::string config_file1 = argv[1];
            std::string config_file2;
            if (argc > 2) {
                config_file2 = argv[2];
            }
            para_proxy_chain_init(config_file1, config_file2);
        }
    } catch (const std::runtime_error & e) {
        xerror("Unhandled runtime_error Exception reached the top of main %s", e.what());
    } catch (const std::exception & e) {
        xerror("Unhandled Exception reached the top of main %s", e.what());
    } catch (...) {
        xerror("unknow exception");
    }
    return 0;
}
