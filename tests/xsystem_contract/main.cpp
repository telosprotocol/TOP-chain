#include "xbase/xhash.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"

#include <gtest/gtest.h>
#include <chrono>

using namespace top;

int main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    xinit_log("./xsystem_contract_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);

    XMETRICS_INIT();
    assert(RUN_ALL_TESTS() == 0);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
