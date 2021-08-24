// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xenable_to_string.h"
#include "xbasic/xhashable.hpp"
#include "xbasic/xserializable_based_on.h"

#include <atomic>
#include <cassert>
#include <exception>
#include <functional>
#include <limits>
#include <ostream>
#include <string>
#include <type_traits>

NS_BEG1(top)

/**
 * @brief The id class.  std::uint64_t is the back.
 *
 */
class xtop_bad_id_access : public std::exception {
public:
    const char *
    what() const noexcept override {
        return "Bad xnullable_id_t access";
    }
};
using xbad_id_access_t = xtop_bad_id_access;

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
class xtop_simple_id final {
    XSTATIC_ASSERT(std::is_integral<IdT>::value);

public:
    using tag_type = TagT;
    using value_type = IdT;

private:
    value_type m_id{MaxValue};

public:
    constexpr xtop_simple_id() = default;
    xtop_simple_id(xtop_simple_id const &) = default;
    xtop_simple_id & operator=(xtop_simple_id const &) = default;
    xtop_simple_id(xtop_simple_id &&) = default;
    xtop_simple_id & operator=(xtop_simple_id &&) = default;
    ~xtop_simple_id() = default;

    constexpr explicit xtop_simple_id(value_type const raw_value) noexcept
        : m_id{ raw_value } {
    }

    void swap(xtop_simple_id & other) noexcept {
        std::swap(m_id, other.m_id);
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool operator==(xtop_simple_id const & other) const noexcept {
        return m_id == other.m_id;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool operator!=(xtop_simple_id const & other) const noexcept {
        return !(*this == other);
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool operator<(xtop_simple_id const & other) const noexcept {
        return m_id < other.m_id;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool operator>(xtop_simple_id const & other) const noexcept {
        return other < *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool operator<=(xtop_simple_id const & other) const noexcept {
        return !(other < *this);
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool operator>=(xtop_simple_id const & other) const noexcept {
        return !(*this < other);
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id operator++(int) {
        xtop_simple_id ret{ *this };
        operator++();
        return ret;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id & operator++() {
        ++m_id;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id operator--(int) {
        xtop_simple_id ret{ *this };
        operator--();
        return ret;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id & operator--() {
        --m_id;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    explicit operator value_type() const {
        return value();
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    value_type value() const {
        return m_id;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id & operator|=(xtop_simple_id const & other) noexcept {
        m_id |= other.m_id;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id operator|(xtop_simple_id const & other) const noexcept {
        auto ret = *this;
        return ret |= other;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id & operator&=(xtop_simple_id const & other) noexcept {
        m_id &= other.m_id;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_simple_id operator&(xtop_simple_id const & other) const noexcept {
        auto ret = *this;
        return ret &= other;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    uint64_t hash() const {
        return static_cast<uint64_t>(m_id);
    }

    std::string to_string() const {
        return std::to_string(m_id);
    }

    static constexpr xtop_simple_id max() noexcept {
        return xtop_simple_id{ MaxValue };
    }

    static constexpr xtop_simple_id min() noexcept {
        return xtop_simple_id{ MinValue };
    }

    int32_t serialize_to(base::xstream_t & stream) const {
        return do_write(stream);
    }

    int32_t serialize_from(base::xstream_t & stream) {
        return do_read(stream);
    }

    int32_t serialize_to(base::xbuffer_t & buffer) const {
        return do_write(buffer);
    }

    int32_t serialize_from(base::xbuffer_t & buffer) {
        return do_read(buffer);
    }

private:
    std::int32_t do_read(base::xstream_t & stream) {
        auto const begin_size = stream.size();
        stream >> m_id;
        return begin_size - stream.size();
    }

    std::int32_t do_write(base::xstream_t & stream) const {
        auto const begin_size = stream.size();
        stream << m_id;
        return stream.size() - begin_size;
    }

    std::int32_t do_read(base::xbuffer_t & buffer) {
        auto const begin_size = buffer.size();
        buffer >> m_id;
        return begin_size - buffer.size();
    }

    std::int32_t do_write(base::xbuffer_t & buffer) const {
        auto const begin_size = buffer.size();
        buffer << m_id;
        return buffer.size() - begin_size;
    }
};

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
using xsimple_id_t = xtop_simple_id<TagT, IdT, MinValue, MaxValue>;

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
std::int32_t operator<<(base::xstream_t & stream, xtop_simple_id<TagT, IdT, MinValue, MaxValue> const & id) {
    return id.serialize_to(stream);
}

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
std::int32_t operator>>(base::xstream_t & stream, xtop_simple_id<TagT, IdT, MinValue, MaxValue> & id) {
    return id.serialize_from(stream);
}

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
std::int32_t operator<<(base::xbuffer_t & buffer, xtop_simple_id<TagT, IdT, MinValue, MaxValue> const & id) {
    return id.serialize_to(buffer);
}

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
std::int32_t operator>>(base::xbuffer_t & buffer, xtop_simple_id<TagT, IdT, MinValue, MaxValue> & id) {
    return id.serialize_from(buffer);
}

//template <typename TagT, typename IdT>
//struct xtop_id_factory final
//{
//    XSTATIC_ASSERT(std::is_integral<IdT>::value);
//
//    static
//    xsimple_id_t<TagT, IdT>
//    new_id() {
//        static std::atomic<IdT> next_id{ 1 };
//        return xsimple_id_t<TagT, IdT>{ next_id.fetch_add(1, std::memory_order_relaxed) };
//    }
//};
//
//template <typename TagT, typename IdT = std::uint64_t>
//using xid_factory_t = xtop_id_factory<TagT, IdT>;

//template <typename TagT, typename IdT>
//class xtop_nullable_id final : public xhashable_t<xtop_nullable_id<TagT, IdT>>
//                             , public xenable_to_string_t<xtop_nullable_id<TagT, IdT>>
//                             , public xserializable_based_on<void>
//{
//    XSTATIC_ASSERT(std::is_integral<IdT>::value);
//
//public:
//    using tag_type = TagT;
//    using value_type = IdT;
//    using hash_result_type = typename xhashable_t<xtop_nullable_id<TagT, IdT>>::hash_result_type;
//
//    XSTATIC_ASSERT(std::is_integral<hash_result_type>::value && std::is_unsigned<hash_result_type>::value);
//
//private:
//    value_type m_id{};
//    bool m_initialized{ false };
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    value_type
//    get_value() const {
//        if (!m_initialized) {
//            throw xbad_id_access_t{};
//        }
//
//        return m_id;
//    }
//
//public:
//    constexpr xtop_nullable_id()                  = default;
//    xtop_nullable_id(xtop_nullable_id const &)             = default;
//    xtop_nullable_id & operator=(xtop_nullable_id const &) = default;
//    xtop_nullable_id(xtop_nullable_id &&)                  = default;
//    xtop_nullable_id & operator=(xtop_nullable_id &&)      = default;
//    ~xtop_nullable_id()                           = default;
//
//    constexpr
//    explicit
//    xtop_nullable_id(value_type const raw_value) noexcept
//        : m_id{ raw_value }, m_initialized{ true } {
//    }
//
//    void
//    swap(xtop_nullable_id & other) noexcept {
//        std::swap(m_id, other.m_id);
//        std::swap(m_initialized, other.m_initialized);
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    operator==(xtop_nullable_id const & other) const noexcept {
//        if (m_initialized != other.m_initialized) {
//            return false;
//        }
//
//        if (!m_initialized) {
//            assert(!other.m_initialized);
//            return true;
//        }
//
//        return m_id == other.m_id;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    operator!=(xtop_nullable_id const & other) const noexcept {
//        return !(*this == other);
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    operator<(xtop_nullable_id const & other) const noexcept {
//        if (!other.m_initialized) {
//            return false;
//        }
//
//        if (!m_initialized) {
//            return true;
//        }
//
//        return m_id < other.m_id;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    operator>(xtop_nullable_id const & other) const noexcept {
//        return other < *this;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    operator<=(xtop_nullable_id const & other) const noexcept {
//        return !(other < *this);
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    operator>=(xtop_nullable_id const & other) const noexcept {
//        return !(*this < other);
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id
//    operator++(int) {
//        xtop_nullable_id ret{ *this };
//        operator++();
//        return ret;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id &
//    operator++() {
//        if (!has_value()) {
//            throw xbad_id_access_t{};
//        }
//
//        ++m_id;
//        return *this;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id
//    operator--(int) {
//        xtop_nullable_id ret{ *this };
//        operator--();
//        return ret;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id &
//    operator--() {
//        if (!has_value()) {
//            throw xbad_id_access_t{};
//        }
//
//        --m_id;
//        return *this;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    explicit
//    operator value_type() const {
//        return value();
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    explicit
//    operator bool() const noexcept {
//        return m_initialized;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    value_type
//    value() const {
//        return get_value();
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    value_type
//    value_or(value_type const & default_value) const {
//        if (has_value()) {
//            return m_id;
//        }
//        return default_value;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    empty() const noexcept {
//        return !has_value();
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    bool
//    has_value() const noexcept {
//        return m_initialized;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id &
//    operator|=(xtop_nullable_id const & other) noexcept {
//        if (empty()) {
//            *this = other;
//        } else if (!other.empty()) {
//            m_id |= other.m_id;
//        }
//
//        return *this;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id
//    operator|(xtop_nullable_id const & other) const noexcept {
//        auto ret = *this;
//        return ret |= other;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id &
//    operator&=(xtop_nullable_id const & other) noexcept {
//        if (empty() || other.empty()) {
//            m_initialized = false;
//        } else {
//            m_id &= other.m_id;
//        }
//
//        return *this;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    xtop_nullable_id
//    operator&(xtop_nullable_id const & other) const noexcept {
//        auto ret = *this;
//        return ret &= other;
//    }
//
//#if defined XCXX14_OR_ABOVE
//    constexpr
//#endif
//    hash_result_type
//    hash() const override {
//        if (has_value()) {
//            return static_cast<hash_result_type>(m_id);
//        } else {
//            return 0;
//        }
//    }
//
//    std::string
//    to_string() const override {
//        if (has_value()) {
//            return std::to_string(m_id);
//        }
//
//        return "(null)";
//    }
//
//    static
//    constexpr
//    xtop_nullable_id
//    max() noexcept {
//        return xtop_nullable_id{ std::numeric_limits<value_type>::max() };
//    }
//
//    static
//    constexpr
//    xtop_nullable_id
//    min() noexcept {
//        return xtop_nullable_id{ std::numeric_limits<value_type>::min() };
//    }
//
//    //friend
//    //std::int32_t
//    //operator<< <>(base::xstream_t & stream, xtop_nullable_id const & id);
//
//    //friend
//    //std::int32_t
//    //operator>> <>(base::xstream_t & stream, xtop_nullable_id & id);
//
//    //friend std::int32_t operator<<<>(base::xbuffer_t & buffer, xtop_nullable_id const & id);
//
//    //friend std::int32_t operator>><>(base::xbuffer_t & buffer, xtop_nullable_id & id);
//
//private:
//    std::int32_t
//    do_read(base::xstream_t & stream) override {
//        auto const begin_size = stream.size();
//        stream >> m_initialized;
//        if (m_initialized) {
//            stream >> m_id;
//        }
//        return begin_size - stream.size();
//    }
//
//    std::int32_t
//    do_write(base::xstream_t & stream) const override {
//        auto const begin_size = stream.size();
//        stream << m_initialized;
//        if (m_initialized) {
//            stream << m_id;
//        }
//        return stream.size() - begin_size;
//    }
//};
//
//template <typename TagT, typename IdT>
//using xnullable_id_t = xtop_nullable_id<TagT, IdT>;

//template <typename TagT, typename IdT>
//std::int32_t
//operator<<(base::xstream_t & stream, xtop_nullable_id<TagT, IdT> const & id) {
//    return id.serialize_to(stream);
//}
//
//template <typename TagT, typename IdT>
//std::int32_t
//operator>>(base::xstream_t & stream, xtop_nullable_id<TagT, IdT> & id) {
//    return id.serialize_from(stream);
//}
//
//template <typename TagT, typename IdT>
//std::int32_t operator<<(base::xbuffer_t & buffer, xtop_nullable_id<TagT, IdT> const & id) {
//    return id.serialize_to(buffer);
//}
//
//template <typename TagT, typename IdT>
//std::int32_t operator>>(base::xbuffer_t & buffer, xtop_nullable_id<TagT, IdT> & id) {
//    return id.serialize_from(buffer);
//}

NS_END1

NS_BEG1(std)

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
struct hash<top::xsimple_id_t<TagT, IdT, MinValue, MaxValue>> final
{
    std::size_t
    operator()(top::xsimple_id_t<TagT, IdT, MinValue, MaxValue> const & id) const noexcept {
        return static_cast<std::size_t>(id.hash());
    }
};

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
struct numeric_limits<top::xsimple_id_t<TagT, IdT, MinValue, MaxValue>> final
{
    static
    constexpr
    top::xsimple_id_t<TagT, IdT, MinValue, MaxValue>
    min() noexcept {
        return top::xsimple_id_t<TagT, IdT, MinValue, MaxValue>::min();
    }

    static
    constexpr
    top::xsimple_id_t<TagT, IdT, MinValue, MaxValue>
    max() noexcept {
        return top::xsimple_id_t<TagT, IdT, MinValue, MaxValue>::max();
    }
};

//template <typename TagT, typename IdT>
//struct hash<top::xnullable_id_t<TagT, IdT>> final
//{
//    std::size_t
//    operator()(top::xnullable_id_t<TagT, IdT> const & id) const noexcept {
//        return static_cast<std::size_t>(id.hash());
//    }
//};
//
//template <typename TagT, typename IdT>
//struct numeric_limits<top::xnullable_id_t<TagT, IdT>> final
//{
//    static
//    constexpr
//    top::xnullable_id_t<TagT, IdT>
//    min() noexcept {
//        return top::xnullable_id_t<TagT, IdT>::min();
//    }
//
//    static
//    constexpr
//    top::xnullable_id_t<TagT, IdT>
//    max() noexcept {
//        return top::xnullable_id_t<TagT, IdT>::max();
//    }
//};

NS_END1

template <typename TagT, typename IdT, IdT MinValue, IdT MaxValue>
std::ostream &
operator<<(std::ostream & o, top::xsimple_id_t<TagT, IdT, MinValue, MaxValue> const & id) {
    o << id.to_string();
    return o;
}

//template <typename TagT, typename IdT>
//std::ostream &
//operator<<(std::ostream & o, top::xnullable_id_t<TagT, IdT> const & id) {
//    o << id.to_string();
//    return o;
//}
