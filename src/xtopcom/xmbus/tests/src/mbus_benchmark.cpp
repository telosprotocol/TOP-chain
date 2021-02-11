#include <inttypes.h>
#include <gtest/gtest.h>
#include "xmbus/xmessage_bus.h"

using namespace top::mbus;

TEST(xmessage_bus_t, benchmark) {
    xmessage_bus_t bus;

    std::vector<xevent_ptr_t> event_list;
    std::vector<std::thread> thread_list;

#define TOTAL_TIMERS  1000000

    for(int i=0;i<4;i++) {
        thread_list.clear();
        event_list.push_back(std::make_shared<xevent_t>((xevent_major_type_t) 1));
        bus.add_listener(1, [&](const xevent_ptr_t& e) {});

        auto begin = std::chrono::high_resolution_clock::now();
        for(int j=0;j<=i;j++) {
            thread_list.push_back(std::thread([&](const xevent_ptr_t& e) {
                for(int k=0;k<TOTAL_TIMERS;k++) {
                    bus.push_event(e);
                }
            }, event_list[j]));
        }
        for(auto& t : thread_list) {
            t.join();
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto total = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        printf("cost %" PRIu64 "  ms, cps %.4f, calls per thread %d, threads %d, listeners %d\n", total, 1000. * TOTAL_TIMERS / total, TOTAL_TIMERS, i+1, i+1);
    }
}
