// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xdata.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xserializable_based_on.h"
#include "xbasic/xobject_ptr.h"
#include "xcommon/xrole_type.h"

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

NS_BEG1(top)

using xdataunit_ptr_t = xobject_ptr_t<base::xdataunit_t>;
using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;
using xstring_ptr_t = xobject_ptr_t<base::xstring_t>;
using xstrdeque_ptr_t = xobject_ptr_t<base::xstrdeque_t>;
using xstrmap_ptr_t = xobject_ptr_t<base::xstrmap_t>;

NS_END1

NS_BEG2(top, data)

#define PAIR_SERIALIZE_SIMPLE(stream, pair)                                                                                                                                        \
    do {                                                                                                                                                                           \
        stream << pair.first;                                                                                                                                                      \
        stream << pair.second;                                                                                                                                                     \
    } while (0)

#define PAIR_DESERIALZE_SIMPLE(stream, pair, type)                                                                                                                                 \
    do {                                                                                                                                                                           \
        type _first, _second;                                                                                                                                                      \
        stream >> _first;                                                                                                                                                          \
        stream >> _second;                                                                                                                                                         \
        pair = {_first, _second};                                                                                                                                                  \
    } while (0)

#define VECTOR_SERIALIZE_SIMPLE(stream, vec)                                                                                                                                       \
    do {                                                                                                                                                                           \
        auto const count = static_cast<std::int32_t>(vec.size());                                                                                                                  \
        stream << count;                                                                                                                                                           \
        for (auto const & v : vec) {                                                                                                                                               \
            stream << v;                                                                                                                                                           \
        }                                                                                                                                                                          \
    } while (0)

#define VECTOR_DESERIALZE_SIMPLE(stream, vec, type)                                                                                                                                \
    do {                                                                                                                                                                           \
        int32_t count;                                                                                                                                                             \
        stream >> count;                                                                                                                                                           \
        for (auto i = 0; i < count; i++) {                                                                                                                                         \
            type value;                                                                                                                                                            \
            stream >> value;                                                                                                                                                       \
            vec.emplace_back(std::move(value));                                                                                                                                    \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_SERIALIZE_SIMPLE(STREAM, MAP)                                                                                                                                          \
    do {                                                                                                                                                                           \
        auto const count = static_cast<std::uint32_t>(MAP.size());                                                                                                                 \
        STREAM << count;                                                                                                                                                           \
        for (auto const & iter : MAP) {                                                                                                                                            \
            STREAM << iter.first;                                                                                                                                                  \
            STREAM << iter.second;                                                                                                                                                 \
        }                                                                                                                                                                          \
    } while (false)

#define MAP_DESERIALIZE_SIMPLE(STREAM, MAP)                                                                                                                                        \
    do {                                                                                                                                                                           \
        using map_t = typename std::remove_reference<typename std::remove_cv<decltype(MAP)>::type>::type;                                                                          \
        using key_t = typename map_t::key_type;                                                                                                                                    \
        using val_t = typename map_t::mapped_type;                                                                                                                                 \
        std::uint32_t count;                                                                                                                                                       \
        STREAM >> count;                                                                                                                                                           \
        for (uint32_t i = 0; i < count; i++) {                                                                                                                                     \
            key_t key;                                                                                                                                                             \
            val_t value;                                                                                                                                                           \
            STREAM >> key;                                                                                                                                                         \
            STREAM >> value;                                                                                                                                                       \
            MAP.insert({std::move(key), std::move(value)});                                                                                                                        \
        }                                                                                                                                                                          \
    } while (false)

#define MAP_OBJECT_SERIALIZE(STREAM, MAP)                                                                                                                                          \
    do {                                                                                                                                                                           \
        int32_t count = (int32_t)MAP.size();                                                                                                                                       \
        STREAM << count;                                                                                                                                                           \
        for (auto & iter : MAP) {                                                                                                                                                  \
            STREAM << iter.first;                                                                                                                                                  \
            iter.second.serialize(STREAM);                                                                                                                                         \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_DESERIALZE(STREAM, MAP, type_k, type_v)                                                                                                                         \
    do {                                                                                                                                                                           \
        int32_t count;                                                                                                                                                             \
        STREAM >> count;                                                                                                                                                           \
        for (int32_t i = 0; i < count; i++) {                                                                                                                                      \
            type_k key;                                                                                                                                                            \
            type_v value;                                                                                                                                                          \
            STREAM >> key;                                                                                                                                                         \
            value.deserialize(STREAM);                                                                                                                                             \
            MAP.emplace(std::make_pair(std::move(key), std::move(value)));                                                                                                         \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_SERIALIZE2(STREAM, MAP)                                                                                                                                         \
    do {                                                                                                                                                                           \
        int32_t count = (int32_t)MAP.size();                                                                                                                                       \
        STREAM << count;                                                                                                                                                           \
        for (auto const & iter : MAP) {                                                                                                                                            \
            STREAM << iter.first;                                                                                                                                                  \
            iter.second.serialize_to(STREAM);                                                                                                                                      \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_DESERIALZE2(STREAM, MAP)                                                                                                                                        \
    do {                                                                                                                                                                           \
        using map_t = typename std::remove_reference<typename std::remove_cv<decltype(MAP)>::type>::type;                                                                          \
        using key_t = typename map_t::key_type;                                                                                                                                    \
        using val_t = typename map_t::mapped_type;                                                                                                                                 \
        int32_t count;                                                                                                                                                             \
        STREAM >> count;                                                                                                                                                           \
        for (int32_t i = 0; i < count; i++) {                                                                                                                                      \
            key_t key;                                                                                                                                                             \
            val_t value;                                                                                                                                                           \
            STREAM >> key;                                                                                                                                                         \
            value.serialize_from(STREAM);                                                                                                                                          \
            MAP.emplace(std::make_pair(std::move(key), std::move(value)));                                                                                                         \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_SERIALIZE3(STREAM, MAP)                                                                                                                                         \
    do {                                                                                                                                                                           \
        auto const count = static_cast<std::uint32_t>(MAP.size());                                                                                                                 \
        STREAM << count;                                                                                                                                                           \
        for (auto const & p : MAP) {                                                                                                                                               \
            p.first.serialize_to(STREAM);                                                                                                                                          \
            p.second.serialize_to(STREAM);                                                                                                                                         \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_DESERIALZE3(STREAM, MAP)                                                                                                                                        \
    do {                                                                                                                                                                           \
        using map_t = typename std::remove_reference<typename std::remove_cv<decltype(MAP)>::type>::type;                                                                          \
        using key_t = typename map_t::key_type;                                                                                                                                    \
        using val_t = typename map_t::mapped_type;                                                                                                                                 \
        uint32_t count;                                                                                                                                                            \
        STREAM >> count;                                                                                                                                                           \
        for (auto i = 0u; i < count; i++) {                                                                                                                                        \
            key_t key;                                                                                                                                                             \
            val_t value;                                                                                                                                                           \
            key.serialize_from(STREAM);                                                                                                                                            \
            value.serialize_from(STREAM);                                                                                                                                          \
            MAP.insert({std::move(key), std::move(value)});                                                                                                                        \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_SERIALIZE4(STREAM, MAP)                                                                                                                                         \
    do {                                                                                                                                                                           \
        auto const count = static_cast<std::uint32_t>(MAP.size());                                                                                                                 \
        STREAM << count;                                                                                                                                                           \
        for (auto const & p : MAP) {                                                                                                                                               \
            STREAM << p.first;                                                                                                                                                     \
            STREAM << p.second;                                                                                                                                                    \
        }                                                                                                                                                                          \
    } while (0)

#define MAP_OBJECT_DESERIALZE4(STREAM, MAP)                                                                                                                                        \
    do {                                                                                                                                                                           \
        using map_t = typename std::remove_reference<typename std::remove_cv<decltype(MAP)>::type>::type;                                                                          \
        using key_t = typename map_t::key_type;                                                                                                                                    \
        using val_t = typename map_t::mapped_type;                                                                                                                                 \
        uint32_t count;                                                                                                                                                            \
        STREAM >> count;                                                                                                                                                           \
        for (auto i = 0u; i < count; i++) {                                                                                                                                        \
            key_t key;                                                                                                                                                             \
            val_t value;                                                                                                                                                           \
            STREAM >> key;                                                                                                                                                         \
            STREAM >> value;                                                                                                                                                       \
            MAP.insert({std::move(key), std::move(value)});                                                                                                                        \
        }                                                                                                                                                                          \
    } while (0)

#define VECTOR_OBJECT_SERIALIZE(STREAM, VECTOR)                                                                                                                                    \
    do {                                                                                                                                                                           \
        auto const count = static_cast<std::uint32_t>(VECTOR.size());                                                                                                              \
        STREAM << count;                                                                                                                                                           \
        for (auto & v : VECTOR) {                                                                                                                                                  \
            v.serialize(STREAM);                                                                                                                                                   \
        }                                                                                                                                                                          \
    } while (false)

#define VECTOR_OBJECT_SERIALIZE2(STREAM, VECTOR)                                                                                                                                   \
    do {                                                                                                                                                                           \
        auto const count = static_cast<std::uint32_t>(VECTOR.size());                                                                                                              \
        STREAM << count;                                                                                                                                                           \
        for (auto const & v : VECTOR) {                                                                                                                                            \
            v.serialize_to(STREAM);                                                                                                                                                \
        }                                                                                                                                                                          \
    } while (false)

#define VECTOR_OBJECT_DESERIALZE(STREAM, VECTOR)                                                                                                                                   \
    do {                                                                                                                                                                           \
        using value_t = decltype(VECTOR)::value_type;                                                                                                                              \
        std::uint32_t count;                                                                                                                                                       \
        STREAM >> count;                                                                                                                                                           \
        for (std::uint32_t i = 0; i < count; i++) {                                                                                                                                \
            value_t value;                                                                                                                                                         \
            value.deserialize(STREAM);                                                                                                                                             \
            VECTOR.push_back(std::move(value));                                                                                                                                    \
        }                                                                                                                                                                          \
    } while (false)

#define VECTOR_OBJECT_DESERIALZE2(STREAM, VECTOR)                                                                                                                                  \
    do {                                                                                                                                                                           \
        using value_t = decltype(VECTOR)::value_type;                                                                                                                              \
        std::uint32_t count;                                                                                                                                                       \
        STREAM >> count;                                                                                                                                                           \
        for (std::uint32_t i = 0; i < count; i++) {                                                                                                                                \
            value_t value;                                                                                                                                                         \
            value.serialize_from(STREAM);                                                                                                                                          \
            VECTOR.push_back(std::move(value));                                                                                                                                    \
        }                                                                                                                                                                          \
    } while (false)

#define ENUM_SERIALIZE(STREAM, enum_value) STREAM << static_cast<std::underlying_type<decltype(enum_value)>::type>(enum_value)

#define ENUM_DESERIALIZE(STREAM, value)                                                                                                                                            \
    do {                                                                                                                                                                           \
        std::underlying_type<decltype(value)>::type temp;                                                                                                                          \
        STREAM >> temp;                                                                                                                                                            \
        value = static_cast<decltype(value)>(temp);                                                                                                                                \
    } while (0)

//
// node info
//
class node_info_t {
public:
    node_info_t() = default;
    node_info_t(node_info_t const &) = default;
    node_info_t & operator=(node_info_t const &) = default;
    node_info_t(node_info_t &&) = default;
    node_info_t & operator=(node_info_t &&) = default;
    ~node_info_t() = default;

    node_info_t(std::string account, std::string public_key)
      : m_account{std::move(account)}, m_publickey{std::move(public_key)} {}

    void serialize(base::xstream_t & stream) {
        stream << m_account;
        stream << m_publickey;
    }

    void deserialize(base::xstream_t & stream) {
        stream >> m_account;
        stream >> m_publickey;
    }

public:
    std::string m_account;                                            // account for the node
    xpublic_key_t m_publickey;                                          // node public key(hex)
};

// register xobject
template <class data_cls>
class register_xcls {
public:
    register_xcls() { top::base::xcontext_t::register_xobject((top::base::enum_xobject_type)data_cls::get_object_type(), data_cls::create_object); }
};

#ifndef REG_CLS
#    define REG_CLS(CLS)                                                                                                                                                           \
        using data::register_xcls;                                                                                                                                                 \
        register_xcls<CLS> g_##CLS
#endif  // REG_CLS

NS_END2
