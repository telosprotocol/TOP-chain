#include <iostream>
#include <gtest/gtest.h>
#include "xbase/xlog.h"
#include "xbase/xhash.h"
#include "xblockstore/xblockstore_face.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"
#include "xloader/xconfig_genesis_loader.h"

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
    cout << "xblockstore test main run" << endl;
    // printf("Running main() from gtest_main.cc\n");
    new xhashtest_t();

    auto genesis_loader = std::make_shared<top::loader::xconfig_genesis_loader_t>(std::string{});
    top::data::xrootblock_para_t rootblock_para;
    genesis_loader->extract_genesis_para(rootblock_para);
    top::data::xrootblock_t::init(rootblock_para);

    XMETRICS_INIT();
    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);

    testing::InitGoogleTest(&argc, argv);

    xinit_log("./xblockstore_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    return RUN_ALL_TESTS();
}
