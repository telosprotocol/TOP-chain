#include <assert.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "xchaininit/xinit.h"
#include "xpbase/base/top_utils.h"
#include "xchaininit/version.h"
#include "xdata/xoperation_config.h"
#include "xvledger/xvledger.h"

#include "xmetrics/xmetrics.h"
// #include "tcmalloc_options.h"

using namespace top;

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
            
            //trigger save data
            top::base::xvchain_t::instance().save_all();
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
    
#ifndef DISABLE_CORE_SIGNAL_CAPTURE
#ifndef DEBUG
    //config signal of cores
    sigaction(SIGSEGV, &_sys_sigact, NULL);
    sigaction(SIGILL, &_sys_sigact, NULL);
    sigaction(SIGFPE, &_sys_sigact, NULL);
    sigaction(SIGABRT, &_sys_sigact, NULL);
#endif
#endif
    
    //config user 'signal
    sigaction(SIGUSR1, &_sys_sigact, NULL);
    sigaction(SIGUSR2, &_sys_sigact, NULL);
}

void ntp_thread_proc() {
    xkinfo("ntp monitor thread running");
    // common ntp servers;
    std::vector<std::string> xvec_ntp_host;
    xvec_ntp_host.push_back(std::string("north-america.pool.ntp.org"));
    xvec_ntp_host.push_back(std::string("us.pool.ntp.org"           ));
    xvec_ntp_host.push_back(std::string("time.nist.gov"             ));
    xvec_ntp_host.push_back(std::string("time-nw.nist.gov"          ));
    xvec_ntp_host.push_back(std::string("europe.pool.ntp.org"       ));
/*
    xvec_ntp_host.push_back(std::string("1.cn.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("2.cn.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("3.cn.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("0.cn.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("cn.pool.ntp.org"           ));
    xvec_ntp_host.push_back(std::string("tw.pool.ntp.org"           ));
    xvec_ntp_host.push_back(std::string("0.tw.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("1.tw.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("2.tw.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("3.tw.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("pool.ntp.org"              ));
    xvec_ntp_host.push_back(std::string("time.windows.com"          ));
    xvec_ntp_host.push_back(std::string("asia.pool.ntp.org"         ));
    xvec_ntp_host.push_back(std::string("oceania.pool.ntp.org"      ));
    xvec_ntp_host.push_back(std::string("south-america.pool.ntp.org"));
    xvec_ntp_host.push_back(std::string("africa.pool.ntp.org"       ));
    xvec_ntp_host.push_back(std::string("ca.pool.ntp.org"           ));
    xvec_ntp_host.push_back(std::string("uk.pool.ntp.org"           ));
    xvec_ntp_host.push_back(std::string("au.pool.ntp.org"           ));
*/

    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(64));
        top::ntp_sync(xvec_ntp_host);
    }
}

int start_monitor_thread() {
#ifdef ENABLE_NTP
    // std::thread(ntp_thread_proc).detach();
#endif
    return 0;
}

int main(int argc, char * argv[]) {
#if 1  // TODO(jimmy) for debug
    catch_system_signals();//setup and hook system signals
#endif
    // use this to join or quit network, config show_cmd = false will come here
    // auto elect_mgr = elect_main.elect_manager();
    try {
        if (argc < 2) {
            std::cout << "usage: xtopchain config_file1 [config_file2]" << std::endl;
            _Exit(0);
        }

        if (strcmp(argv[1], "-v") == 0) {
            print_version();
            _Exit(0);
        } else {
            print_version();
            std::string config_file1 = argv[1];
            std::string config_file2;
            if (argc > 2) {
                config_file2 = argv[2];
            }
            start_monitor_thread();
            topchain_init(config_file1, config_file2);
        }
    } catch (const std::runtime_error& e) {
        xerror("Unhandled runtime_error Exception reached the top of main %s", e.what());
    } catch (const std::exception& e) {
        xerror("Unhandled Exception reached the top of main %s", e.what());
    } catch (...) {
        xerror("unknow exception");
    }
    return 0;
}
