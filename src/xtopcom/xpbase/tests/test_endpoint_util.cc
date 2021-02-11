#include <gtest/gtest.h>

#define private public
#define protected public
#include "xpbase/base/endpoint_util.h"

namespace top {
namespace base {
namespace test {

class TestEndpointUtil : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(TestEndpointUtil, ParseEndpoints) {
    {
        const std::string endpoints;

        std::vector<std::string> vec;
        ParseEndpoints(endpoints, vec);
        ASSERT_TRUE(vec.empty());

        std::set<std::pair<std::string, uint16_t>> mapep;
        ParseEndpoints(endpoints, mapep);
        ASSERT_TRUE(mapep.empty());
    }

    {
        const std::string endpoints = "1:2";

        std::vector<std::string> vec;
        ParseEndpoints(endpoints, vec);
        ASSERT_EQ(1u, vec.size());
        ASSERT_EQ("1:2", vec[0]);

        std::set<std::pair<std::string, uint16_t>> mapep;
        ParseEndpoints(endpoints, mapep);
        ASSERT_EQ(1u, mapep.size());
        ASSERT_EQ("1", mapep.begin()->first);
        ASSERT_EQ(2u, mapep.begin()->second);
    }

    {
        const std::string endpoints = "1:2,3:4";
        std::vector<std::string> vec1;
        ParseEndpoints(endpoints, vec1);
        ASSERT_EQ(2u, vec1.size());
        ASSERT_EQ("1:2", vec1[0]);
        ASSERT_EQ("3:4", vec1[1]);

        std::set<std::pair<std::string, uint16_t>> map1;
        ParseEndpoints(endpoints, map1);
        ASSERT_EQ(2u, map1.size());

        std::set<std::pair<std::string, uint16_t>> map2;
        ParseVecEndpoint(vec1, map2);
        ASSERT_EQ(map1, map2);

        std::vector<std::string> vec2;
        ParseSetEndpoint(map1, vec2);
        ASSERT_EQ(vec1, vec2);
    }
}

TEST_F(TestEndpointUtil, ParseEndpoints_3) {
    {
        const std::string endpoints = "1:2, 3:4,5:6";
        std::vector<std::string> vec;
        ParseEndpoints(endpoints, vec);
        ASSERT_EQ(3u, vec.size());
        ASSERT_EQ(" 3:4", vec[0]);
        ASSERT_EQ("1:2", vec[1]);  // !!
        ASSERT_EQ("5:6", vec[2]);
    }
}

TEST_F(TestEndpointUtil, ParseEndpoints_ignore_invalid) {
    {
        const std::string endpoints = "1:2, 56";
        std::vector<std::string> vec;
        ParseEndpoints(endpoints, vec);
        ASSERT_EQ(1u, vec.size());
        ASSERT_EQ("1:2", vec[0]);

        std::set<std::pair<std::string, uint16_t>> mapep;
        ParseEndpoints(endpoints, mapep);
        ASSERT_EQ(1u, mapep.size());
        ASSERT_EQ("1", mapep.begin()->first);
        ASSERT_EQ(2u, mapep.begin()->second);
    }

    {
        const std::string endpoints = "5:a,3:4";
        std::set<std::pair<std::string, uint16_t>> mapep;
        ParseEndpoints(endpoints, mapep);
        ASSERT_EQ(1u, mapep.size());
        ASSERT_EQ("3", mapep.begin()->first);
        ASSERT_EQ(4u, mapep.begin()->second);
    }
}

TEST_F(TestEndpointUtil, MergeEndpoints) {
    {
        const std::set<std::pair<std::string, uint16_t>> map1 = {
            {"1", 2}
        };
        std::set<std::pair<std::string, uint16_t>> map2 = {
            {"3", 4},
            {"5", 6},
        };

        MergeEndpoints(map1, map2);
        ASSERT_EQ(3u, map2.size());
        ASSERT_EQ(map1.begin()->first, map2.begin()->first);
        ASSERT_EQ(map1.begin()->second, map2.begin()->second);
    }
}

TEST_F(TestEndpointUtil, FormatMapEndpoint) {
    {
        std::set<std::pair<std::string, uint16_t>> mapep;
        const auto str = FormatSetEndpoint(mapep);
        ASSERT_EQ("", str);
    }

    {
        std::set<std::pair<std::string, uint16_t>> mapep = {
            {"3", 4},
            {"5", 6},
        };

        const auto str = FormatSetEndpoint(mapep);
        ASSERT_EQ("3:4,5:6", str);
    }
}

TEST_F(TestEndpointUtil, FormatVecEndpoint) {
    {
        std::vector<std::string> vec;
        const auto str = FormatVecEndpoint(vec);
        ASSERT_EQ("", str);
    }

    {
        std::vector<std::string> vec = {
            "3:4", "5:6"
        };

        const auto str = FormatVecEndpoint(vec);
        ASSERT_EQ("3:4,5:6", str);
    }
}

}  // namespace test
}  // namespace base
}  // namespace top
