//
//  main.cpp
//  xBFT-basic-test
//
#include "xbase/xhash.h"
#include "xbase/xutl.h"
#include "xbase/xthread.h"
#include "xBFT/xconspdu.h"
#include "xBFT/xconsobj.h"
#include "xBFT/xconsengine.h"
#include "xBFT/xconsaccount.h"
#include "xutility/xhash.h"
#include "xunitblock.hpp"
#include "xtestshard.hpp"

#include <limits.h>

using namespace top;
using namespace top::test;

namespace top
{
    class xhashtest_t : public base::xhashplugin_t
    {
    public:
        xhashtest_t()
            :base::xhashplugin_t(-1) //-1 = support every hash types
        {
        }
    private:
        xhashtest_t(const xhashtest_t &);
        xhashtest_t & operator = (const xhashtest_t &);
        virtual ~xhashtest_t(){};
    public:
        virtual const std::string hash(const std::string & input,enum_xhash_type type) override
        {
            const uint256_t hash_to_sign = utl::xsha2_256_t::digest(input);
            return std::string((const char*)hash_to_sign.data(),hash_to_sign.size());
            //return base::xstring_utl::tostring(base::xhash64_t::digest(input));
        }
    };

}

#define __network_simulator_test__
//#define __network_outoforder_test__
int main(int argc, const char * argv[])
{
#ifdef __WIN_PLATFORM__
    xinit_log("C:\\Users\\taylo\\Downloads\\", true, true);
#else
    xinit_log("/tmp/",true,true);
#endif
    
#ifdef DEBUG
    xset_log_level(enum_xlog_level_debug);
    xdup_trace_to_terminal(true);
#else
    //xset_log_level(enum_xlog_level_debug);
    xset_log_level(enum_xlog_level_key_info);
#endif
    
    const std::string account_publick_addr = "1234567890abcdef";
    const std::string test_account_address =  top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account, 0, account_publick_addr);
    
    std::string test_account_txs = "txs: ";
    const uint32_t random_len = top::base::xtime_utl::get_fast_randomu() % 1024;
    for(int j = 0; j < random_len; ++j) //avg 512 bytes per packet
    {
        uint8_t random_seed1 = (uint8_t)(top::base::xtime_utl::get_fast_randomu() % 120);
        if(random_seed1 < 33)
            random_seed1 += 33;
        test_account_txs.push_back(random_seed1);
    }
    
    new top::xhashtest_t(); //register this plugin into xbase
    
    //type and param
    std::multimap<uint32_t,std::string> nodes_list;
 
#ifdef __network_simulator_test__
    
    #ifdef __network_outoforder_test__
            
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest  | enum_net_outoforder_type_extreamhigh,std::string()));
    
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest  | enum_net_outoforder_type_small,std::string()));
        
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest  | enum_net_outoforder_type_high,std::string()));
        
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));

    
    #elif defined(__network_lossrate_test__)
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest | enum_net_lossrate_type_small | enum_net_outoforder_type_small,std::string()));
        
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest | enum_net_lossrate_type_high | enum_net_outoforder_type_high,std::string()));
        
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest | enum_net_lossrate_type_extreamhigh |enum_net_outoforder_type_extreamhigh,std::string()));
    #else
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest | enum_net_lossrate_type_small | enum_net_outoforder_type_small,std::string()));
        
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest | enum_net_lossrate_type_high | enum_net_outoforder_type_high,std::string()));
        
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
        nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest | enum_net_lossrate_type_extreamhigh |enum_net_outoforder_type_extreamhigh,std::string()));
    #endif
   
#else
    nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
    nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
    nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
    nodes_list.insert(std::multimap<uint32_t,std::string>::value_type(enum_xtestnode_role_honest,std::string()));
#endif
    
    //global shared runtime
    base::xworkerpool_t_impl<1> *global_worker_pool = new base::xworkerpool_t_impl<1>(top::base::xcontext_t::instance());
    xtestshard * test_shard = new xtestshard(*global_worker_pool,nodes_list);
 
    sleep(2); //let xtestshard finish initialization

    int total_txs = 0;
    bool stop_test = false;
    while(stop_test == false)
    {
        //deliver one tx per 2 seconds
        ++total_txs;
        test_shard->on_txs_recv(test_account_address,test_account_txs);
        
        if(total_txs > 1000)
            sleep(100);
        if((total_txs % 100) == 99)
            sleep(10);
        else
            sleep(1);
    }
    test_shard->release_ref();
    sleep(2000);//let all resource quit
    
    return 0;
}
