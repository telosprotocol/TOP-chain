//
//  check_cast.cc
//
//  Created by @Charlie.Xie on 02/12/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/check_cast.h"

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits>

#ifndef LONG_MIN
#define LONG_MIN std::numeric_limits<long>::min()  // NOLINT
#endif

namespace top {

template<>
long check_cast<long, const char*> (const char* s) {  // NOLINT
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
    long res = strtol(s, &end_ptr, 0);  // NOLINT
    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == LONG_MIN) {
            throw CheckCastException(
                std::string("ERROR: exceeded long in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

template<>
long long check_cast<long long, const char*>(const char* s) {  // NOLINT
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
    long long res = strtoll(s, &end_ptr, 0);  // NOLINT
    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == LONG_MIN) {
            throw CheckCastException(
                std::string("ERROR: exceeded int64_t in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

template<>
unsigned long check_cast<unsigned long, const char*>(const char* s) {  // NOLINT
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
    unsigned long res = strtoul(s, &end_ptr, 0);  // NOLINT

    if (memchr(s, '-', end_ptr - s) != NULL) {
        throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
    }

    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == check_cast<unsigned long>(LONG_MIN)) {  // NOLINT
            throw CheckCastException(
                std::string("ERROR: exceeded int64_t in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

template<>
unsigned long long check_cast<unsigned long long, const char*>(const char* s) {  // NOLINT
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
    unsigned long long res = strtoull(s, &end_ptr, 0);  // NOLINT

    if (memchr(s, '-', end_ptr - s) != NULL) {
        throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
    }

    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == check_cast<uint64_t>(LONG_MIN)) {
            throw CheckCastException(
                std::string("ERROR: exceeded int64_t in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

template<>
float check_cast<float, const char*>(const char* s) {
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
#if __GNUC__ <= 2
    float res = float(strtod(s, &end_ptr));  // NOLINT
#else
    float res = strtof(s, &end_ptr);
#endif
    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == LONG_MIN) {
            throw CheckCastException(
                std::string("ERROR: exceeded int64_t in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

template<>
double check_cast<double, const char*>(const char* s) {
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
    double res = strtod(s, &end_ptr);

    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == LONG_MIN) {
            throw CheckCastException(
                std::string("ERROR: exceeded int64_t in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

template<>
long double check_cast<long double, const char*>(const char* s) {  // NOLINT
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    errno = 0;
    char* end_ptr;
#if __GNUC__ <= 2
    long double res = strtod(s, &end_ptr);  // NOLINT
#else
    long double res = strtold(s, &end_ptr);  // NOLINT
#endif

    switch (errno) {
    case 0:
        if (end_ptr == s || *end_ptr != '\0') {
            throw CheckCastException(std::string("ERROR: in string invalid: ") + std::string(s));
        }
        return res;
    case ERANGE:
        if (res == LONG_MIN) {
            throw CheckCastException(
                std::string("ERROR: exceeded int64_t in string invalid: ") + std::string(s));
        } else {
            throw CheckCastException();
        }
    default:
        throw CheckCastException();
    }
}

}  // namespace top
