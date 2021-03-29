#include <gtest/gtest.h>
#include "xbase/xhash.h"
#include "xutility/xhash.h"
#include "xbase/xlog.h"
#include "xdata/xrootblock.h"

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

int
main(int argc, char * argv[]) {
    new xhashtest_t();
    testing::InitGoogleTest(&argc, argv);

    xinit_log("./a.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    top::data::xrootblock_para_t para;
    top::data::xrootblock_t::init(para);

    return RUN_ALL_TESTS();
}

