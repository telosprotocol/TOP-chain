#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"

using namespace std;

int main(int argc, char **argv) {
    // printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xpbase.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    return RUN_ALL_TESTS();
}