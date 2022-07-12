#pragma once

#ifdef LEAK_TRACER
#include <csignal>

#include "leaktracer/MemoryTrace.hpp"
#endif

#ifdef ENABLE_GPERF
#include <csignal>

#include "gperftools/profiler.h"
#endif

#ifdef ENABLE_GHPERF
#include "gperftools/heap-profiler.h"
#endif

#ifdef ENABLE_TCMALLOC
#include "gperftools/malloc_extension.h"
#endif

namespace top
{

#ifdef LEAK_TRACER
    void setup_leak();
#endif

#ifdef ENABLE_GPERF
    void setup_gpref();
#endif

#ifdef ENABLE_GHPERF
    void setup_ghperf();
#endif

#ifdef ENABLE_TCMALLOC
    void setup_tcmalloc();
#endif

    void setup_options();
} // namespace top
