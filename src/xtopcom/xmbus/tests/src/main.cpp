#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include "xmbus/xmessage_bus.h"
#include "xmbus/xevent_common.h"

int
main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(message_bus, tests) {
    top::mbus::xmessage_bus_t mb;

    int listener1_receives = 0, listener2_receives = 0;
    int sourcer1_receives = 0, sourcer2_receives = 0;

    top::mbus::xevent_queue_cb_t listener1 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        std::static_pointer_cast<top::mbus::xevent_store_t>(e);
                // count no direction
                ++listener1_receives;
            };

    top::mbus::xevent_queue_cb_t listener2 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        std::static_pointer_cast<top::mbus::xevent_store_t>(e);
                if (ptr->direction == top::mbus::xevent_t::to_listener) {
                    ++listener2_receives;
                }
            };

    top::mbus::xevent_queue_cb_t sourcer1 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        std::static_pointer_cast<top::mbus::xevent_store_t>(e);
                if (ptr->direction == top::mbus::xevent_t::to_sourcer) {
                    ++sourcer1_receives;
                    ptr->owner = "store replid";
                }
            };

    top::mbus::xevent_queue_cb_t sourcer2 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        std::static_pointer_cast<top::mbus::xevent_store_t>(e);
                ++sourcer2_receives;
                ptr->owner = "store replid";
            };

    top::mbus::xevent_store_ptr_t ptr = std::make_shared<top::mbus::xevent_store_t>(
            top::mbus::xevent_store_t::type_block_to_db,
            "owner",
            top::mbus::xevent_t::to_listener);
    ptr->err = top::mbus::xevent_t::succ;
    mb.push_event(ptr);
    ASSERT_TRUE(ptr->err == top::mbus::xevent_t::fail);


    uint32_t id1 = mb.add_listener(top::mbus::xevent_major_type_store, listener1);
    uint32_t id2 = mb.add_listener(top::mbus::xevent_major_type_store, listener2);
    uint32_t id3 = mb.add_sourcer(top::mbus::xevent_major_type_store, sourcer1);
    uint32_t id4 = mb.add_sourcer(top::mbus::xevent_major_type_store, sourcer2);

    ASSERT_TRUE(mb.size() == top::mbus::xevent_major_type_max);
    ASSERT_TRUE(mb.listeners_size() == 2);
    ASSERT_TRUE(mb.sourcers_size() == 2);

    // call listeners first time
    mb.push_event(ptr);
    ASSERT_TRUE(ptr->direction == top::mbus::xevent_store_t::to_listener);
    ASSERT_TRUE(ptr->owner == "owner");
    ASSERT_TRUE(listener1_receives == 1);
    ASSERT_TRUE(listener2_receives == 1);
    ASSERT_TRUE(sourcer1_receives == 0);
    ASSERT_TRUE(sourcer2_receives == 0);

    // call listeners second time
    mb.push_event(ptr);
    ASSERT_TRUE(ptr->direction == top::mbus::xevent_store_t::to_listener);
    ASSERT_TRUE(ptr->owner == "owner");
    ASSERT_TRUE(listener1_receives == 2);
    ASSERT_TRUE(listener2_receives == 2);
    ASSERT_TRUE(sourcer1_receives == 0);
    ASSERT_TRUE(sourcer2_receives == 0);

    // call sourcers first time
    ptr->direction = top::mbus::xevent_store_t::to_sourcer;
    mb.push_event(ptr);
    ASSERT_TRUE(ptr->owner == "store replid");
    ASSERT_TRUE(listener1_receives == 2);
    ASSERT_TRUE(listener2_receives == 2);
    ASSERT_TRUE(sourcer1_receives == 1);
    ASSERT_TRUE(sourcer2_receives == 1);

    // call sourcers second time
    mb.push_event(ptr);
    ASSERT_TRUE(ptr->owner == "store replid");
    ASSERT_TRUE(listener1_receives == 2);
    ASSERT_TRUE(listener2_receives == 2);
    ASSERT_TRUE(sourcer1_receives == 2);
    ASSERT_TRUE(sourcer2_receives == 2);

    mb.remove_listener(top::mbus::xevent_major_type_store, id1);
    ASSERT_TRUE(mb.listeners_size() == 1);

    mb.remove_listener(top::mbus::xevent_major_type_store, id2);
    ASSERT_TRUE(mb.listeners_size() == 0);

    mb.remove_sourcer(top::mbus::xevent_major_type_store, id3);
    ASSERT_TRUE(mb.sourcers_size() == 1);

    mb.remove_sourcer(top::mbus::xevent_major_type_store, id4);
    ASSERT_TRUE(mb.sourcers_size() == 0);
}

TEST(message_bus, timer) {
    int count = 0;
    top::mbus::xmessage_bus_t bus(true, 200);
    bus.add_listener(top::mbus::xevent_major_type_timer,
            [&](const top::mbus::xevent_ptr_t& e) {
        ++count;
        std::cout << "get timer event " << count << std::endl;
    });

    for(int i=0;i<3;i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(220));
        ASSERT_TRUE(i + 1 == count) << count;
    }
}

TEST(message_bus, multi_threads) {
    std::vector<std::thread> threads;
    top::mbus::xmessage_bus_t bus;
    std::vector<uint32_t> ids;
    for(int i=0;i<10;i++) {
        threads.push_back(std::thread([&]{
            ids.push_back(bus.add_listener(2, [](const top::mbus::xevent_ptr_t&){}));
        }));
    }

    for(auto& t : threads) {
        t.join();
    }

    for(uint32_t i=1;i<=10;i++) {
        ASSERT_TRUE(std::find(ids.begin(), ids.end(), i) != ids.end());
        std::cout << i << std::endl;
    }
}
