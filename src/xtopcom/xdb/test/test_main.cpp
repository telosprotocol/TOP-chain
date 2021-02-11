#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
//#include "xlog2.h"

using namespace std;
int main(int argc, char **argv) {
    cout << "xdb test main run" << endl;
    // printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xdb_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xinfo("------------------------------------------------------------------");
    xinfo("new log start here");

    return RUN_ALL_TESTS();
}