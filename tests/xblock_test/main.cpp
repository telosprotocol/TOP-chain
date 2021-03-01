#include <gtest/gtest.h>
#include <memory>
#include "xbase/xlog.h"

#include "xbase/xhash.h"
#include "xbase/xthread.h"
#include "xBFT/xconspdu.h"
#include "xBFT/xconsobj.h"
#include "xBFT/xconsaccount.h"
//#include "xBFT/xconsnode.h"
#include "xstore/xstore_face.h"


using namespace top;

namespace top
{
    class xhashtest_t : public base::xhashplugin_t
    {
    public:
        xhashtest_t()
            :base::xhashplugin_t(-1) //-1 = support every hash types
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

}

int main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    xinit_log("/tmp/xblock_test.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    new top::xhashtest_t(); //register this plugin into xbase
    return RUN_ALL_TESTS();
}
