#include "gtest/gtest.h"

#include <chrono>
#include <random>
#include <sstream>

class ServiceType {
public:
    // 13-14 ms
    ServiceType(uint64_t type) {
        std::string info{""};
        m_info += "[ver " + std::to_string((type >> 63)) + "]-";
        m_info += "[network " + std::to_string((type << 1) >> (1 + 43)) + "]-";
        m_info += "[zone " + std::to_string((type << 21) >> (21 + 36)) + "]-";
        m_info += "[cluster " + std::to_string((type << 28) >> (28 + 29)) + "]-";
        m_info += "[group " + std::to_string((type << 35) >> (35 + 21)) + "]-";
        m_info += "[height " + std::to_string((type << 43) >> 43) + "]";
        m_info = info;
    }

    std::string m_info;
};

class ServiceType2 {
public:
    // 9-10 ms
    ServiceType2(uint64_t type) {
        m_info = "[ver " + std::to_string((type >> 63)) + "]-" + "[network " + std::to_string((type << 1) >> (1 + 43)) + "]-" + "[zone " +
                 std::to_string((type << 21) >> (21 + 36)) + "]-" + "[cluster " + std::to_string((type << 28) >> (28 + 29)) + "]-" + "[group " +
                 std::to_string((type << 35) >> (35 + 21)) + "]-" + "[height " + std::to_string((type << 43) >> 43) + "]";
    }

    std::string m_info;
};

class ServiceType3 {
public:
    // 8-9 ms
    ServiceType3(uint64_t type) {
        std::string buffer;
        buffer.reserve(100);
        std::ostringstream oss(buffer);

        oss << "[ver " << ((type >> 63)) << "]-"
            << "[network " << ((type << 1) >> (1 + 43)) << "]-"
            << "[zone " << ((type << 21) >> (21 + 36)) << "]-"
            << "[cluster " << ((type << 28) >> (28 + 29)) << "]-"
            << "[group " << ((type << 35) >> (35 + 21)) << "]-"
            << "[height " << ((type << 43) >> 43) << "]";

        m_info = oss.str();
    }

    std::string m_info;
};

TEST(ser, current) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    // Generate a random uint64
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
    uint64_t rand_num = dist(gen);
    // Output the random number

    auto cost = 0;

    for (auto _i = 0; _i < 10000; _i++) {
        rand_num = dist(gen);
        // Output the random number

        auto start = std::chrono::high_resolution_clock::now();
        ServiceType x = ServiceType{rand_num};
        auto end = std::chrono::high_resolution_clock::now();

        auto this_cost = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        cost = cost + this_cost.count();
    }
    std::cout << cost << std::endl;
}

TEST(ser, perf2) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    // Generate a random uint64
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
    uint64_t rand_num = dist(gen);
    // Output the random number

    auto cost = 0;

    for (auto _i = 0; _i < 10000; _i++) {
        rand_num = dist(gen);
        // Output the random number

        auto start = std::chrono::high_resolution_clock::now();
        ServiceType3 x = ServiceType3{rand_num};
        auto end = std::chrono::high_resolution_clock::now();

        auto this_cost = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        cost = cost + this_cost.count();
    }
    std::cout << cost << std::endl;
}
