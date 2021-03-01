#include <iostream>
#include "xbase/xlog.h"
#include "xbase/xthread.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xutility/xhash.h"
#include "xdata/xrootblock.h"
#include "xconfig/xconfig_register.h"

//#include "gperftools/heap-profiler.h"
//#include "gperftools/profiler.h"

using namespace std;

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

#if 0
void *HeapCheck(void *arg) {
    while (1) {
        HeapProfilerDump("");
        sleep(60);
    }

    return nullptr;
}

void *CpuCheck(void *arg) {

    sleep(35);

    ProfilerStop();

    return nullptr;
}
#endif

extern void test_xtc();

int main(int argc, char **argv) {

    cout << "mock test main run" << endl;
#if 0
#if 0
    HeapProfilerStart("/tmp/heapdump");
    pthread_t tid;
    pthread_create(&tid, NULL, HeapCheck, NULL);
#else
    ProfilerStart("test.prof");
    {
        pthread_t tid;
        pthread_create(&tid, NULL, CpuCheck, NULL);
    }
#endif
#endif

    new xhashtest_t();
    top::data::xrootblock_para_t para;
    top::data::xrootblock_t::init(para);

    auto& config_center = top::config::xconfig_register_t::get_instance();
    config_center.init_static_config();

    xinit_log("./xtc_mock.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    xdbg("------------------------------------------------------------------");
    xinfo("new log start here");

    test_xtc();

    return 0;
}
