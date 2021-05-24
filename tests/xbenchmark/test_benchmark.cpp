#include "benchmark/benchmark.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"

#include <string>

static void BM_using_tostring(benchmark::State& state) {
    for (auto _: state) {
        int height = 100000;
        std::string value = std::to_string(height);
    }
}

static void BM_using_xstream(benchmark::State& state) {
    for (auto _: state) {
        int height = 100000;
        top::base::xstream_t  stream(top::base::xcontext_t::instance());
        stream << height;
        std::string value{(char*)stream.data(), stream.size()};
    }
}

BENCHMARK(BM_using_tostring);
BENCHMARK(BM_using_xstream);
BENCHMARK_MAIN();