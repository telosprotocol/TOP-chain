#include <fstream>

#include <json/json.h>

#include "xbase/xaes.h"
#include "xbase/xutl.h"
#include "xchaininit/xchain_params.h"
#include "xchaininit/xconfig.h"
#include "xcrypto/xckey.h"
#include "xdata/xchain_param.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xdata_common.h"
#include "xdata/xgenesis_data.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xelect_transaction.hpp"
#include "xdata/xgenesis_data.h"


#include "xconfig/xconfig_register.h"

namespace top {

using data::xuser_params;
using data::xdev_params;
using data::xplatform_params;
using data::xstaticec_params;
using data::node_info_t;

void xchain_params::initconfig_using_configcenter() {
    load_user_config();
    load_dev_config();
    load_platform_config();
}


void xchain_params::load_user_config() {
    xuser_params& user_params = xuser_params::get_instance();
    auto& config_register = top::config::xconfig_register_t::get_instance();

    std::string node_id;
    if (!config_register.get("node_id", node_id)) {
        assert(0);
    }
    user_params.account = common::xnode_id_t{ node_id };

    std::string public_key;
    if (!config_register.get("public_key", public_key)) {
        assert(0);
    }
    //user_params.publickey = base::xstring_utl::base64_decode(public_key);
    user_params.publickey = public_key;

    std::string sign_key;
    if (!config_register.get("sign_key", sign_key)) {
        assert(0);
    }
    //user_params.signkey = base::xstring_utl::base64_decode(sign_key);
    user_params.signkey = sign_key;
#if defined XENABLE_MOCK_ZEC_STAKE
    std::string role_type;
    if(!config_register.get("node_type",role_type)){
        assert(0);
    }
    user_params.node_role_type = common::to_role_type(role_type);
    if (user_params.node_role_type == common::xrole_type_t::invalid) {
        xerror("[user config] node_type invalid");
        throw std::logic_error{"node_type invalid"};
    }
#endif
    return;
}

void xchain_params::load_dev_config() {
    xdev_params& dev_params = xdev_params::get_instance();
    auto& config_register = top::config::xconfig_register_t::get_instance();

    if (!config_register.get("seed_edge_host", dev_params.seed_edge_host)) {
        //assert(0);
    }
}

void xchain_params::load_platform_config() {
    xplatform_params& platform_params = xplatform_params::get_instance();
    auto& config_register = top::config::xconfig_register_t::get_instance();

    // TODO(smaug) maybe we should use the real local_ip (for p2p nat)
    platform_params.local_ip = XGET_CONFIG(ip);
    platform_params.local_port = XGET_CONFIG(platform_business_port);
    platform_params.public_endpoints = XGET_CONFIG(platform_public_endpoints);
    platform_params.url_endpoints = XGET_CONFIG(platform_url_endpoints);
    platform_params.show_cmd = XGET_CONFIG(platform_show_cmd);
    platform_params.db_path = XGET_CONFIG(platform_db_path);
}

int xchain_params::get_uuid(std::string& uuid) {
    //cat /sys/class/dmi/id/product_uuid
    //dmidecode -s system-uuid
    FILE *output = popen("cat /sys/class/dmi/id/product_uuid", "r");
    if (!output) {
        return 1;
    }
    char tmp[512]={0};
    fgets(tmp, sizeof(tmp), output);
    if (tmp[strlen(tmp) - 1] == '\n') {
        tmp[strlen(tmp) - 1] = '\0';
    }
    pclose(output);
    if (strlen(tmp)==0){
        printf("get uuid fail.\n");
        return 1;
    }
    printf("local_uuid:%s\n", tmp);
    uuid = tmp;
    return 0;
}

}
