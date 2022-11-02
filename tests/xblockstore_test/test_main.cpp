#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xblockstore/xblockstore_face.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"
#include "xloader/xconfig_genesis_loader.h"
#include "test_common.hpp"

using namespace std;
using namespace top;

uint64_t xhashtest_t::hash_calc_count;
bool   xhashtest_t::print_hash_calc;
int main(int argc, char **argv) {
    cout << "xblockstore test main run" << endl;
    // printf("Running main() from gtest_main.cc\n");
    new xhashtest_t();

    auto genesis_loader = std::make_shared<top::loader::xconfig_genesis_loader_t>(std::string{});
    top::data::xrootblock_para_t rootblock_para;
    genesis_loader->extract_genesis_para(rootblock_para);
    top::data::xrootblock_t::init(rootblock_para);

    XMETRICS_INIT();
    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);

    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xblockstore_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    auto ret = RUN_ALL_TESTS();
    sleep(15);  // for xbase exit double free abnormal issue
    return ret;
}
