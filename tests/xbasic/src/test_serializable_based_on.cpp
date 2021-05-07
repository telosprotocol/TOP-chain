// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xserializable_based_on.h"

#include <gtest/gtest.h>

class old_data final : public top::xserializable_based_on<void> {
public:
    int i{ 0 };
    int j{ 1 };
    old_data * next{ nullptr };

    std::int32_t
    do_write(top::base::xstream_t & stream) const override {
        auto const begin_size = stream.size();
        stream << i;
        stream << j;
        if (next != nullptr) {
            next->serialize_to(stream);
        }
        auto const end_size = stream.size();
        return end_size - begin_size;
    }

    std::int32_t
    do_read(top::base::xstream_t & stream) override {
        auto const begin_size = stream.size();
        stream >> i;
        stream >> j;
        if (stream.size() > 0) {
            next = new old_data;
            next->serialize_from(stream);
        }
        auto const end_size = stream.size();
        return begin_size - end_size;
    }
};

class new_data final : public top::xserializable_based_on<void> {
public:
    int i{ 0 };
    int j{ 1 };
    new_data * next{ nullptr };
    new_data * pre{ nullptr };

    std::int32_t
    do_write(top::base::xstream_t & stream) const override {
        auto const begin_size = stream.size();
        stream << i;
        stream << j;
        if (next != nullptr) {
            next->serialize_to(stream);

            if (pre != nullptr) {
                pre->serialize_to(stream);
            }
        }

        auto const end_size = stream.size();
        return end_size - begin_size;
    }

    std::int32_t
    do_read(top::base::xstream_t & stream) override {
        auto const begin_size = stream.size();
        stream >> i;
        stream >> j;
        if (stream.size() > 0) {
            next = new new_data;
            next->serialize_from(stream);

            if (stream.size() > 0) {
                pre = new new_data;
                pre->serialize_from(stream);
            }
        }
        auto const end_size = stream.size();
        return begin_size - end_size;
    }
};

TEST(serializable, read_old_data1) {
    old_data expected;
    expected.i = 1;
    expected.j = 2;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    old_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_EQ(expected.next, actual.next);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    old_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_EQ(expected.next, actual2.next);
}

TEST(serializable, read_old_data2) {
    old_data expected;
    expected.i = 1;
    expected.j = 2;
    expected.next = new old_data;
    expected.next->i = 3;
    expected.next->j = 4;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    old_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_TRUE(actual.next != nullptr);
    EXPECT_EQ(expected.next->i, actual.next->i);
    EXPECT_EQ(expected.next->j, actual.next->j);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    old_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_TRUE(actual2.next != nullptr);
    EXPECT_EQ(expected.next->i, actual2.next->i);
    EXPECT_EQ(expected.next->j, actual2.next->j);
}

TEST(serializable, new_read_old_data1) {
    old_data expected;
    expected.i = 1;
    expected.j = 2;
    expected.next = new old_data;
    expected.next->i = 3;
    expected.next->j = 4;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    new_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_TRUE(actual.next != nullptr);
    EXPECT_TRUE(actual.pre == nullptr);
    EXPECT_EQ(expected.next->i, actual.next->i);
    EXPECT_EQ(expected.next->j, actual.next->j);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    new_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_TRUE(actual2.next != nullptr);
    EXPECT_TRUE(actual2.pre == nullptr);
    EXPECT_EQ(expected.next->i, actual2.next->i);
    EXPECT_EQ(expected.next->j, actual2.next->j);
}

TEST(serializable, new_read_old_data2) {
    old_data expected;
    expected.i = 1;
    expected.j = 2;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    new_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_TRUE(actual.next == nullptr);
    EXPECT_TRUE(actual.pre == nullptr);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    new_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_TRUE(actual2.next == nullptr);
    EXPECT_TRUE(actual2.pre == nullptr);
}

TEST(serializable, old_read_new_data1) {
    new_data expected;
    expected.i = 1;
    expected.j = 2;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    old_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_TRUE(actual.next == nullptr);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    old_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_TRUE(actual2.next == nullptr);
}

TEST(serializable, old_read_new_data2) {
    new_data expected;
    expected.i = 1;
    expected.j = 2;
    expected.next = new new_data;
    expected.next->i = 3;
    expected.next->j = 4;
    expected.next->next = new new_data;
    expected.next->next->i = 5;
    expected.next->next->j = 6;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    old_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_TRUE(actual.next != nullptr);
    EXPECT_EQ(expected.next->i, actual.next->i);
    EXPECT_EQ(expected.next->j, actual.next->j);
    EXPECT_TRUE(expected.next->next != nullptr);
    EXPECT_EQ(expected.next->next->i, actual.next->next->i);
    EXPECT_EQ(expected.next->next->j, actual.next->next->j);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    old_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_TRUE(actual2.next != nullptr);
    EXPECT_EQ(expected.next->i, actual2.next->i);
    EXPECT_EQ(expected.next->j, actual2.next->j);
    EXPECT_TRUE(expected.next->next != nullptr);
    EXPECT_EQ(expected.next->next->i, actual2.next->next->i);
    EXPECT_EQ(expected.next->next->j, actual2.next->next->j);
}

TEST(serializable, old_read_new_data3) {
    new_data expected;
    expected.i = 1;
    expected.j = 2;
    expected.next = new new_data;
    expected.next->i = 3;
    expected.next->j = 4;
    expected.pre = new new_data;
    expected.pre->i = 5;
    expected.pre->j = 6;
    expected.next->next = new new_data;
    expected.next->next->i = 7;
    expected.next->next->j = 8;

    top::base::xstream_t s{ top::base::xcontext_t::instance() };
    expected.do_write(s);

    old_data actual;
    actual.do_read(s);

    EXPECT_EQ(expected.i, actual.i);
    EXPECT_EQ(expected.j, actual.j);
    EXPECT_TRUE(actual.next != nullptr);
    EXPECT_EQ(expected.next->i, actual.next->i);
    EXPECT_EQ(expected.next->j, actual.next->j);
    EXPECT_TRUE(expected.next->next != nullptr);
    EXPECT_EQ(expected.next->next->i, actual.next->next->i);
    EXPECT_EQ(expected.next->next->j, actual.next->next->j);

    std::error_code ec;
    auto bytes = expected.serialize_based_on<top::base::xstream_t>(ec);
    EXPECT_EQ(0, ec.value());

    old_data actual2;
    actual2.deserialize_based_on<top::base::xstream_t>(bytes, ec);
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ(expected.i, actual2.i);
    EXPECT_EQ(expected.j, actual2.j);
    EXPECT_TRUE(actual2.next != nullptr);
    EXPECT_EQ(expected.next->i, actual2.next->i);
    EXPECT_EQ(expected.next->j, actual2.next->j);
    EXPECT_TRUE(expected.next->next != nullptr);
    EXPECT_EQ(expected.next->next->i, actual2.next->next->i);
    EXPECT_EQ(expected.next->next->j, actual2.next->next->j);
}

TEST(serializable, deserialize_from_invalid_input) {
    std::error_code ec;
    top::xbyte_buffer_t empty_bytes;

    EXPECT_NO_THROW(
        old_data data;
        data.deserialize_based_on<top::base::xstream_t>(empty_bytes, ec);
    );
    EXPECT_NE(0, ec.value());
}
