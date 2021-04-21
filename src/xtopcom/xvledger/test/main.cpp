//
//  main.cpp
//  xvledger-test
//
//  Created by Taylor Wei on 3/20/20.
//  Copyright © 2021 Taylor Wei. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "xbase/xlog.h"
#include "xbase/xutl.h"

int test_xstate(bool is_stress_test);

using namespace top;
int main(int argc,char* argv[])
{
#ifdef __WIN_PLATFORM__
    xinit_log("C:\\Users\\taylo\\Downloads\\", true, true);
#else
    xinit_log("/tmp/",true,true);
#endif
    xset_log_level(enum_xlog_level_debug);
    xdup_trace_to_terminal(true);
    
    xset_trace_lines_per_file(1000);
   
    if(test_xstate(true) < 0)
        return -1;
    
    printf("finish all test successful \n");
    //const int total_time_to_wait = 20 * 1000; //20 second
    while(1)
    {
        sleep(1);
    }
    return 0;
}
