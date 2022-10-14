#include "xbase/xhash.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"

#include <gtest/gtest.h>

#include <chrono>

using namespace top;

int main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    xinit_log("./xquic_test.log", true, true);
    xset_log_level(enum_xlog_level_info);

    XMETRICS_INIT();
    auto result = RUN_ALL_TESTS();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return result;
}
