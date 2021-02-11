#pragma once

#include <string>
#include <thread>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include "gperftools/heap-profiler.h"
// #include "gperftools/malloc_extension.h"

#ifdef ENABLE_GPERF
#define USING_GPERF_HEAP
#endif  // ENABLE_GPERF

namespace top {
    void DoHeapCheck() {
#ifdef USING_GPERF_HEAP
        // for (;;) {
            // if (::IsHeapProfilerRunning()) {
            //     ::HeapProfilerDump("");
            //     MallocExtension::instance()->ReleaseFreeMemory();
            // }
            // ::HeapProfilerDump("");
            // std::this_thread::sleep_for(std::chrono::minutes(20));
        // }
#endif
    }

    void StartCheckHeap() {
#ifdef ENABLE_TCMALLOC
        // MallocExtension::instance()->SetMemoryReleaseRate(7.0);
#endif

#ifdef USING_GPERF_HEAP
        // MallocExtension::instance()->SetMemoryReleaseRate(7.0);
        // static const int pid = ::getpid();
    #ifdef USING_CHAIN_LOG_PLOG
        // static const std::string pid_str = "/chain/log/plog/heap_" + std::to_string(pid);
    #else
        // static const std::string pid_str = "/tmp/heap_" + std::to_string(pid);
    #endif
        // ::HeapProfilerStart(pid_str.c_str());
        // std::cout << "==== StartCheckHeap ====" << std::endl;
        // std::thread(top::DoHeapCheck).detach();
#endif
    }
}  // namespace top


