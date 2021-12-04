#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xbase/xutl.h"
#include "xvledger/xvblock.h"

using namespace std;
using namespace top;

class xhashtest_t : public top::base::xhashplugin_t
{
public:
    xhashtest_t():
        top::base::xhashplugin_t(-1) //-1 = support every hash types
    {
    }
private:
    xhashtest_t(const xhashtest_t &);
    xhashtest_t & operator = (const xhashtest_t &);
    virtual ~xhashtest_t(){};
public:
    virtual const std::string hash(const std::string & input,enum_xhash_type type) override
    {
        return base::xstring_utl::tostring(base::xhash64_t::digest(input));
    }
};

int main(int argc, char **argv) {
    cout << "xmigrate test main run" << endl;
    // printf("Running main() from gtest_main.cc\n");
    new xhashtest_t();
    base::xvblock_t::register_object(base::xcontext_t::instance());

    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xmigrate_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    return RUN_ALL_TESTS();
}
