#include "xbase/xhash.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xmetrics/xmetrics.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"

#include <gtest/gtest.h>

using namespace top;

class xhashtest_t : public base::xhashplugin_t {
public:
    xhashtest_t()
      : base::xhashplugin_t(-1)  //-1 = support every hash types
    {}

private:
    xhashtest_t(const xhashtest_t &);
    xhashtest_t & operator=(const xhashtest_t &);
    virtual ~xhashtest_t(){};

public:
    virtual const std::string hash(const std::string & input, enum_xhash_type type) override {
        // std::cout << "input:" << input << std::endl;
        //std::cout << "calc hash type:" << type << std::endl;
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char *>(hash.data()), hash.size());
        // return base::xstring_utl::tostring(base::xhash64_t::digest(input));
    }
};

int main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    xinit_log("./xstatistic_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    new xhashtest_t();

    data::xrootblock_para_t para;
    data::xrootblock_t::init(para);

    XMETRICS_INIT();
    return RUN_ALL_TESTS();
}
