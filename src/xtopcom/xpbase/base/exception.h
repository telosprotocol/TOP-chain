//
//  exception.h
//
//  Created by @Charlie.Xie on 02/12/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <string>
#include <exception>

namespace top {

class CheckCastException : public std::exception {
public:
    CheckCastException() : err_message_("ERROR: check cast failed!") {}
    explicit CheckCastException(const std::string& err) : err_message_(err) {}
    virtual char const* what() const noexcept { return err_message_.c_str(); }

private:
    std::string err_message_;
};

}  // namespace top
