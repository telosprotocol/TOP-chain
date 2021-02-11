#include <gtest/gtest.h>
#include "xbase/xlog.h"

int main(int argc, char * argv[]) {
    xinit_log("./xdata_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xinfo("------------------------------------------------------------------");
    xinfo("new log start here");

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
