#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xstore/xstore.h"
//#include "xlog2.h"

using namespace std;

int main(int argc, char **argv) {
    cout << "signer test main run" << endl;
    // printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xsigner.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    return RUN_ALL_TESTS();
}