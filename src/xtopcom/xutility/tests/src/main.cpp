#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include "xbase/xutl.h"
#include "xutility/xmerkle.hpp"
#include "xbase/xcontext.h"
#include "xutility/xpriority_queue.hpp"
#include "xbasic/xserializable_based_on.h"

using std::string;
using std::vector;

template<typename _Th, typename _Tv>
class xtest_digest_wrap_t {
public:

    void split_string(const std::string& s, std::vector<std::string>& v, const std::string& c) {
        std::string::size_type pos1, pos2;
        pos2 = s.find(c);
        pos1 = 0;
        while (std::string::npos != pos2) {
            v.push_back(s.substr(pos1, pos2 - pos1));

            pos1 = pos2 + c.size();
            pos2 = s.find(c, pos1);
        }
        if (pos1 != s.length())
            v.push_back(s.substr(pos1));
    }

    xtest_digest_wrap_t(const char* str) :
    xtest_digest_wrap_t(string(str))
    {
    }

    xtest_digest_wrap_t(string str) {
        split_string(str, sub_strs, string(" "));
        for (auto& s : sub_strs) {
            hash_list.push_back(digest(s));
        }
        int index = 0;
        int old_size = hash_list.size();
        if (old_size > 1) {
            while (true) {
                add_next_level(index, hash_list.size() - 1);
                index = old_size;
                if (hash_list.size() - old_size == 1) break;
                old_size = hash_list.size();
            }
        }
    }

    void add_next_level(int start, int end) {
        int round_end = (end - start + 1) / 2 * 2 - 1 + start;
        for (int i = start; i <= round_end; i += 2) {
            hash_list.push_back(digest(hash_list[i], hash_list[i + 1]));
        }
        if (round_end != end) {
            hash_list.push_back(digest(hash_list[end]));
        }
    }

    string digest(string &str) {
        _Tv v = _Th::digest(str);
        return string((char*) v.data(), v.size());
    }

    string digest(const char* str) {
        _Tv v = _Th::digest(str, strlen(str));
        return string((char*) v.data(), v.size());
    }

    string digest(string& left, string& right) {
        _Th h;
        h.update(left);
        h.update(right);
        _Tv v;
        h.get_hash(v);
        return string((char*) v.data(), v.size());
    }

    string digest(const char* left, const char* right) {
        _Th h;
        h.update(left, strlen(left));
        h.update(right, strlen(right));
        _Tv v;
        h.get_hash(v);
        return string((char*) v.data(), v.size());
    }


    std::vector<std::string> hash_list{};
    std::vector<std::string> sub_strs{};

    static std::shared_ptr<xtest_digest_wrap_t<_Th, _Tv>>
    create_object(int str_segs) {
        std::string s;
        for (int i = 0; i < str_segs; i++) {
            s += std::to_string(i);
            if (i < str_segs - 1) s += " ";
        }
        return std::make_shared<xtest_digest_wrap_t<_Th, _Tv>>(s);
    }
};

int
main(int argc, char * argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(string_utl, base_split) {
    string orig_str = "hello,world,";
    vector<string> sub_str;
    top::base::xstring_utl::split_string(orig_str, ',', sub_str);
    ASSERT_EQ(sub_str.size(), 2);
    ASSERT_EQ(sub_str[0], "hello");
    ASSERT_EQ(sub_str[1], "world");
}

TEST(string_utl, base_ip) {
    string orig_str = "127.0.0.1";
    vector<string> sub_str;
    top::base::xstring_utl::split_string(orig_str, ',', sub_str);
    ASSERT_EQ(sub_str.size(), 1);
    ASSERT_EQ(sub_str[0], "127.0.0.1");
}

#define MATCH_NODE_STR(n, s) \
    ASSERT_TRUE(string((char*) n.signature.data(), n.signature.size()) == s)

TEST(xmerkle, sha_test) {
    top::utl::xsha2_256_t h;
    top::uint256_t v;

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw1("this");
    h.reset();
    h.update(string("this"));
    h.get_hash(v);
    ASSERT_TRUE(string((char*) v.data(), v.size()) == dw1.hash_list.back());

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw2("is");
    h.reset();
    h.update(string("is"));
    h.get_hash(v);
    ASSERT_TRUE(string((char*) v.data(), v.size()) == dw2.hash_list.back());

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw3("atest");
    h.reset();
    h.update(string("a"));
    h.update(string("test"));
    h.get_hash(v);
    ASSERT_TRUE(string((char*) v.data(), v.size()) == dw3.hash_list.back());
}

TEST(xmerkle_path_node_t, stream) {
    top::base::xstream_t s(top::base::xcontext_t::instance());
    top::xmerkle_path_node_t < top::uint256_t > n, n1;

    char data[32] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};

    n.level = 0xA7B;
    n.pos = 0x37C;
    memcpy(n.signature.data(), data, 32);

    ASSERT_TRUE(n.serialize_to(s) == 48);
    ASSERT_TRUE(n1.serialize_from(s) == 48);

    ASSERT_TRUE(n1.level == n.level);
    ASSERT_TRUE(n1.pos == n.pos);
    ASSERT_TRUE(memcmp(n1.signature.data(), data, 32) == 0);
}

TEST(xmerkle, calc_root) {
    top::xmerkle_t<top::utl::xsha2_256_t, top::uint256_t> m;
    ASSERT_TRUE(m.calc_root(std::vector<std::string>()).empty());

    const char* strs[9]{
        "this",
        "this is",
        "this is a",
        "this is a test",
        "this is a test for",
        "this is a test for merkle",
        "this is a test for merkle tree",
        "this is a test for merkle tree totally",
        "this is a test for merkle tree totally 9"
    };
    size_t sizes[9]{1, 3, 6, 7, 11, 12, 14, 15, 20};
    for (int i = 0; i < 9; i++) {
        xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw(strs[i]);
        ASSERT_TRUE(dw.hash_list.size() == sizes[i]);
        ASSERT_TRUE(dw.hash_list.back() == m.calc_root(dw.sub_strs)) << " index " << i;
    }
}

TEST(xmerkle, calc_path) {
    std::vector<top::xmerkle_path_node_t < top::uint256_t >> hash_path;
    top::xmerkle_t<top::utl::xsha2_256_t, top::uint256_t> m;

    ASSERT_FALSE(m.calc_path(std::vector<std::string>(), 0, hash_path));

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw1("this");
    hash_path.clear();
    ASSERT_FALSE(m.calc_path(dw1.sub_strs, -1, hash_path));
    ASSERT_FALSE(m.calc_path(dw1.sub_strs, 1, hash_path));
    ASSERT_TRUE(m.calc_path(dw1.sub_strs, 0, hash_path));
    ASSERT_TRUE(hash_path.empty());

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw2("this is");
    ASSERT_FALSE(m.calc_path(dw2.sub_strs, 2, hash_path));
    int dw2_indexes[2]{1, 0};
    for (int i = 0; i < 2; i++) {
        hash_path.clear();
        ASSERT_TRUE(m.calc_path(dw2.sub_strs, i, hash_path));
        ASSERT_TRUE(hash_path.size() == 1);
        MATCH_NODE_STR(hash_path[0], dw2.hash_list[dw2_indexes[i]]);
    }

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw3("this is a");
    int dw3_indexes[3][2]{
        {1, 4},
        {0, 4},
        {2, 3}
    };
    for (int i = 0; i < 3; i++) {
        hash_path.clear();
        ASSERT_TRUE(m.calc_path(dw3.sub_strs, i, hash_path));
        ASSERT_TRUE(hash_path.size() == 2);
        MATCH_NODE_STR(hash_path[0], dw3.hash_list[dw3_indexes[i][0]]);
        MATCH_NODE_STR(hash_path[1], dw3.hash_list[dw3_indexes[i][1]]);
    }

    xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t> dw9("this is a test for merkle tree totally 9");
    int dw9_indexes[8][4]{
        {1, 10, 15, 18},
        {0, 10, 15, 18},
        {3, 9, 15, 18},
        {2, 9, 15, 18},
        {5, 12, 14, 18},
        {4, 12, 14, 18},
        {7, 11, 14, 18},
        {6, 11, 14, 18}
    };
    for (int i = 0; i < 8; i++) {
        hash_path.clear();
        ASSERT_TRUE(m.calc_path(dw9.sub_strs, i, hash_path));
        ASSERT_TRUE(hash_path.size() == 4);
        MATCH_NODE_STR(hash_path[0], dw9.hash_list[dw9_indexes[i][0]]);
        MATCH_NODE_STR(hash_path[1], dw9.hash_list[dw9_indexes[i][1]]);
        MATCH_NODE_STR(hash_path[2], dw9.hash_list[dw9_indexes[i][2]]);
        MATCH_NODE_STR(hash_path[3], dw9.hash_list[dw9_indexes[i][3]]);
    }

    hash_path.clear();
    ASSERT_TRUE(m.calc_path(dw9.sub_strs, 8, hash_path));
    ASSERT_TRUE(hash_path.size() == 2);
    MATCH_NODE_STR(hash_path[0], dw9.hash_list[8]);
    MATCH_NODE_STR(hash_path[1], dw9.hash_list[17]);
}

TEST(xmerkle, validate_path) {
    std::vector<top::xmerkle_path_node_t < top::uint256_t >> hash_path;
    top::xmerkle_t<top::utl::xsha2_256_t, top::uint256_t> m;

    ASSERT_FALSE(m.validate_path(std::string(), std::string("any"), hash_path));
    ASSERT_FALSE(m.validate_path(std::string("any"), std::string(), hash_path));

    // test 2 ^ N + 1
    for(int i=1;i<=1025;i++) {
        std::shared_ptr<xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t>> ptr =
                xtest_digest_wrap_t<top::utl::xsha2_256_t, top::uint256_t>::create_object(i);
        if (!(i % 10)) {
            std::cout << i << "/" << std::flush; // this tast case cause much time, tracking...
        }
        for (auto n = 0u; n < ptr->sub_strs.size(); n++) {
            hash_path.clear();
            ASSERT_TRUE(m.calc_path(ptr->sub_strs, n, hash_path));
            ASSERT_TRUE(m.validate_path(ptr->sub_strs[n], ptr->hash_list.back(), hash_path)) << "i=" << i << " n=" << n;
        }
    }

}

struct xtest_priority_queue_t {
    int value {0};

    xtest_priority_queue_t(int v) :
    value(v) {}
};

TEST(xpriority_queue, tests) {
    using xtest_priority_queue_ptr_t =
            std::shared_ptr<xtest_priority_queue_t>;

    top::utl::xpriority_queue_t<xtest_priority_queue_ptr_t> q(3);

#define GET_QUEUE_SIZE() \
    [&]()->int{ \
            int s = 0; \
            for(auto& l : q.get_queue()) { \
                s += l->size(); \
            } \
            return s; \
        }()

    q.add(2, std::make_shared<xtest_priority_queue_t>(1));
    q.add(1, std::make_shared<xtest_priority_queue_t>(2));
    q.add(0, std::make_shared<xtest_priority_queue_t>(3));
    q.add(1, std::make_shared<xtest_priority_queue_t>(4));
    q.add(2, std::make_shared<xtest_priority_queue_t>(5));

    xtest_priority_queue_ptr_t ptr;
    EXPECT_TRUE(q.peek(0, ptr));
    EXPECT_TRUE(ptr->value == 3);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 5);

    EXPECT_TRUE(q.peek(1, ptr));
    EXPECT_TRUE(ptr->value == 2);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 5);

    EXPECT_TRUE(q.peek(2, ptr));
    EXPECT_TRUE(ptr->value == 1);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 5);

    EXPECT_TRUE(q.pop(0, ptr));
    EXPECT_TRUE(ptr->value == 3);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 4);

    EXPECT_TRUE(q.peek(ptr));
    EXPECT_TRUE(ptr->value == 2);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 4);

    EXPECT_TRUE(q.pop(ptr));
    EXPECT_TRUE(ptr->value == 2);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 3);

    EXPECT_TRUE(q.pop(ptr));
    EXPECT_TRUE(ptr->value == 4);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 2);

    EXPECT_TRUE(q.pop(ptr));
    EXPECT_TRUE(ptr->value == 1);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 1);

    EXPECT_TRUE(q.pop(ptr));
    EXPECT_TRUE(ptr->value == 5);
    EXPECT_TRUE(GET_QUEUE_SIZE() == 0);

    EXPECT_FALSE(q.pop(ptr));
}
