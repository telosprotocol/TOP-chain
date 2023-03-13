// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xobject_ptr.h"

#include "tests/xbasic/test_xobject_ptr_fixture.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, basic)

TEST(xobject_ptr_t, default_constructor) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = new xbase_object2;
    ASSERT_EQ(1, another->get_refcount());
    another->release_ref();
}

TEST(xobject_ptr_t, nullptr_t) {
    xobject_ptr_t<xbase_object1> object;
    //ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    xobject_ptr_t<xbase_object1> object1{nullptr};
    //ASSERT_EQ(nullptr, object1);
    ASSERT_EQ(nullptr, object1.get());

    xobject_ptr_t<xbase_object1> object2{NULL};
    //ASSERT_EQ(nullptr, object2);
    ASSERT_EQ(nullptr, object2.get());
}

TEST(xobject_ptr_t, copy_constructor1) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another{object};
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());
}

TEST(xobject_ptr_t, copy_constructor2) {
    auto object = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another{object};
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());
}

TEST(xobject_ptr_t, move_constructor1) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another{std::move(object)};
    ASSERT_EQ(1, another->get_refcount());
    //ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());
}

TEST(xobject_ptr_t, move_constructor2) {
    auto object = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another{std::move(object)};
    ASSERT_EQ(1, another->get_refcount());
    //ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());
}

#if 0
// BAD test case, top::base::xauto_ptr prevents move construction to xobject_ptr_t.
TEST(xobject_ptr_t, construct_from_xauto_ptr1) {
    auto object = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    // top::xobject_ptr_t<xbase_object1> another{top::base::make_auto_ptr<xbase_object1>()};
    // top::xobject_ptr_t<xbase_object1> another{std::move(object)};
    top::xobject_ptr_t<xbase_object1> another;
    ASSERT_EQ(1, another->get_refcount());
    ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());
}
#endif

TEST(xobject_ptr_t, construct_from_xauto_ptr2) {
    auto object = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another{object};
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());
}

NS_END3
