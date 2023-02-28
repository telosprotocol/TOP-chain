// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xobject_ptr.h"

#include "tests/xbasic/test_xobject_ptr_fixture.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, basic)

TEST(xobject_ptr_t, static_pointer_cast1) {
    // T to T

    auto base_obj1 = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, base_obj1->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(base_obj1);
        ASSERT_EQ(2, base_obj1->get_refcount());
        ASSERT_EQ(base_obj1->get_refcount(), base_obj1_casted->get_refcount());
    }

    ASSERT_EQ(1, base_obj1->get_refcount());
}

TEST(xobject_ptr_t, static_pointer_cast2) {
    // Derived to Base

    auto derived_obj1 = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj1->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(derived_obj1);
        ASSERT_EQ(2, derived_obj1->get_refcount());
        ASSERT_EQ(derived_obj1->get_refcount(), base_obj1_casted->get_refcount());
    }

    ASSERT_EQ(1, derived_obj1->get_refcount());
}

TEST(xobject_ptr_t, static_pointer_cast3) {
    // Multi-Derived to every Base

    auto derived_obj2 = top::make_object_ptr<xderived_object2>();
    ASSERT_EQ(1, derived_obj2->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(derived_obj2);
        ASSERT_EQ(2, derived_obj2->get_refcount());
        ASSERT_EQ(derived_obj2->get_refcount(), base_obj1_casted->get_refcount());
    }

    {
        auto base_obj2_casted = top::static_xobject_ptr_cast<xbase_object2>(derived_obj2);
        ASSERT_EQ(2, derived_obj2->get_refcount());
        ASSERT_EQ(derived_obj2->get_refcount(), base_obj2_casted->get_refcount());
    }

    ASSERT_EQ(1, derived_obj2->get_refcount());
}

TEST(xobject_ptr_t, static_pointer_cast4) {
    // T to T from xauto_ptr to xobject_ptr_t
    auto base_obj1 = top::base::make_auto_ptr<xbase_object1>();
    ASSERT_EQ(1, base_obj1->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(base_obj1);

        ASSERT_EQ(base_obj1->get_refcount(), base_obj1_casted->get_refcount());
        ASSERT_EQ(2, base_obj1->get_refcount());
    }

    ASSERT_EQ(1, base_obj1->get_refcount());
}

TEST(xobject_ptr_t, dynamic_pointer_cast1) {
    // Derived to Base and back to Derived

    auto derived_obj1 = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj1->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(derived_obj1);
        auto derived_obj1_casted = top::dynamic_xobject_ptr_cast<xderived_object1>(base_obj1_casted);

        ASSERT_EQ(3, derived_obj1_casted->get_refcount());
        ASSERT_EQ(derived_obj1->get_refcount(), derived_obj1_casted->get_refcount());
    }

    ASSERT_EQ(1, derived_obj1->get_refcount());
}

TEST(xobject_ptr_t, dynamic_pointer_cast2) {
    // Derived to Base1, Base1 to Base2

    auto derived_obj2 = top::make_object_ptr<xderived_object2>();
    ASSERT_EQ(1, derived_obj2->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(derived_obj2);
        auto base_obj2_casted = top::dynamic_xobject_ptr_cast<xbase_object2>(base_obj1_casted);

        ASSERT_EQ(3, base_obj2_casted->get_refcount());
        ASSERT_EQ(derived_obj2->get_refcount(), base_obj2_casted->get_refcount());
    }

    ASSERT_EQ(1, derived_obj2->get_refcount());
}

TEST(xobject_ptr_t, dynamic_pointer_cast3) {
    // Derived to Base1, Base1 to Base2 (invalid)

    auto derived_obj1 = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj1->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(derived_obj1);
        auto base_obj2_casted = top::dynamic_xobject_ptr_cast<xbase_object2>(base_obj1_casted);

        ASSERT_EQ(nullptr, base_obj2_casted.get());
        ASSERT_EQ(2, derived_obj1->get_refcount());
        ASSERT_EQ(base_obj1_casted->get_refcount(), derived_obj1->get_refcount());
    }

    ASSERT_EQ(1, derived_obj1->get_refcount());
}

TEST(xobject_ptr_t, dynamic_pointer_cast4) {
    // Derived to Base and back to Derived

    auto derived_obj1 = top::base::make_auto_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj1->get_refcount());

    {
        derived_obj1->add_ref();
        top::base::xauto_ptr<xbase_object1> base_obj1{derived_obj1.get()};
        ASSERT_EQ(2, base_obj1->get_refcount());

        {
            auto derived_obj1_casted = top::dynamic_xobject_ptr_cast<xderived_object1>(base_obj1);

            ASSERT_EQ(3, derived_obj1_casted->get_refcount());
            ASSERT_EQ(derived_obj1->get_refcount(), derived_obj1_casted->get_refcount());
        }
    }

    ASSERT_EQ(1, derived_obj1->get_refcount());
}

TEST(xobject_ptr_t, dynamic_pointer_cast5) {
    // Derived to Base1, Base1 to Base2

    auto derived_obj2 = top::make_object_ptr<xderived_object2>();
    ASSERT_EQ(1, derived_obj2->get_refcount());

    {
        derived_obj2->add_ref();
        top::base::xauto_ptr<xbase_object1> base_obj1{derived_obj2.get()};
        ASSERT_EQ(2, base_obj1->get_refcount());

        {
            auto base_obj2_casted = top::dynamic_xobject_ptr_cast<xbase_object2>(base_obj1);

            ASSERT_EQ(3, base_obj2_casted->get_refcount());
            ASSERT_EQ(derived_obj2->get_refcount(), base_obj2_casted->get_refcount());
        }
    }

    ASSERT_EQ(1, derived_obj2->get_refcount());
}

TEST(xobject_ptr_t, dynamic_pointer_cast6) {
    // Derived to Base1, Base1 to Base2 (invalid)

    auto derived_obj1 = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj1->get_refcount());

    {
        derived_obj1->add_ref();
        top::base::xauto_ptr<xbase_object1> base_obj1{derived_obj1.get()};
        ASSERT_EQ(2, base_obj1->get_refcount());

        {
            auto base_obj2_casted = top::dynamic_xobject_ptr_cast<xbase_object2>(base_obj1);

            ASSERT_EQ(nullptr, base_obj2_casted.get());
            ASSERT_EQ(2, derived_obj1->get_refcount());
            ASSERT_EQ(base_obj1->get_refcount(), derived_obj1->get_refcount());
        }
    }

    ASSERT_EQ(1, derived_obj1->get_refcount());
}

TEST(xobject_ptr_t, to_xauto_ptr1) {
    auto obj_ptr = top::make_object_ptr<xbase_object1>();
    ASSERT_EQ(1, obj_ptr.get()->get_refcount());

    {
        top::base::xauto_ptr<xbase_object1> auto_ptr = obj_ptr;

        ASSERT_EQ(auto_ptr->get_refcount(), obj_ptr->get_refcount());
        ASSERT_EQ(2, auto_ptr->get_refcount());
    }

    ASSERT_EQ(1, obj_ptr->get_refcount());
}

#if 0
TEST(xauto_ptr_cast, static_pointer_cast1) {
    // Derive to Base
    auto derived_obj_ptr = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj_ptr->get_refcount());

    {
        auto base_obj1 = top::static_xauto_ptr_cast<xbase_object1>(derived_obj_ptr);

        ASSERT_EQ(base_obj1->get_refcount(), derived_obj_ptr->get_refcount());
        ASSERT_EQ(2, base_obj1->get_refcount());
    }

    ASSERT_EQ(1, derived_obj_ptr->get_refcount());
}

TEST(xauto_ptr_cast, static_pointer_cast2) {
    // Multi-Derived to every Base

    auto derived_obj2 = top::make_object_ptr<xderived_object2>();
    ASSERT_EQ(1, derived_obj2->get_refcount());

    {
        auto base_obj1_casted = top::static_xauto_ptr_cast<xbase_object1>(derived_obj2);
        ASSERT_EQ(2, derived_obj2->get_refcount());
        ASSERT_EQ(derived_obj2->get_refcount(), base_obj1_casted->get_refcount());
    }

    {
        auto base_obj2_casted = top::static_xauto_ptr_cast<xbase_object2>(derived_obj2);
        ASSERT_EQ(2, derived_obj2->get_refcount());
        ASSERT_EQ(derived_obj2->get_refcount(), base_obj2_casted->get_refcount());
    }

    ASSERT_EQ(1, derived_obj2->get_refcount());
}

TEST(xauto_ptr_cast, dynamic_pointer_cast1) {
    // Derived to Base

    auto derived_obj1 = top::make_object_ptr<xderived_object1>();
    ASSERT_EQ(1, derived_obj1->get_refcount());

    {
        auto base_obj1_casted = top::static_xobject_ptr_cast<xbase_object1>(derived_obj1);
        ASSERT_EQ(2, base_obj1_casted->get_refcount());

        auto derived_obj1_casted = top::dynamic_xauto_ptr_cast<xderived_object1>(base_obj1_casted);

        ASSERT_EQ(3, derived_obj1_casted->get_refcount());
        ASSERT_EQ(derived_obj1->get_refcount(), derived_obj1_casted->get_refcount());
    }

    ASSERT_EQ(1, derived_obj1->get_refcount());
}
#endif

NS_END3
