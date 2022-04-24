#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"

#include "tests/xevm_engine_test/evm_test_fixture/xtest_evm_fixture.h"

int evm_tests_argc;
char ** evm_tests_argv;


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
        auto hash = top::utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char*>(hash.data()), hash.size());
    }
};


int main(int argc, char **argv) {
    cout << "xevm test main run" << endl;

    XMETRICS_INIT();

    testing::InitGoogleTest(&argc, argv);
    evm_tests_argc = argc;
    evm_tests_argv = argv;

    xinit_log("./xevm_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    new xhashtest_t();
    top::data::xrootblock_para_t para;
    top::data::xrootblock_t::init(para);

    return RUN_ALL_TESTS();
}
