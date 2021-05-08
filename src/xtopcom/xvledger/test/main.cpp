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
#include "xtestdb.hpp"

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbase/xhash.h"
#include "xvledger/xvledger.h"
#include "xblockstore/xblockstore_face.h"

class xhashtest_t : public top::base::xhashplugin_t
{
public:
    xhashtest_t()
    :top::base::xhashplugin_t(-1) //-1 = support every hash types
    {
    }
private:
    xhashtest_t(const xhashtest_t &);
    xhashtest_t & operator = (const xhashtest_t &);
    virtual ~xhashtest_t(){};
public:
    virtual const std::string hash(const std::string & input,enum_xhash_type type) override
    {
        return top::base::xstring_utl::tostring(top::base::xhash64_t::digest(input));
    }
};

int test_xstate(bool is_stress_test);
int test_block_builder(bool is_stress_test);

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
    
    new xhashtest_t(); //register this plugin into xbase

    const std::string  default_path = "/";
    top::test::xstoredb_t * global_db_store = new top::test::xstoredb_t(default_path);
    top::base::xvchain_t::instance().set_xdbstore(global_db_store);
 
    top::test::xveventbus_impl * mbus_store = new top::test::xveventbus_impl();
    base::xvchain_t::instance().set_xevmbus(mbus_store);
    
    top::store::get_vblockstore(); //trigger creatr blockstore as well
    
    if(test_xstate(true) < 0)
        return -1;
    
    if(test_block_builder(true) < 0)
        return -2;
    
    printf("finish all test successful \n");
    //const int total_time_to_wait = 20 * 1000; //20 second
    while(1)
    {
        sleep(1);
    }
    return 0;
}
