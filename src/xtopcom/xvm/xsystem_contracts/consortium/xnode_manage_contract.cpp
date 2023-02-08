
#include "xnode_manage_contract.h"

#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xserialization/xserialization.h"
#include "xca_auth/xca_auth.h"
#include "xdata/xrootblock.h"

using namespace top::data::system_contract;
using top::base::xcontext_t;

NS_BEG3(top, xvm, consortium)

#if !defined(XNODE_MANAGE_CONTRACT_MODULE)
#define XNODE_MANAGE_CONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "xNodeManage_"

#define XNODE_CONTRACT XNODE_MANAGE_CONTRACT_MODULE XCONTRACT_PREFIX

// todo
// std::vector<std::string> committee_addrs = data::xrootblock_t::get_tcc_initial_committee_addr();
//  XCONTRACT_ENSURE((find(committee_addrs.begin(), committee_addrs.end(), source_addr) != committee_addrs.end()), "invalid source addr's call!");
// checkt address
#define XNODE_CONTRACT_ADDRESS_CHECK()                                                                                           \
    do {                                                                                                                         \
        auto const& source_addr = SOURCE_ADDRESS();                                                                              \
        auto const& account = SELF_ADDRESS();                                                                                    \
        xinfo("[xnode_manage_contract][] self_account %s, source_addr %s \n", account.to_string().c_str(), source_addr.c_str()); \
        XCONTRACT_ENSURE(account.to_string() == top::sys_contract_rec_node_manage_addr, "invalid source base's call!");          \
        auto root_account = STRING_GET(data::system_contract::XPROPERTY_NODE_ROOT_ACCOUNT_KEY);                                  \
        XCONTRACT_ENSURE(source_addr == root_account, "invalid root_account address base's call!");                              \
    } while (0);

xnode_manage_contract::xnode_manage_contract(common::xnetwork_id_t const& network_id)
    : xbase_t { network_id }
{
}

void xnode_manage_contract::setup()
{
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(SELF_ADDRESS(), table_id)) {
        xwarn("[xnode_manage_contract::setup] EXTRACT_TABLE_ID failed, node pid: %d, account: %s", getpid(), SELF_ADDRESS().to_string().c_str());
        return;
    }
    xdbg("[xnode_manage_contract::setup] table id: %d", table_id);

    MAP_CREATE(XPROPERTY_NODE_CHECK_OPTION_KEY);
    MAP_SET(XPROPERTY_NODE_CHECK_OPTION_KEY, "check_all", "1");
    MAP_SET(XPROPERTY_NODE_CHECK_OPTION_KEY, "check_ca", "1");
    MAP_SET(XPROPERTY_NODE_CHECK_OPTION_KEY, "check_expiry_time", "1");

    STRING_CREATE(XPROPERTY_NODE_ROOT_ACCOUNT_KEY);
    STRING_CREATE(XPROPERTY_NODE_ROOT_CA_KEY);
    auto root_account = data::xrootblock_t::get_extend_data_by_key(EXTEND_ROOT_ACCOUNT_KEY);
    STRING_SET(XPROPERTY_NODE_ROOT_ACCOUNT_KEY, root_account);
    auto root_ca = data::xrootblock_t::get_extend_data_by_key(EXTEND_ROOT_ACCOUNT_CA_KEY);
    STRING_SET(XPROPERTY_NODE_ROOT_CA_KEY, root_ca);

    auto nodes_ca_map_str =  data::xrootblock_t::get_extend_data_by_key(EXTEND_ROOT_SEED_NODES_CA_MAP_KEY);
    MAP_CREATE(XPROPERTY_NODE_INFO_MAP_KEY);
    
    std::map<std::string, std::string> nodes_ca_map;
    base::xstream_t _stream_map(base::xcontext_t::instance(), (uint8_t*)nodes_ca_map_str.data(), (uint32_t)nodes_ca_map_str.size());
    _stream_map >> nodes_ca_map;

    for (auto node : nodes_ca_map) {
        data::system_contract::xnode_manage_account_info_t reg_account_info;
        reg_account_info.reg_time = 0;
        reg_account_info.account = node.first;
        reg_account_info.cert_info = node.second;
        base::xstream_t _stream(base::xcontext_t::instance());
        reg_account_info.serialize_to(_stream);
        std::string account_info_str((char*)_stream.data(), _stream.size());
        if (nodeInfoValidateCaCheck(reg_account_info)) {
            reg_account_info.expiry_time = ULONG_LONG_MAX;
            reg_account_info.cert_time = ULONG_LONG_MAX;
            nodeInfoInsert(reg_account_info);
        }
    }
}

void xnode_manage_contract::nodeInfoReg(std::string const& account_str, uint64_t const expiry_time, std::string const& account_cert)
{
    xinfo("[xnode_manage_contract::nodeInfoReg] account[%s] expiry_time=%ld.", account_str.c_str(), expiry_time);
    XNODE_CONTRACT_ADDRESS_CHECK();
    int64_t check_time = expiry_time;

    XCONTRACT_ENSURE(check_time >= 0, "xnode_manage_contract::nodeInfoReg expiry_time must be unsigned long ");
    std::string out_str;
    data::system_contract::xnode_manage_account_info_t account_reg_info;
    if (false == nodeInfoExist(account_str, out_str)) {
        account_reg_info.account = account_str;
    } else {
        base::xstream_t info_stream { base::xcontext_t::instance(), (uint8_t*)out_str.data(), static_cast<uint32_t>(out_str.size()) };
        account_reg_info.serialize_from(info_stream);
    }
    account_reg_info.cert_info = account_cert;
    account_reg_info.expiry_time = TIME() + expiry_time;
    // todo, del this lcert info in log
    xinfo("[xnode_manage_contract::nodeInfoReg] account[%s] expiry_time[%lu] cert_info[%s]. ",
        account_reg_info.account.c_str(), account_reg_info.expiry_time, account_reg_info.cert_info.c_str());
    if (nodeInfoValidateCheck(account_reg_info)) {
        account_reg_info.cert_time += TIME();
        nodeInfoInsert(account_reg_info);
    } else {
        XCONTRACT_ENSURE(false, "xnode_manage_contract::nodeInfoReg failed");  
    }
}

void xnode_manage_contract::nodeInfoUnreg(std::string const& unreg_account_str)
{
    xinfo("xnode_manage_contract::nodeInfoUnreg account[%s].", unreg_account_str.c_str());
    XNODE_CONTRACT_ADDRESS_CHECK();

    std::string out_str;
    data::system_contract::xnode_manage_account_info_t account_unreg_info;
    if (nodeInfoExist(unreg_account_str, out_str)) {
        base::xstream_t info_stream { base::xcontext_t::instance(), (uint8_t*)out_str.data(), static_cast<uint32_t>(out_str.size()) };
        account_unreg_info.serialize_from(info_stream);
        nodeInfoRemove(account_unreg_info);
    } else {
        XCONTRACT_ENSURE(false, "xnode_manage_contract::nodeInfoUnreg failed");  
    }
}

bool xnode_manage_contract::nodeInfoExist(std::string const& account_str, std::string& out_str)
{
    if (MAP_GET2(XPROPERTY_NODE_INFO_MAP_KEY, account_str, out_str)) {
        xdbg("[xnode_manage_contract::nodeInfoExist] reg_account not exist, address:%s", account_str.c_str());
        return false;
    } else {
        if (out_str.empty()) {
            xwarn("[xnode_manage_contract::nodeInfoExist] account conent is empty, address:%s", account_str.c_str());
            return false;
        }
        return true;
    }
}

bool xnode_manage_contract::nodeInfoValidateCheck(data::system_contract::xnode_manage_account_info_t& reg_account_info)
{
    auto node_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_all");
    xdbg("xnode_manage_contract::nodeInfoValidateCheck. node_check_flag=%s ", node_check_flag.c_str());
    if (node_check_flag == "0") {
        reg_account_info.cert_time = 0;
        return true;
    }

    if (!(nodeInfoExpiryTimeCheck(reg_account_info))) {
        return false;
    }

    if (!nodeInfoValidateCaCheck(reg_account_info)) {
        return false;
    }

    return true;
}

bool xnode_manage_contract::nodeInfoValidateCaCheck(data::system_contract::xnode_manage_account_info_t& reg_account_info)
{
    auto node_ca_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_ca");
    xdbg("xnode_manage_contract::nodeInfoValidateCaCheck. node_check_flag=%s ", node_ca_check_flag.c_str());

    if (node_ca_check_flag != "1") {
        reg_account_info.cert_time = 0;
        return true;
    }

    auto root_ca = STRING_GET(XPROPERTY_NODE_ROOT_CA_KEY);
    if (root_ca.empty()) {
        xdbg("xnode_manage_contract::nodeInfoValidateCaCheck ca root empty()");
        return false;
    }

    top::ca_auth::xtop_ca_auth ca_auth_context;
    ca_auth_context.add_root_cert(root_ca.c_str());
    if (1 == ca_auth_context.verify_leaf_cert(reg_account_info.cert_info.c_str())) {
        if (ca_auth_context.get_cert_expiry_time() > 0) {
            xinfo("xnode_manage_contract::nodeInfoValidateCaCheck account%s ok.", reg_account_info.account.c_str());
            reg_account_info.cert_time = ca_auth_context.get_cert_expiry_time();
            return true;
        }
    }
    xwarn("xnode_manage_contract::nodeInfoValidateCaCheck account%s error.", reg_account_info.account.c_str());
    return false;
}

bool xnode_manage_contract::nodeInfoExpiryTimeCheck(data::system_contract::xnode_manage_account_info_t const& reg_account_info)
{
    auto node_time_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_expiry_time");

    if (node_time_check_flag == "0") {
        return true;
    }

    auto now = TIME();
    if (reg_account_info.expiry_time <= now) {
        xwarn("xnode_manage_contract::nodeInfoExpiryTimeCheck account%s is timeout.", reg_account_info.account.c_str());
        return false;
    }
    return true;
}

void xnode_manage_contract::nodeInfoInsert(data::system_contract::xnode_manage_account_info_t& reg_account_info)
{
    reg_account_info.reg_time = TIME();

    base::xstream_t _stream(base::xcontext_t::instance());
    reg_account_info.serialize_to(_stream);
    std::string account_info_str((char*)_stream.data(), _stream.size());
    MAP_SET(XPROPERTY_NODE_INFO_MAP_KEY, reg_account_info.account, account_info_str);
    xinfo("xnode_manage_contract::nodeInfoInsert. account=%s ", reg_account_info.account.c_str());
}

void xnode_manage_contract::nodeInfoRemove(data::system_contract::xnode_manage_account_info_t& reg_account_info)
{
    MAP_REMOVE(XPROPERTY_NODE_INFO_MAP_KEY, reg_account_info.account);
    xinfo("xnode_manage_contract::nodeInfoRemove. account=%s ", reg_account_info.account.c_str());
}

void xnode_manage_contract::nodeInfoAuthConfig(std::string const& check_type, std::string const& check_flag)
{
    xinfo("xnode_manage_contract::nodeInfoAuthConfig check_type[%s] value[%s]", check_type.c_str(), check_flag.c_str());
    XNODE_CONTRACT_ADDRESS_CHECK();

    std::string read_check_flag;
    try {
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, check_type)) {
            read_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, check_type);
            if (read_check_flag != check_flag) {
                MAP_SET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, check_type, check_flag);
            }
            return;
        }
    } catch (std::runtime_error const& e) {
        xwarn("[xnode_manage_contract][nodeInfoAuthConfig] read read_check_flag error:%s", e.what());
        throw;
    }

    xwarn("xnode_manage_contract::nodeInfoAuthConfig check type  %s check_flag %s error.", check_type.c_str(), check_flag.c_str());
}

void xnode_manage_contract::nodeInfoRootCaReplace(std::string const& root_account, std::string const& root_ca)
{
    xinfo("xnode_manage_contract::nodeInfoRootCaReplace ");
    XNODE_CONTRACT_ADDRESS_CHECK();

    STRING_SET(XPROPERTY_NODE_ROOT_ACCOUNT_KEY, root_account);
    STRING_SET(XPROPERTY_NODE_ROOT_CA_KEY, root_ca);

    xinfo("xnode_manage_contract::nodeInfoRootCaReplace root_account %s root_ca %s replace.", root_account.c_str(), root_ca.c_str());
}

void xnode_manage_contract::nodeInfoCaRebase()
{

    XMETRICS_TIME_RECORD(XNODE_CONTRACT "nodeInfoCaRebase_all_time");
    XMETRICS_CPU_TIME_RECORD(XNODE_CONTRACT "nodeInfoCaRebase_cpu_time");

    auto node_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_all");
    xdbg("xnode_manage_contract::nodeInfoCaRebase. node_check_flag=%s ", node_check_flag.c_str());
    if (node_check_flag != "1") {
        return;
    }

    auto node_ca_check_flag = MAP_GET(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_ca");
    xdbg("xnode_manage_contract::nodeInfoCaRebase. check_ca=%s ", node_ca_check_flag.c_str());
    if (node_ca_check_flag != "1") {
        return;
    }

    std::map<std::string, std::string> map_nodes;
    XMETRICS_TIME_RECORD(XNODE_CONTRACT "XPROPERTY_NODE_INFO_MAP_KEY");
    MAP_COPY_GET(data::system_contract::XPROPERTY_NODE_INFO_MAP_KEY, map_nodes);

    for (auto const& it : map_nodes) {
        data::system_contract::xnode_manage_account_info_t reg_node_info;
        base::xstream_t stream(xcontext_t::instance(), (uint8_t*)it.second.c_str(), it.second.size());
        reg_node_info.serialize_from(stream);

        if (!nodeInfoValidateCaCheck(reg_node_info)) {
            nodeInfoRemove(reg_node_info);
            xinfo("xnode_manage_contract::nodeInfoCaRebase del account %s later.", reg_node_info.account.c_str());
        }
    }
}


NS_END3
