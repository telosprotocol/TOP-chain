#include "xvm/xsystem_contracts/node_manage/xnode_manage_contract.h"

#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xserialization/xserialization.h"
#include "xca_auth/xca_auth.h"


using namespace top::data::system_contract;
using top::base::xcontext_t;

NS_BEG4(top, xvm, system_contracts, rec)

#if !defined (XNODE_MANAGE_CONTRACT_MODULE)
#    define XNODE_MANAGE_CONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "xNodeManage_"

#define XNODE_CONTRACT XNODE_MANAGE_CONTRACT_MODULE XCONTRACT_PREFIX

static std::string const ca_root_conent =
    R"T(-----BEGIN CERTIFICATE-----
MIIDmzCCAoOgAwIBAgIJAM3iW7K+W3CVMA0GCSqGSIb3DQEBCwUAMGQxCzAJBgNV
BAYTAkNOMREwDwYDVQQIDAhzaGFuZ2hhaTERMA8GA1UEBwwIc2hhbmdoYWkxEDAO
BgNVBAoMB2V4YW1wbGUxCzAJBgNVBAsMAml0MRAwDgYDVQQDDAdyb290LWNhMB4X
DTIyMTEwODEyMzAwNloXDTMyMTEwNTEyMzAwNlowZDELMAkGA1UEBhMCQ04xETAP
BgNVBAgMCHNoYW5naGFpMREwDwYDVQQHDAhzaGFuZ2hhaTEQMA4GA1UECgwHZXhh
bXBsZTELMAkGA1UECwwCaXQxEDAOBgNVBAMMB3Jvb3QtY2EwggEiMA0GCSqGSIb3
DQEBAQUAA4IBDwAwggEKAoIBAQC8pej34AyEtRyWqjzC6IhzjISlONTl1gCfb3xB
Cpct+bnycdTpJy24pKk5X1/TV5Dn9d2oSB/1G1dAQRQt2TsJcK8ZUwLhVmCMuWFg
cXikt4lqKuOObQucoUCz/AQgceKnADqh4E5OB2RPXZa5Aea1sl8jRAvY3iGYH+b8
bfu0PNv+E/sI7dsG8Y54akZ89HrldtnjLoJGa5+JgBSElE6mUpQYdI0cxNRZh/Ro
D9JuKMqXJbbAX72dxQwVtLriZ8iWi0wWwePa3LHPeqXwvMnE/H8EVuAHIAHc0MCj
Fby6n5WmlA4q5+t0I++UgHsDkmnocrGEou8P8kJgLmvM2WSBAgMBAAGjUDBOMB0G
A1UdDgQWBBSVR6WBPyFyQr4Mmbz663b7UHEPqDAfBgNVHSMEGDAWgBSVR6WBPyFy
Qr4Mmbz663b7UHEPqDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCj
Pd40HykRHivlIK4aKsdKisB7PsHMORInpwYtBL/6eh978lUp3xePlwHK5VUgsbBt
Pa11++nmnscBUiWSycO3MCwZjnyvZNsFgxZTKpsjzk6Tqh6DvzuGwSozC5ukv+DQ
+uOswyxPqgmf0qjVK9tX52W5sASoN3ekXCErQmiHQ5+1I6n9QOpRF8VR4wlnNFgx
eWEtDXYJ89ptC0pnhouFPYzpYBPkUQPGqFtB/RNEJX+QpLoF6bMftTox2xOq8vvx
nnbX5/JkpFTFIEXMPCRBjEy/ZuuoHyltruduB4RGvf5vWx1GS53iG8g5bbYJF7WM
RK2u3uM3gn7ba/6kb1N+
-----END CERTIFICATE-----
)T";

xnode_manage_contract::xnode_manage_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xnode_manage_contract::setup() {

    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(SELF_ADDRESS(), table_id)) {
        xwarn("[xnode_manage_contract::setup] EXTRACT_TABLE_ID failed, node pid: %d, account: %s", getpid(), SELF_ADDRESS().to_string().c_str());
        return;
    }
    xdbg("[xnode_manage_contract::setup] table id: %d", table_id);

    
    STRING_CREATE(XPROPERTY_NODE_CHECK_KEY);
    STRING_SET(XPROPERTY_NODE_CHECK_KEY, "1");

    STRING_CREATE(XPROPERTY_NODE_CA_CHECK_KEY);
    STRING_SET(XPROPERTY_NODE_CA_CHECK_KEY, "1");

    //init first root ca
    LIST_CREATE(XPROPERTY_NODE_ROOT_CA_KEY);
    LIST_PUSH_BACK(XPROPERTY_NODE_ROOT_CA_KEY, ca_root_conent.c_str());

    MAP_CREATE(XPROPERTY_NODE_INFO_MAP_KEY);

    //todo, read from block
}

void xnode_manage_contract::node_info_reg(std::string const& account_info_str)
{
    xinfo("xnode_manage_contract::node_info_reg ");
    // check account source
    auto const& source_addr = SOURCE_ADDRESS();
    auto const& account = SELF_ADDRESS();

    std::string base_addr = "";
    uint32_t table_id = 0;

    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    xinfo("[xtable_statistic_cons_contract][on_collect_statistic_info_cons] self_account %s, source_addr %s, base_addr %s\n", account.to_string().c_str(),
        source_addr.c_str(), base_addr.c_str());

    //todo 
    //std::vector<std::string> committee_addrs = data::xrootblock_t::get_tcc_initial_committee_addr();
   // XCONTRACT_ENSURE((find(committee_addrs.begin(), committee_addrs.end(), source_addr) != committee_addrs.end()), "invalid source addr's call!");

    XCONTRACT_ENSURE(base_addr == top::sys_contract_rec_node_manage_addr, "invalid source base's call!");

    data::system_contract::xnode_manage_account_info_t account_reg_info;
    base::xstream_t stream { base::xcontext_t::instance(), (uint8_t*)account_info_str.data(), static_cast<uint32_t>(account_info_str.size()) };
    stream >> account_reg_info.account;
    stream >> account_reg_info.cert_info;

    // check if exist
    if (false == node_info_reg_exist(account_reg_info)) {
        if (node_info_reg_check(account_reg_info)) {
            node_info_reg_add(account_reg_info);
        }
    }
}

bool  xnode_manage_contract::node_info_reg_exist(data::system_contract::xnode_manage_account_info_t const & reg_account_info)
{
    std::string value_str;
    if (MAP_GET2(XPROPERTY_NODE_INFO_MAP_KEY, reg_account_info.account, value_str)) {
        xdbg("[xnode_manage_contract::node_info_reg_exist] reg_account not exist, address:%s", reg_account_info.account.c_str());
        return false;
    } else {
       return true;
    }
}

bool  xnode_manage_contract::node_info_reg_check(data::system_contract::xnode_manage_account_info_t const & reg_account_info)
{
    auto node_check_flag = STRING_GET(data::system_contract::XPROPERTY_NODE_CHECK_KEY);
    xdbg("xnode_manage_contract::node_info_reg_check. node_check_flag=%s ", node_check_flag.c_str());
    if (node_check_flag == "0") {
        return true;
    }

    auto node_ca_check_flag = STRING_GET(data::system_contract::XPROPERTY_NODE_CA_CHECK_KEY);
    xdbg("xnode_manage_contract::node_info_reg_check. node_check_flag=%s ", node_ca_check_flag.c_str());
    if (node_check_flag == "1") {

        auto root_ca_list_size = LIST_SIZE(XPROPERTY_NODE_ROOT_CA_KEY);
        xdbg("xnode_manage_contract::node_info_reg_check ca root size:%d", root_ca_list_size);
        if (root_ca_list_size < 1) {
            return false;
        }

        top::ca_auth::xtop_ca_auth ca_auth_context;
        for (int32_t i = 0; i < root_ca_list_size; i++) {
            std::string root_ca_str;
            LIST_POP_FRONT(XPROPERTY_NODE_ROOT_CA_KEY, root_ca_str);
             ca_auth_context.add_root_cert(root_ca_str.c_str());
        }
        ca_auth_context.init_self_cert(reg_account_info.cert_info.c_str(), 1);

        XCONTRACT_ENSURE(1 ==  ca_auth_context.verify_self_cert(), "cert failed!");

    }

    return true;
}

void  xnode_manage_contract::node_info_reg_add(data::system_contract::xnode_manage_account_info_t &reg_account_info)
{
    reg_account_info.reg_time = TIME();
    reg_account_info.validity = true;
  
    top::base::xstream_t _stream(base::xcontext_t::instance());
    reg_account_info.serialize_to(_stream);
    std::string account_info_str((char *)_stream.data(), _stream.size());
    MAP_SET(XPROPERTY_NODE_INFO_MAP_KEY, reg_account_info.account, account_info_str);
    xinfo("xnode_manage_contract::node_info_reg_add. account=%s ", reg_account_info.account.c_str());
}

NS_END4
