//
//  main.cpp
//  xvledger-test
//
//  Created by Taylor Wei on 3/15/20.
//  Copyright © 2021 Taylor Wei. All rights reserved.
//

#include "xbase/xutl.h"
 
int test_xstate(bool is_stress_test);

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
    
    bool is_stress_test = false;
    int test_result = test_xstate(is_stress_test);
    if(test_result != 0)
    {
        printf("test_xstate found error,exit \n");
        return 0;
    }
    
    sleep(2); //let xtestshard finish initialization
    
    bool stop_test = false;
    while(stop_test == false)
    {
        sleep(1);
    }

    sleep(2000);//let all resource quit
}
