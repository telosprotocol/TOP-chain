// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xerror/xerror.h"

#include "xbase/xlog.h"

#include <cassert>
#include <string>
#include <system_error>
#include <typeinfo>

NS_BEG1(top)

template <typename T>
class xtop_enable_to_string {
public:
    xtop_enable_to_string()                                          = default;
    xtop_enable_to_string(xtop_enable_to_string const &)             = default;
    xtop_enable_to_string & operator=(xtop_enable_to_string const &) = default;
    xtop_enable_to_string(xtop_enable_to_string &&)                  = default;
    xtop_enable_to_string & operator=(xtop_enable_to_string &&)      = default;
    virtual ~xtop_enable_to_string()                                 = default;

    virtual std::string to_string() const = 0;

    std::string to_string(std::error_code & ec) const {
        assert(!ec);
        try {
            return to_string();
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            xwarn("%s", (std::string{typeid(T).name()} + "::to_string failed").c_str());
        } catch (...) {
            ec = top::error::xbasic_errc_t::serialization_error;
            xerror("%s", "unknown error");
        }

        return {};
    }

    virtual int32_t from_string(std::string const & s) {
        assert(false);
        return 0;
    };

    void from_string(std::string const & s, std::error_code & ec) {
        assert(!ec);
        try {
            auto ret = from_string(s);
            if (ret <= 0) {
                ec = top::error::xbasic_errc_t::deserialization_error;
            }
        } catch (top::error::xtop_error_t const & eh) {
            ec = eh.code();
            xwarn("%s", (std::string{typeid(T).name()} + "::from_string failed").c_str());
        } catch (...) {
            ec = top::error::xbasic_errc_t::deserialization_error;
            xerror("%s", "unknown error");
        }
    }
};
template <typename T>
using xenable_to_string_t = xtop_enable_to_string<T>;

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_string_t<T>, T>::value>::type * = nullptr>
std::string to_string(xenable_to_string_t<T> const & v) {
    return v.to_string();
}

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_string_t<T>, T>::value>::type * = nullptr>
std::string to_string(xenable_to_string_t<T> const & v, std::error_code & ec) {
    return v.xenable_to_string_t<T>::to_string(ec);
}

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_string_t<T>, T>::value>::type * = nullptr>
T from_string(std::string const & s) {
    T ret;
    ret.from_string(s);
    return ret;
}

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_string_t<T>, T>::value>::type * = nullptr>
T from_string(std::string const & s, std::error_code & ec) {
    T ret;
    ret.xenable_to_string_t<T>::from_string(s, ec);
    return ret;
}

NS_END1
