// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"

#include <cstddef>

NS_BEG3(top, network, tests)

template <typename T>
class xtop_object_manager
{
public:
    xtop_object_manager()                                        = default;
    xtop_object_manager(xtop_object_manager const &)             = delete;
    xtop_object_manager & operator=(xtop_object_manager const &) = delete;
    xtop_object_manager(xtop_object_manager &&)                  = default;
    xtop_object_manager & operator=(xtop_object_manager &&)      = default;
    virtual ~xtop_object_manager()                               = default;

    virtual
    void
    start() = 0;

    virtual
    void
    stop() = 0;

    virtual
    std::size_t
    object_count() const noexcept = 0;

    virtual
    T &
    object(std::size_t const index) = 0;

    template <typename U, typename = typename std::enable_if<std::is_base_of<T, U>::value>::type>
    U &
    object(std::size_t const index) {
        return static_cast<U &>(object(index));
    }

    virtual
    T const &
    object(std::size_t const index) const = 0;

    template <typename U, typename = typename std::enable_if<std::is_base_of<T, U>::value>::type>
    U const &
    object(std::size_t const index) const {
        return static_cast<U &>(object(index));
    }
};
template <typename T>
using xobject_manager_t = xtop_object_manager<T>;

NS_END3
