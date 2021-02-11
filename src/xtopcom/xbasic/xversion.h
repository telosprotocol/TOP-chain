// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define MAKE_RAW_PTR(T) (new T())
#define MAKE_SHARED_PTR(T) (std::make_shared<T>())
#define MAKE_OBJECT_PTR(T) (top::make_object_ptr<T>())

#define SERIALIZE_NONNULL_PTR(ptr) \
    ptr->serialize_to(stream)

#define DESERIALIZE_NONNULL_PTR(ptr, create_ptr_func) \
    ptr = create_ptr_func;\
    ptr->serialize_from(stream)

#define DEFAULT_DESERIALIZE_NONNULL_PTR(ptr, T) \
    DESERIALIZE_NONNULL_PTR(ptr, MAKE_OBJECT_PTR(T))

#define REFLECT_DESERIALIZE_NONNULL_PTR(ptr, T) \
    auto obj_ptr = T::read_from(stream);\
    ptr.attach(obj_ptr)

#define REFLECT_DESERIALIZE_NONNULL_OBJ_PTR(ptr) \
    REFLECT_DESERIALIZE_NONNULL_PTR(ptr, xdataobj_t)

#define REFLECT_DESERIALIZE_NONNULL_UNIT_PTR(ptr) \
    REFLECT_DESERIALIZE_NONNULL_PTR(ptr, xdatauint_t)

#define SERIALIZE_PTR(ptr)                          \
    stream << ((uint8_t)(ptr == nullptr ? 0 : 1));  \
    if (ptr != nullptr)

#define DESERIALIZE_PTR(ptr)                           \
    for(uint8_t null_sign=0, c=0; c==0 && stream >> null_sign && null_sign == 1; c++)

#define DEFAULT_SERIALIZE_PTR(ptr) \
    SERIALIZE_PTR(ptr) {\
        ptr->serialize_to(stream);\
    }

#define DEFAULT_DESERIALIZE_PTR(ptr, T) \
    DESERIALIZE_PTR() {\
        ptr = MAKE_OBJECT_PTR(T);\
        ptr->serialize_from(stream);\
    }

#define REFLECT_DESERIALIZE_PTR(ptr, T) \
    DESERIALIZE_PTR() {\
        auto obj_ptr = T::read_from(stream);\
        ptr.attach(obj_ptr);\
    }

#define REFLECT_DESERIALIZE_OBJ_PTR(ptr) \
    REFLECT_DESERIALIZE_PTR(ptr, xdataobj_t)

#define REFLECT_DESERIALIZE_UNIT_PTR(ptr) \
    REFLECT_DESERIALIZE_PTR(ptr, xdataunit_t)

#define SERIALIZE_FIELD_BT(var) \
    stream << var

#define DESERIALIZE_FIELD_BT(var) \
    stream >> var

#define SERIALIZE_FIELD_DU(var) \
    var.serialize_to(stream)

#define DESERIALIZE_FIELD_DU(var) \
    var.serialize_from(stream)

#define VERSION_COMPATIBILITY \
    if(stream.size() > 0)

#define SERIALIZE_CONTAINER(container)   \
    stream << ((uint32_t) container.size()); \
    for(auto& item : container)

#define DESERIALIZE_CONTAINER(container) \
    for(uint32_t i=0, c=0; c==0 && stream >> i;c++) \
        for(uint32_t j=0;j<i;j++)

#define KEEP_SIZE() \
    int32_t __start = stream.size()

#define CALC_LEN()  \
    std::abs(stream.size() - __start)

