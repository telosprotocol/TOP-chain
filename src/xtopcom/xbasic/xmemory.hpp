// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbasic/xobject_ptr.h"

#include <memory>
#include <type_traits>

NS_BEG1(top)

template <typename T, std::size_t alignment>
struct xtop_aligned_object final {
    XSTATIC_ASSERT(sizeof(T) <= alignment);
    alignas(alignment) T payload;
};

template <typename T, std::size_t alignment>
using xaligned_object_t = xtop_aligned_object<T, alignment>;

template <typename T>
using xcacheline_aligned_t = xaligned_object_t<T, _CONST_CPU_CACHE_LINE_BYTES_>;

template <typename T>
class observer_ptr
{
public:
    using element_type = T;
    using pointer_type = typename std::add_pointer<T>::type;
    using reference_type = typename std::add_lvalue_reference<T>::type;

private:
    pointer_type m_ptr{ nullptr };

public:
    constexpr observer_ptr()                       = default;
    observer_ptr(observer_ptr const &)             = default;
    observer_ptr & operator=(observer_ptr const &) = default;
    observer_ptr(observer_ptr &&)                  = default;
    observer_ptr & operator=(observer_ptr &&)      = default;
    ~observer_ptr()                                = default;

    // pointer-accepting constructors
    constexpr
    observer_ptr(std::nullptr_t) noexcept {
    }

    explicit
    constexpr
    observer_ptr(pointer_type ptr) noexcept
        : m_ptr{ ptr } {
    }

    observer_ptr(std::shared_ptr<element_type> const & ptr) noexcept
        : m_ptr{ ptr.get() } {
    }

    observer_ptr(std::unique_ptr<element_type> const & ptr) noexcept
        : m_ptr{ ptr.get() } {
    }

    // explicit
    // observer_ptr(xobject_ptr_t<element_type> const & ptr) noexcept
    //     : m_ptr{ ptr.get() } {
    // }

    // copying constructor (in addition to compiler-generated copy constructor)
    template
    <
        typename U,
        typename = typename std::enable_if
        <
            std::is_convertible<typename std::add_pointer<U>::type, pointer_type>::value
        >::type
    >
    observer_ptr(observer_ptr<U> ptr) noexcept
        : m_ptr{ ptr.get()}
    {
    }

    // 3.2.3, observer_ptr observers
    pointer_type
    get() const noexcept {
        return m_ptr;
    }

    reference_type
    operator*() const {
        return *get();
    }

    pointer_type
    operator->() const noexcept {
        return get();
    }

    explicit
    operator bool() const noexcept {
        return get() != nullptr;
    }

    // 3.2.4, observer_ptr conversions
    explicit
    operator pointer_type() const noexcept {
        return get();
    }

    // 3.2.5, observer_ptr modifiers
    pointer_type
    release() noexcept {
        pointer_type tmp = get();  // TODO(bluecl): [-Wconstexpr-not-const]
        reset();  // TODO(bluecl): [-Wc++14-extensions]
        return tmp;  // TODO(bluecl): [-Wc++14-extensions]
    }

    void
    reset(pointer_type ptr = nullptr) noexcept {
        m_ptr = ptr;
    }

    void
    swap(observer_ptr & ptr) noexcept {
        std::swap(m_ptr, ptr.m_ptr);
    }
};

template<typename T>
constexpr
observer_ptr<T>
make_observer(std::nullptr_t) noexcept {
    return observer_ptr<T>{};
}

template<typename T>
constexpr
observer_ptr<T>
make_observer(T * p) noexcept {
    return observer_ptr<T>{ p };
}

template<typename T>
observer_ptr<T>
make_observer(std::shared_ptr<T> & p) noexcept {
    return observer_ptr<T>{ p.get() };
}

template<typename T>
observer_ptr<T>
make_observer(std::unique_ptr<T> & p) noexcept {
    return observer_ptr<T>{ p.get() };
}

template<typename T>
observer_ptr<T>
make_observer(xobject_ptr_t<T> & p) noexcept {
    return observer_ptr<T>{ p.get() };
}

template<typename T, typename U>
bool
operator==(observer_ptr<T> p1, observer_ptr<U> p2) {
    return p1.get() == p2.get();
}

template<typename T, typename U>
bool
operator!=(observer_ptr<T> p1, observer_ptr<U> p2) {
    return !(p1 == p2);
}

template<typename T>
bool
operator==(observer_ptr<T> ptr, std::nullptr_t) noexcept {
    return !ptr;
}

template<typename T>
bool
operator==(std::nullptr_t, observer_ptr<T> ptr) noexcept {
    return !ptr;
}

template<typename T>
bool
operator!=(observer_ptr<T> ptr, std::nullptr_t) noexcept {
    return bool(ptr);
}

template<typename T>
bool
operator!=(std::nullptr_t, observer_ptr<T> ptr) noexcept {
    return bool(ptr);
}

template<typename T, typename U>
bool
operator<(observer_ptr<T> p1, observer_ptr<U> p2) {
    return std::less<typename std::common_type<typename std::add_pointer<T>::type, typename std::add_pointer<U>::type>::type>{}(p1.get(), p2.get());
}

template<typename T, typename U>
bool
operator>(observer_ptr<T> p1, observer_ptr<U> p2) {
    return p2 < p1;
}

template<typename T, typename U>
bool
operator<=(observer_ptr<T> p1, observer_ptr<U> p2) {
    return !(p2 < p1);
}

template<typename T, typename U>
bool
operator>=(observer_ptr<T> p1, observer_ptr<U> p2) {
    return !(p1 < p2);
}

template<typename T>
void
swap(observer_ptr<T> & p1, observer_ptr<T> & p2) noexcept {
    p1.swap(p2);
}

#if !defined XCXX14_OR_ABOVE

template<typename T>
struct xtop_make_unique
{
    using single_object = std::unique_ptr<T>;
};

template<typename T>
struct xtop_make_unique<T[]>
{
    using array_object = std::unique_ptr<T[]>;
};

template<typename T, size_t Bound>
struct xtop_make_unique<T[Bound]>
{
    struct invalid_type {};
};

// top::make_unique for single objects
template<typename T, typename ... ArgsT>
typename xtop_make_unique<T>::single_object
make_unique(ArgsT && ... args) {
    return std::unique_ptr<T>(new T{ std::forward<ArgsT>(args)... });
}

// std::make_unique for arrays of unknown bound
template<typename T>
typename xtop_make_unique<T>::array_object
make_unique(std::size_t const n) {
    return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]{});
}

// disable top::make_unique for arrays of known bound
template<typename T, typename... ArgsT>
typename xtop_make_unique<T>::invalid_type
make_unique(ArgsT &&...) = delete;

#else

template <typename T, typename ... ArgsT>
std::unique_ptr<T>
make_unique(ArgsT && ... args) {
    return std::make_unique<T, ArgsT ...>(std::forward<ArgsT>(args)...);
}

template<typename T>
std::unique_ptr<T>
make_unique(std::size_t const n) {
    return std::make_unique<T>(n);
}

#endif

NS_END1
