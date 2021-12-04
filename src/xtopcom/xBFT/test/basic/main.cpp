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

#include "xblockstore/xblockstore_face.h"
#include "xmigrate/xvmigrate.h"
//#include "xmigrate/src/xdbmigrate.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

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

#define SIGTERM_MSG "SIGTERM received.\n"

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
        
            if(signum == SIGUSR1)
            {
                int * ptr = 0;
                *ptr = 6; //triggger crash ->generate core dump
            }
            else
            {
                //trigger save data
                top::base::xvchain_t::instance().save_all();
            }
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
    static struct sigaction _sys_sigact;
    memset(&_sys_sigact, 0, sizeof(_sys_sigact));
    
    _sys_sigact.sa_sigaction = on_sys_signal_callback;
    _sys_sigact.sa_flags = SA_SIGINFO;
 
    //config signal of termine
    sigaction(SIGTERM, &_sys_sigact, NULL);
    signal(SIGINT, SIG_IGN); //disable INT signal
    signal(SIGHUP, SIG_IGN); //disable HUP signal
    
    //config signal of cores
    sigaction(SIGSEGV, &_sys_sigact, NULL);
    sigaction(SIGILL, &_sys_sigact, NULL);
    sigaction(SIGFPE, &_sys_sigact, NULL);
    sigaction(SIGABRT, &_sys_sigact, NULL);
    
    //config user 'signal
    sigaction(SIGUSR1, &_sys_sigact, NULL);
    sigaction(SIGUSR2, &_sys_sigact, NULL);
}
 
//#define __network_simulator_test__
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
    
    top::base::xvconfig_t* sys_config_ptr = new top::base::xvconfig_t();
    //configure bootstrap
    sys_config_ptr->set_config("system.version", "0.0.0.1");
    sys_config_ptr->set_config("system.boot.size", "1");
    //configure db migrate as bootstrap
    sys_config_ptr->set_config("system.boot.0.object_key", "/init/migrate/db" );
    sys_config_ptr->set_config("system.boot.0.object_version","0.0.0.1");
    //configu db filter options
    
    sys_config_ptr->set_config("/init/migrate/db/src_path", "/private/tmp/xdb" );
    sys_config_ptr->set_config("/init/migrate/db/dst_path", "/private/tmp/xdb2" );
    sys_config_ptr->set_config("/init/migrate/db/size", "3" );
    sys_config_ptr->set_config("/init/migrate/db/0/object_key","/init/migrate/db/kvfilter");
    sys_config_ptr->set_config("/init/migrate/db/1/object_key","/init/migrate/db/blkfilter");
    sys_config_ptr->set_config("/init/migrate/db/2/object_key","/init/migrate/db/txsfilter");
    top::base::init_migrate();
    
    top::base::xsysobject_t * init_module = top::base::xvsyslibrary::instance(). create_object(top::base::xvsysinit_t::get_register_key());
    if(init_module == nullptr)
    {
        xassert(0);
        return -1;
    }
    if(init_module->init(*sys_config_ptr) != enum_xcode_successful)
    {
        xassert(0);
        return -2;
    }
    init_module->start();
    
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
 
    top::store::enable_block_recycler(true);
    top::store::install_block_recycler(NULL);
    top::base::xvchain_t::instance().enable_auto_prune(true);
    
    catch_system_signals();
    sleep(2); //let xtestshard finish initialization

    base::xvaccount_t test_account_obj(test_account_address);
    base::xvactmeta_t acct_met_out(test_account_obj);
    base::xvactmeta_t acct_met_in(test_account_obj);
    
    base::xautostream_t<1024> _stream(base::xcontext_t::instance());
    int writed_bytes = acct_met_out.serialize_to(_stream);
    int read_bytes = acct_met_in.serialize_from(_stream);
    xassert(writed_bytes == read_bytes);
    
    _stream.reset();
    base::xvbindex_t vindx_in;
    base::xvbindex_t vbindx_out;
    vbindx_out.reset_account_addr(test_account_obj);
    
    writed_bytes = vbindx_out.serialize_to(_stream);
    read_bytes = vindx_in.serialize_from(_stream);
    vindx_in.reset_account_addr(test_account_obj);
    xassert(writed_bytes == read_bytes);
    
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
