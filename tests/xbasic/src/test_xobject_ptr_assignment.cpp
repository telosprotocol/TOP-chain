// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xbasic/test_xobject_ptr_fixture.h"
#include "xbase/xobject_ptr.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, basic)

TEST(xobject_ptr_t, copy_assignment1) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    xobject_ptr_t<xbase_object1> another;
    another = object;
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());
}

TEST(xobject_ptr_t, copy_assignment2) {
    auto object = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, object->get_refcount());

    xobject_ptr_t<xbase_object1> another;
    another = object;
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());
}

TEST(xobject_ptr_t, copy_assignment3) {
    xobject_ptr_t<xbase_object1> object;
    //ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    xobject_ptr_t<xbase_object1> another;
    another = object;
    //ASSERT_EQ(nullptr, another);
    ASSERT_EQ(nullptr, another.get());
}

TEST(xobject_ptr_t, copy_assignment4) {
    xobject_ptr_t<xderived_object1> object;
    //ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    xobject_ptr_t<xbase_object1> another;
    another = object;
    //ASSERT_EQ(nullptr, another);
    ASSERT_EQ(nullptr, another.get());
}

TEST(xobject_ptr_t, copy_assignment5) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = top::make_object_ptr<xbase_object1>();
    auto * another_ptr = another.get();

    another = object;
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());

    // ASSERT_ANY_THROW(another_ptr->get_refcount());
}

TEST(xobject_ptr_t, copy_assignment6) {
    auto object = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = top::make_object_ptr<xbase_object1>();
    auto * another_ptr = another.get();

    another = object;
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());

    // ASSERT_ANY_THROW(another_ptr->get_refcount());
}

TEST(xobject_ptr_t, move_assignment1) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    xobject_ptr_t<xbase_object1> another;
    another = std::move(object);
    ASSERT_EQ(1, another->get_refcount());
    // ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());
}

TEST(xobject_ptr_t, move_assignment2) {
    auto object = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, object->get_refcount());

    xobject_ptr_t<xbase_object1> another;
    another = std::move(object);
    ASSERT_EQ(1, another->get_refcount());
    // ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());
}

TEST(xobject_ptr_t, move_assignment3) {
    xobject_ptr_t<xbase_object1> object;
    // ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    xobject_ptr_t<xbase_object1> another;
    another = std::move(object);
    // ASSERT_EQ(nullptr, another);
    ASSERT_EQ(nullptr, another.get());
}

TEST(xobject_ptr_t, move_assignment4) {
    xobject_ptr_t<xderived_object1> object;
    // ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    xobject_ptr_t<xbase_object1> another;
    another = std::move(object);
    // ASSERT_EQ(nullptr, another);
    ASSERT_EQ(nullptr, another.get());
}

TEST(xobject_ptr_t, move_assignment5) {
    auto object = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, another->get_refcount());
    // auto * another_ptr = another.get();

    another = std::move(object);
    ASSERT_EQ(1, another->get_refcount());
    // ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    // ASSERT_ANY_THROW(another_ptr->get_refcount());
}

TEST(xobject_ptr_t, move_assignment6) {
    auto object = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = top::make_object_ptr<xbase_object1>();
    // auto * another_ptr = another.get();

    another = std::move(object);
    ASSERT_EQ(1, another->get_refcount());
    // ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    // ASSERT_ANY_THROW(another_ptr->get_refcount());
}

TEST(xobject_ptr_t, copy_from_xauto_ptr1) {
    top::base::xauto_ptr<xbase_object1> object{nullptr};
    ASSERT_EQ(nullptr, object.get());

    top::xobject_ptr_t<xbase_object1> another;
    another = object;
    ASSERT_EQ(nullptr, another.get());
}

TEST(xobject_ptr_t, copy_assignment_from_xauto_ptr2) {
    top::base::xauto_ptr<xbase_object1> object{nullptr};
    ASSERT_EQ(nullptr, object.get());

    auto another = top::make_object_ptr<xbase_object1>();
    auto * another_ptr = another.get();

    another = object;
    ASSERT_EQ(nullptr, another.get());

    // ASSERT_ANY_THROW(another_ptr->get_refcount());
}

TEST(xobject_ptr_t, copy_assignment_from_xauto_ptr3) {
    auto object = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another;
    another = object;
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());
}

TEST(xobject_ptr_t, copy_assignment_from_xauto_ptr4) {
    auto object = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = top::make_object_ptr<xbase_object1>();
    // auto * another_ptr = another.get();

    another = object;
    ASSERT_EQ(2, another->get_refcount());
    ASSERT_EQ(object->get_refcount(), another->get_refcount());

    // ASSERT_ANY_THROW(another_ptr->get_refcount());
}

#if 0
// BAD test case, top::base::xauto_ptr prevents move construction to xobject_ptr_t.
TEST(xobject_ptr_t, move_assignment_from_xauto_ptr1) {
    top::base::xauto_ptr<xbase_object1> object{nullptr};
    ASSERT_EQ(nullptr, object);

    top::xobject_ptr_t<xbase_object1> another;
    another = std::move(object);
    ASSERT_EQ(nullptr, another);
}

TEST(xobject_ptr_t, move_assignment_from_xauto_ptr2) {
    top::base::xauto_ptr<xbase_object1> object{nullptr};
    ASSERT_EQ(nullptr, object);

    auto another = top::make_object_ptr<xbase_object1>();
    auto * another_ptr = another.get();

    another = std::move(object);
    ASSERT_EQ(nullptr, another);

    ASSERT_ANY_THROW(another_ptr->get_refcount());
}

TEST(xobject_ptr_t, move_assignment_from_xauto_ptr3) {
    auto object = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    top::xobject_ptr_t<xbase_object1> another;
    another = std::move(object);
    ASSERT_EQ(1, another->get_refcount());
    ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());
}

TEST(xobject_ptr_t, move_assignment_from_xauto_ptr4) {
    auto object = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, object->get_refcount());

    auto another = top::make_object_ptr<xbase_object1>();
    auto * another_ptr = another.get();

    another = std::move(object);
    ASSERT_EQ(1, another->get_refcount());
    ASSERT_EQ(nullptr, object);
    ASSERT_EQ(nullptr, object.get());

    ASSERT_ANY_THROW(another_ptr->get_refcount());
}
#endif

NS_END3
