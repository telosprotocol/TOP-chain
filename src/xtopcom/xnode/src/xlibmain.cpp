// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <stdio.h>
#include <unistd.h>
#include <string>

#include "xbase/xutl.h"
#include "xbase/xbase.h"
// #include "xpbftservice.h"

#ifdef __MAC_PLATFORM__
    #define XEXPORT  extern "C" __attribute__((visibility("default")))
#else
    #define XEXPORT  extern "C"
#endif

#define __DEFAULT_MAIN_LOG_NAME__            "xlog"

#ifdef __WIN_PLATFORM__
    #define __DEFAULT_MAIN_LOG_PATH__        "c://"
#else
    #define __DEFAULT_MAIN_LOG_PATH__        "/tmp"
#endif

XEXPORT int init_component(const char* _sys_process_folder,const char* _config_file, const char* _config_file_extra)
{
#ifdef DEBUG
    int tracking_thread = 1;
    int log_level = enum_xlog_level_debug;
#else //restore to info level when reach stable version
    int tracking_thread = 0;
    int log_level = enum_xlog_level_error;
#endif
    
    if( (NULL == _sys_process_folder) || (NULL == _config_file) )
        return -1; //invalid parameters
    
    const std::string const_sys_process_folder(_sys_process_folder);
    std::string init_log_folder = std::string(__DEFAULT_MAIN_LOG_PATH__);
    if(top::base::xfile_utl::folder_exist((const_sys_process_folder + "/log"))) //check whether dedicated log folder has created or not
    {
        init_log_folder = const_sys_process_folder + "/log";
    }
    const std::string init_log_file = init_log_folder + "/" + top::base::xstring_utl::tostring(getpid()) +  "." + __DEFAULT_MAIN_LOG_NAME__ + ".log";
    xinit_log(init_log_file.c_str(),tracking_thread,true);
    xset_log_level((enum_xlog_level)log_level);
    
    // static top::xchain::xpbft_service_t s_global_service;    
    return 0;
}

XEXPORT int close_component()
{
    return 0;
}
