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


namespace top {

using data::xuser_params;
using data::node_info_t;

void xchain_params::initconfig_using_configcenter() {
    load_user_config();
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

    // std::string sign_key;
    // if (!config_register.get("sign_key", sign_key)) {
    //     assert(0);
    // }
    //user_params.signkey = base::xstring_utl::base64_decode(sign_key);
    // user_params.signkey = sign_key;
#if defined(XENABLE_MOCK_ZEC_STAKE)
    std::string role_type;
    if(!config_register.get("node_type",role_type)){
        assert(0);
    }
    user_params.node_role_type = common::to_miner_type(role_type);
    if (user_params.node_role_type == common::xminer_type_t::invalid) {
        xerror("[user config] node_type invalid");
        throw std::logic_error{"node_type invalid"};
    }
#endif
    return;
}

int xchain_params::get_uuid(std::string& uuid) {
    //cat /sys/class/dmi/id/product_uuid
    //dmidecode -s system-uuid
    FILE *output = popen("cat /sys/class/dmi/id/product_uuid", "r");
    if (!output) {
        return 1;
    }
    char tmp[512]={0};

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-result"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-result"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

    fgets(tmp, sizeof(tmp), output);

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

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
