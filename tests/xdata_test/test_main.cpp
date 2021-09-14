#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"

using namespace std;
using namespace top;

int main(int argc, char **argv) {
    cout << "xdata test main run" << endl;

    XMETRICS_INIT();

    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xdata_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    return RUN_ALL_TESTS();
}
