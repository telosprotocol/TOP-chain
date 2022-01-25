#include "xbase/xhash.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"

#include <gtest/gtest.h>
#include <chrono>

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

int main(int argc, char * argv[]) {
    new xhashtest_t();
    top::data::xrootblock_para_t para;
    para.m_tcc_accounts = {"T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB", "T00000LNEZSwcYJk6w8zWbR78Nhw8gbT2X944CBy", "T00000LfVA4mibYtKsGqGpGRxf8VZYHmdwriuZNo"};
    top::data::xrootblock_t::init(para);

    testing::InitGoogleTest(&argc, argv);
    xinit_log("./xcheckpoint_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);

    XMETRICS_INIT();
    auto result = RUN_ALL_TESTS();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return result;
}
