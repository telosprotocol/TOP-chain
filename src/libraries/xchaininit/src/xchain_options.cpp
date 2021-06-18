#include "xchaininit/xchain_options.h"

namespace top
{
#ifdef LEAK_TRACER
    void setup_leak()
    {
        std::signal(SIGUSR1, export_mem_trace);
        leaktracer::MemoryTrace::GetInstance().startMonitoringAllThreads();
    }

    void export_mem_trace(int signal)
    {
        leaktracer::MemoryTrace::GetInstance().stopMonitoringAllocations();
        leaktracer::MemoryTrace::GetInstance().stopAllMonitoring();

        std::ofstream oleaks;

        oleaks.open(global_node_id + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "leaks.out", std::ios_base::out);
        if (oleaks.is_open())
            leaktracer::MemoryTrace::GetInstance().writeLeaks(oleaks);
        else
            std::cerr << "Failed to write to \"leaks.out\"\n";

        oleaks.close();
    }
#endif

#ifdef ENABLE_GPERF
    void setGperfStatus(int signum)
    {
        static bool is_open = false;
        if (signum != SIGUSR2)
        {
            return;
        }
        if (!is_open)
        { // start
            is_open = true;
            ProfilerStart(std::string{"gperf_" + global_node_id + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".prof"}.c_str());
            xkinfo("ProfilerStart success");
            std::cout << "ProfilerStart success" << std::endl;
        }
        else
        { // stop
            is_open = false;
            ProfilerStop();
            xkinfo("ProfilerStop success");
            std::cout << "ProfilerStop success" << std::endl;
        }
    }

    void setup_gpref()
    {
        std::signal(SIGUSR2, setGperfStatus);
        std::cout << "———— ENABLE_GPERF ————" << std::endl;
    }
#endif

#ifdef ENABLE_GHPERF
    void do_heap_check()
    {
        for (;;)
        {
            if (::IsHeapProfilerRunning())
            {
                ::HeapProfilerDump("");
                // MallocExtension::instance()->ReleaseFreeMemory();
            }
            ::HeapProfilerDump("");
            std::this_thread::sleep_for(std::chrono::minutes(30));
        }
    }

    void setup_ghperf()
    {
        static const int pid = ::getpid();
#ifdef USING_CHAIN_LOG_PLOG
        static const std::string pid_str = "/chain/log/plog/heap_" + std::to_string(pid);
#else
        static const std::string pid_str = "/tmp/heap_" + std::to_string(pid);
#endif
        ::HeapProfilerStart(pid_str.c_str());
        std::cout << "==== StartCheckHeap ====" << std::endl;
        std::thread(top::do_heap_check).detach();
    }
#endif

#ifdef ENABLE_TCMALLOC
    void tc_malloc_free()
    {
        for (;;)
        {
            MallocExtension::instance()->ReleaseFreeMemory();
            std::this_thread::sleep_for(std::chrono::minutes(30));
        }
    }

    void setup_tcmalloc()
    {
        MallocExtension::instance()->SetMemoryReleaseRate(6);
        std::thread(top::tc_malloc_free).detach();
    }
#endif

    void setup_options()
    {
#ifdef LEAK_TRACER
        setup_leak();
#endif

#ifdef ENABLE_GPERF
        setup_gpref();
#endif

#ifdef ENABLE_GHPERF
        setup_ghperf();
#endif

#ifdef ENABLE_TCMALLOC
        setup_tcmalloc();
#endif
    }
} // namespace top