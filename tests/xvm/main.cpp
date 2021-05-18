#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xcontract/xdemo_param.h"
#include "xcontract/xdemo.h"
#include "xvm/manager/xcontract_manager.h"
#include "xdata/xrootblock.h"

class xhashtest_t : public top::base::xhashplugin_t
{
public:
    xhashtest_t():
        top::base::xhashplugin_t(-1) //-1 = support every hash types
    {
    }
private:
    xhashtest_t(const xhashtest_t &);
    xhashtest_t & operator = (const xhashtest_t &);
    virtual ~xhashtest_t(){};
public:
    virtual const std::string hash(const std::string & input,enum_xhash_type type) override
    {
        auto hash = top::utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char*>(hash.data()), hash.size());
    }
};

int main(int argc, char * argv[])
{
    using namespace top::xvm::xcontract;

    new xhashtest_t();
    top::data::xrootblock_para_t para;
    top::data::xrootblock_t::init(para);

    top::contract::xcontract_manager_t::instance().register_contract<hello_param>(top::common::xaccount_address_t{"T-400"}, top::common::xtopchain_network_id);
    top::contract::xcontract_manager_t::instance().register_contract_cluster_address(top::common::xaccount_address_t{"T-400"}, top::common::xaccount_address_t{"T-400"});
    top::contract::xcontract_manager_t::instance().register_contract_cluster_address(top::common::xaccount_address_t{"T-200"}, top::common::xaccount_address_t{"T-200"});
    top::contract::xcontract_manager_t::instance().register_contract_cluster_address(top::common::xaccount_address_t{"T-123456789012345678901234567890125"}, top::common::xaccount_address_t{"T-123456789012345678901234567890125"});
    top::contract::xcontract_manager_t::instance().register_contract_cluster_address(top::common::xaccount_address_t{"T-x-topsystemtimer"}, top::common::xaccount_address_t{"T-x-topsystemtimer"});
    testing::InitGoogleTest(&argc, argv);
    xinit_log("./xvm_test.log", true, true);
    xset_log_level((enum_xlog_level)0);
    return RUN_ALL_TESTS();
}
