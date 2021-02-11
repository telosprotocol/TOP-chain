// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#if defined XCXX17
#   include <optional>
#   define XSTD_OPTIONAL
#elif defined XCXX14
#   if __has_include(<optional>)
#       include <optional>
#   elif __has_include(<experimental/optional>)
#       include <experimental/optional>
#       define XEXPERIMENTAL_OPTIONAL
#   else
#       error "Cannot find <optional> or <experimental/optional>"
#   endif
#else
#   include <exception>
#   include <memory>
#endif

#include "xbasic/xmemory.hpp"

#include <string>
#include <type_traits>
#include <utility>

NS_BEG1(top)

#if defined XSTD_OPTIONAL

template <typename T>
using optional = std::optional<T>;

using std::nullopt_t;
XINLINE_CONSTEXPR nullopt_t nullopt = std::nullopt;

using std::in_place_t;
XINLINE_CONSTEXPR in_place_t in_place = std::in_place;

using std::bad_optional_access;

template <typename T>
constexpr
optional<std::decay_t<T>>
make_optional(T && value) {
    return std::make_optional<T>(std::forward<T>(value));
}

template <typename T, typename ... Args>
constexpr
optional<T>
make_optional(Args &&... args) {
    return std::make_optional<T, Args...>(std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
constexpr
optional<T>
make_optional(std::initializer_list<U> il, Args &&... args) {
    return std::make_optional<T, U, Args...>(il, std::forward<Args>(args)...);
}

#elif defined XEXPERIMENTAL_OPTIONAL

template <typename T>
using optional = std::experimental::optional<T>;

using std::experimental::nullopt_t;
XINLINE_CONSTEXPR nullopt_t nullopt = std::experimental::nullopt;

using std::experimental::in_place_t;
XINLINE_CONSTEXPR in_place_t in_place = std::experimental::in_place;

using std::experimental::bad_optional_access;

template <typename T>
constexpr
optional<std::decay_t<T>>
make_optional(T && value) {
    return std::experimental::make_optional<T>(std::forward<T>(value));
}

template <typename T, typename ... Args>
constexpr
optional<T>
make_optional(Args &&... args) {
    return optional<T>(in_place, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
constexpr
optional<T>
make_optional(std::initializer_list<U> il, Args &&... args) {
    return optional<T>(in_place, il, std::forward<Args>(args)...);
}

#else

struct nullopt_t
{
    enum class _Construct {
        _Token
    };

    explicit
    constexpr
    nullopt_t(_Construct) {
    }
};

XINLINE_CONSTEXPR nullopt_t nullopt { nullopt_t::_Construct::_Token };

class bad_optional_access : public std::exception {
public:
    virtual
    const char *
    what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class optional final
{
    std::unique_ptr<T> m_value{};

public:

    using value_type = T;

    constexpr
    optional() noexcept = default;

    constexpr
    optional(nullopt_t) noexcept {
    }

    optional(optional const & other)
        : m_value{ static_cast<bool>(other) ? top::make_unique<T>(*other) : nullptr }
    {
    }

    optional(optional && other) noexcept
        : m_value{ std::move(other.m_value) }
    {
    }

    template <typename U = value_type>
    optional(U && v) : m_value{ top::make_unique<T>(std::forward<U>(v)) }
    {
    }

    ~optional() = default;

    optional &
    operator=(nullopt_t) noexcept {
        m_value.reset();
        return *this;
    }

    optional &
    operator=(optional const & other) {
        if (this != &other) {
            if (static_cast<bool>(other)) {
                m_value = top::make_unique<T>(*other);
            } else {
                m_value.reset();
            }
        }
        return *this;
    }

    optional &
    operator=(optional && other) {
       if (this != &other) {
           if (static_cast<bool>(other)) {
               m_value = std::move(other.m_value);
           } else {
               m_value.reset();
           }
       }

       return *this;
    }

    template <typename U>
    optional &
    operator=(U && v) {
       if (m_value == nullptr) {
           m_value = top::make_unique<U>(std::move(v));
       } else {
           *m_value = std::move(v);
       }

       return *this;
    }

    const T *
    operator->() const {
        return m_value.get();
    }

    T *
    operator->() {
        return m_value.get();
    }

    const T &
    operator*() const & {
        return *m_value;
    }

    T & operator*() & {
        return *m_value;
    }

    const T &&
    operator*() const && {
        return std::move(*m_value);
    }

    T &&
    operator*() && {
        return std::move(*m_value);
    }

    constexpr
    explicit
    operator bool() const noexcept {
        return m_value != nullptr;
    }

    constexpr
    bool
    has_value() const noexcept {
        return m_value != nullptr;
    }

    T &
    value() & {
        if (!has_value()) {
            throw bad_optional_access{};
        }

        return *m_value;
    }

    const T &
    value() const & {
        if (!has_value()) {
            throw bad_optional_access{};
        }

        return *m_value;
    }

    T && value() && {
        if (!has_value()) {
            throw bad_optional_access{};
        }

        return std::move(*m_value);
    }

    const T &&
    value() const && {
        if (!has_value()) {
            throw bad_optional_access{};
        }

        return std::move(*m_value);
    }

    template <typename U >
    T
    value_or(U && default_value) const & {
        static_assert(std::is_copy_constructible<T>::value,
                      "The const overload of optional<T>::value_or requires T to be copy constructible "
                      "(N4659 23.6.3.5 [optional.observe]/18).");
        static_assert(std::is_convertible<U, T>::value,
                      "optional<T>::value_or(U) requires U to be convertible to T (N4659 23.6.3.5 [optional.observe]/18).");

        if (has_value()) {
            return *m_value;
        }

        return static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    T
    value_or(U && default_value) && {
        static_assert(std::is_move_constructible<T>::value,
                      "The rvalue overload of optional<T>::value_or requires T to be move constructible "
                      "(N4659 23.6.3.5 [optional.observe]/20).");
        static_assert(std::is_convertible<U, T>::value,
                      "optional<T>::value_or(U) requires U to be convertible to T (N4659 23.6.3.5 [optional.observe]/20).");

        if (has_value()) {
            return std::move(*m_value);
        }

        return static_cast<T>(std::forward<U>(default_value));
    }

    void
    swap(optional & other) noexcept(std::is_nothrow_move_constructible<T>::value) { // exchange state with other
        m_value.swap(other.m_value);
    }

    void
    reset() noexcept {
        m_value.reset();
    }
};

template<typename T, typename U>
bool
operator==(optional<T> const & lhs, optional<U> const & rhs) {
    if (static_cast<bool>(lhs) != static_cast<bool>(rhs)) { return false; }
    if (static_cast<bool>(lhs) == false) { return true; }
    return *lhs == *rhs;
}

template<typename T, typename U>
bool
operator<(optional<T> const & lhs, optional<U> const & rhs) {
    if (static_cast<bool>(rhs) == false) { return false; }
    if (static_cast<bool>(lhs) == false) { return true; }
    return *lhs < *rhs;
}

#endif

template <typename T>
bool
has_value(optional<T> const & v) noexcept
{
#if defined XEXPERIMENTAL_OPTIONAL
    return static_cast<bool>(v);
#else
    return v.has_value();
#endif
}

template <typename T>
T const &
value(optional<T> const & v)
{
    return v.value();
}

template <typename T>
T &
value(optional<T> & v)
{
    return v.value();
}

template <typename T>
T &&
value(optional<T> && v)
{
    return std::move(v).value();
}

template <typename T>
T const &&
value(optional<T> const && v)
{
    return std::move(v).value();
}

template <typename T>
void
reset(optional<T> & v)
{
#if defined XEXPERIMENTAL_OPTIONAL
  v = nullopt;
#else
  v.reset();
#endif
}

NS_END1
