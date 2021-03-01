// #include <gtest/gtest.h>
// #include "xchaininit/xinit.h"
#include "xbase/xlog.h"
#include <memory>
#include "xpbftservice.h"
#include "vhost_mock.hpp"
// #include "xelect/xelect_face.h"

// int main(int argc, char * argv[]) {
//     testing::InitGoogleTest(&argc, argv);
//     xinit_log("/tmp/xelect-test.log", true, true);
//     xset_log_level(enum_xlog_level_debug);
//     return RUN_ALL_TESTS();
// }

using namespace top::xchain;

int main(int argc, char * argv[]) {
    top::vnetwork::xvhost_face_t * phost = new vhost_mock();
    xvnetwork_test * pnet = new xvnetwork_test();
}