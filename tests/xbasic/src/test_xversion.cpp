#include <gtest/gtest.h>
#include "xbasic/xversion.h"
#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbasic/xserialize_face.h"

enum class test_enum : uint16_t {
    none,
    first,
    second,
    third,
    forth
};

struct NORMAL_STRUCT : public top::basic::xserialize_face_t {
    NORMAL_STRUCT() = default;
    NORMAL_STRUCT(uint8_t d) : b_data(d) {}

    int32_t do_write(top::base::xstream_t & stream) override {
        KEEP_SIZE();
        SERIALIZE_FIELD_BT(b_data);
        return CALC_LEN();
    }

    int32_t do_read(top::base::xstream_t & stream) override {
        KEEP_SIZE();
        DESERIALIZE_FIELD_BT(b_data);
        return CALC_LEN();
    }

    uint8_t  b_data{};

protected:
    ~NORMAL_STRUCT() = default;
};

class VERSION1 : public NORMAL_STRUCT {
public:
    int32_t do_write(top::base::xstream_t & stream) override {
        KEEP_SIZE();
        NORMAL_STRUCT::do_write(stream);
        SERIALIZE_FIELD_BT(e_data);
        SERIALIZE_FIELD_BT(i_data);

        // vector
        SERIALIZE_CONTAINER(list) {
            DEFAULT_SERIALIZE_PTR(item);
        }

        // map
        SERIALIZE_CONTAINER(map) {
            SERIALIZE_FIELD_BT(item.first);
            DEFAULT_SERIALIZE_PTR(item.second);
        }

        return CALC_LEN();
    }

    int32_t do_read(top::base::xstream_t & stream) override {
        KEEP_SIZE();
        NORMAL_STRUCT::do_read(stream);
        DESERIALIZE_FIELD_BT(e_data);
        DESERIALIZE_FIELD_BT(i_data);

        // vector
        DESERIALIZE_CONTAINER(list) {
            DESERIALIZE_PTR(ns) {
                auto ns = new NORMAL_STRUCT();
                ns->serialize_from(stream);
                list.push_back(ns);
            };
        }

        // map
        DESERIALIZE_CONTAINER(map) {
            int key;
            top::xobject_ptr_t<NORMAL_STRUCT> value;

            DESERIALIZE_FIELD_BT(key);
            DEFAULT_DESERIALIZE_PTR(value, NORMAL_STRUCT);
            map[key] = value;
        }

        return CALC_LEN();
    }

public: // can be protected or private
    test_enum e_data{};
    uint32_t  i_data{};
    std::vector<NORMAL_STRUCT*>      list;
    std::map<int, top::xobject_ptr_t<NORMAL_STRUCT>>   map;

protected:
    ~VERSION1() override {
        for (auto * i : list) {
            if (i != nullptr) {
                i->release_ref();
            }
        }
    }
};

class VERSION2 : public NORMAL_STRUCT {
public:
    int32_t do_write(top::base::xstream_t & stream) override {
        KEEP_SIZE();
        NORMAL_STRUCT::do_write(stream);
        SERIALIZE_FIELD_BT(e_data);
        SERIALIZE_FIELD_BT(i_data);

        // vector
        SERIALIZE_CONTAINER(list) {
            DEFAULT_SERIALIZE_PTR(item);
        }

        // map
        SERIALIZE_CONTAINER(map) {
            SERIALIZE_FIELD_BT(item.first);
            DEFAULT_SERIALIZE_PTR(item.second);
        }

        SERIALIZE_FIELD_BT(li_data);
        DEFAULT_SERIALIZE_PTR(raw_ptr);
        DEFAULT_SERIALIZE_PTR(obj_ptr);

        return CALC_LEN();
    }

    int32_t do_read(top::base::xstream_t & stream) override {
        KEEP_SIZE();
        NORMAL_STRUCT::do_read(stream);
        DESERIALIZE_FIELD_BT(e_data);
        DESERIALIZE_FIELD_BT(i_data);

        // vector
        DESERIALIZE_CONTAINER(list) {
            DESERIALIZE_PTR(ns) {
                auto ns = new NORMAL_STRUCT();
                ns->serialize_from(stream);
                list.push_back(ns);
            };
        }

        // map
        DESERIALIZE_CONTAINER(map) {
            int key;
            top::xobject_ptr_t<NORMAL_STRUCT> value;

            DESERIALIZE_FIELD_BT(key);
            DEFAULT_DESERIALIZE_PTR(value, NORMAL_STRUCT);
            map[key] = value;
        }

        // for compatibility
        VERSION_COMPATIBILITY {
            DESERIALIZE_FIELD_BT(li_data);
        }

        // for compatibility
        VERSION_COMPATIBILITY {
            DESERIALIZE_PTR(raw_ptr) {
                raw_ptr = new NORMAL_STRUCT();
                raw_ptr->serialize_from(stream);
            }
        }

        // for compatibility
        VERSION_COMPATIBILITY {
            DEFAULT_DESERIALIZE_PTR(obj_ptr, NORMAL_STRUCT);
        }

        return CALC_LEN();
    }

public: // can be protected or private
    test_enum e_data{};
    uint32_t  i_data{};
    std::vector<NORMAL_STRUCT*>      list;
    std::map<int, top::xobject_ptr_t<NORMAL_STRUCT>>   map;

    // extending by more field
    uint64_t li_data{};
    NORMAL_STRUCT* raw_ptr{};
    top::xobject_ptr_t<NORMAL_STRUCT> obj_ptr{};

protected:
    ~VERSION2() override {
        if (raw_ptr != nullptr) {
            raw_ptr->release_ref();
        }

        for (auto * i : list) {
            if (i != nullptr) {
                i->release_ref();
            }
        }
    }
};


TEST(xversion_t, tests) {
    {
        top::xobject_ptr_t<VERSION1> v1{top::make_object_ptr<VERSION1>()};
        v1->e_data = test_enum::third;
        v1->b_data = 1;
        v1->i_data = 2;
        v1->list.push_back(new NORMAL_STRUCT{3});
        v1->map[1] = top::make_object_ptr<NORMAL_STRUCT>(static_cast<uint8_t>(4));
        ASSERT_EQ(1, v1->map[1]->get_refcount());

        {
            top::xobject_ptr_t<VERSION1> v11{top::make_object_ptr<VERSION1>()};
            ASSERT_EQ(1, v11->get_refcount());
            top::base::xstream_t s1(top::base::xcontext_t::instance());
            v1->serialize_to(s1);
            v11->serialize_from(s1);

            ASSERT_TRUE(s1.size() == 0);
            ASSERT_TRUE(v11->e_data == test_enum::third);
            ASSERT_TRUE(v11->b_data == 1);
            ASSERT_TRUE(v11->i_data == 2);
            ASSERT_TRUE(v11->list.size() == 1);
            ASSERT_TRUE(v11->list[0]->b_data == 3);
            ASSERT_TRUE(v11->map.size() == 1);
            ASSERT_TRUE(v11->map[1]->b_data == 4);
        }

        {
            top::xobject_ptr_t<VERSION2> v22{top::make_object_ptr<VERSION2>()};
            // version compatibility 1
            top::base::xstream_t s3(top::base::xcontext_t::instance());
            v1->serialize_to(s3);
            v22->serialize_from(s3);

            ASSERT_TRUE(s3.size() == 0);
            ASSERT_TRUE(v22->e_data == test_enum::third);
            ASSERT_TRUE(v22->b_data == 1);
            ASSERT_TRUE(v22->i_data == 2);
            ASSERT_TRUE(v22->li_data == 0);
            ASSERT_TRUE(v22->raw_ptr == nullptr);
            ASSERT_TRUE(v22->obj_ptr == nullptr);
            ASSERT_TRUE(v22->list.size() == 1);
            ASSERT_TRUE(v22->list[0]->b_data == 3);
            ASSERT_TRUE(v22->map.size() == 1);
            ASSERT_TRUE(v22->map[1]->b_data == 4);
        }
    }

    {
        top::xobject_ptr_t<VERSION2> v2{top::make_object_ptr<VERSION2>()};
        v2->e_data = test_enum::first;
        v2->b_data = 1;
        v2->i_data = 2;
        v2->li_data = 3;
        v2->raw_ptr = new VERSION1();
        v2->raw_ptr->b_data = 4;
        v2->obj_ptr = top::make_object_ptr<VERSION1>();
        v2->obj_ptr->b_data = 6;

        {
            top::xobject_ptr_t<VERSION2> v21{top::make_object_ptr<VERSION2>()};
            top::base::xstream_t s2(top::base::xcontext_t::instance());
            v2->serialize_to(s2);
            v21->serialize_from(s2);

            ASSERT_TRUE(s2.size() == 0);
            ASSERT_TRUE(v21->e_data == test_enum::first);
            ASSERT_TRUE(v21->b_data == 1);
            ASSERT_TRUE(v21->i_data == 2);
            ASSERT_TRUE(v21->li_data == 3);
            ASSERT_TRUE(v21->raw_ptr->b_data == 4);
            ASSERT_TRUE(v21->obj_ptr->b_data == 6);
            ASSERT_TRUE(v21->list.empty());
            ASSERT_TRUE(v21->map.empty());
        }

        {
            top::xobject_ptr_t<VERSION1> v12{top::make_object_ptr<VERSION1>()};
            // version compatibility forward
            top::base::xstream_t s4(top::base::xcontext_t::instance());
            v2->serialize_to(s4);
            v12->serialize_from(s4);

            ASSERT_TRUE(s4.size() == 0);
            ASSERT_TRUE(v12->e_data == test_enum::first);
            ASSERT_TRUE(v12->b_data == 1);
            ASSERT_TRUE(v12->i_data == 2);
            ASSERT_TRUE(v12->list.empty());
            ASSERT_TRUE(v12->map.empty());
        }
    }

    {
        // nullptr pointers
        top::xobject_ptr_t<VERSION2> v23{top::make_object_ptr<VERSION2>()}, v24{top::make_object_ptr<VERSION2>()};
        v23->e_data = test_enum::first;
        v23->b_data = 1;
        v23->i_data = 2;

        top::base::xstream_t s5(top::base::xcontext_t::instance());
        v23->serialize_to(s5);
        v24->serialize_from(s5);

        ASSERT_TRUE(s5.size() == 0);
        ASSERT_TRUE(v24->e_data == test_enum::first);
        ASSERT_TRUE(v24->b_data == 1);
        ASSERT_TRUE(v24->i_data == 2);
        ASSERT_TRUE(v24->li_data == 0);
        ASSERT_TRUE(v24->raw_ptr == nullptr);
        ASSERT_TRUE(v24->obj_ptr == nullptr);
        ASSERT_TRUE(v24->list.empty());
        ASSERT_TRUE(v24->map.empty());
    }
}
