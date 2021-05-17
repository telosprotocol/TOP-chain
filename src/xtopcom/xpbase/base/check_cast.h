//
//  check_cast.h
//
//  Created by @Charlie.Xie on 02/12/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <stdint.h>
#include <limits.h>
#include <limits>
#include <string>
#include <sstream>

#include "xpbase/base/exception.h"

namespace top {

template<typename DestType, typename SrcType>
DestType check_cast(SrcType value);

template<typename DestType>
inline DestType check_cast(DestType value) {
    return value;
}

template<>
inline short check_cast<short, short>(short value) {
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, short>(short value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, short>(short value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, short>(short value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, short>(short value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    if (value > 255) {
        throw CheckCastException(std::string("value bound error[255]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, short>(short value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, short>(short value) {
    if (value < -128) {
        throw CheckCastException(std::string("value bound error[-128]"));
    }
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, short>(short value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, short>(short value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, short>(short value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, short>(short value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, short>(short value) {
    if (value < -128) {
        throw CheckCastException(std::string("value bound error[-128]"));
    }
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, short>(short value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, long double>(long double value) {
    if (value < -32768.0L) {
        throw CheckCastException(std::string("value bound error[-32768.0L]"));
    }
    if (value > 32767.0L) {
        throw CheckCastException(std::string("value bound error[32767.0L]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, long double>(long double value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, long double>(long double value) {
    if (value < -2147483648.0L) {
        throw CheckCastException(std::string("value bound error[-2147483648.0L]"));
    }
    if (value > 2147483647.0L) {
        throw CheckCastException(std::string("value bound error[2147483647.0L]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, long double>(long double value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, long double>(long double value) {
    if (value < 0.0L) {
        throw CheckCastException(std::string("value bound error[0.0L]"));
    }
    if (value > 255.0L) {
        throw CheckCastException(std::string("value bound error[255.0L]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, long double>(long double value) {
    if (value < 0.0L) {
        throw CheckCastException(std::string("value bound error[0.0L]"));
    }
    if (value > 18446744073709551615.0L) {
        throw CheckCastException(std::string("value bound error[18446744073709551615.0L]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, long double>(long double value) {
    if (value < -128.0L) {
        throw CheckCastException(std::string("value bound error[-128.0L]"));
    }
    if (value > 127.0L) {
        throw CheckCastException(std::string("value bound error[127.0L]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, long double>(long double value) {
    if (value < 0.0L) {
        throw CheckCastException(std::string("value bound error[0.0L]"));
    }
    if (value > 65535.0L) {
        throw CheckCastException(std::string("value bound error[65535.0L]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, long double>(long double value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, long double>(long double value) {
    if (value < 0.0L) {
        throw CheckCastException(std::string("value bound error[0.0L]"));
    }
    if (value > 18446744073709551615.0L) {
        throw CheckCastException(std::string("value bound error[18446744073709551615.0L]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, long double>(long double value) {
    if (value < -9223372036854775808.0L) {
        throw CheckCastException(std::string("value bound error[-9223372036854775808.0L]"));
    }
    if (value > 9223372036854775807.0L) {
        throw CheckCastException(std::string("value bound error[9223372036854775807.0L]"));
    }
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, long double>(long double value) {
    if (value < -128.0L) {
        throw CheckCastException(std::string("value bound error[-128.0L]"));
    }
    if (value > 127.0L) {
        throw CheckCastException(std::string("value bound error[127.0L]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, long double>(long double value) {
    if (value < 0.0L) {
        throw CheckCastException(std::string("value bound error[0.0L]"));
    }
    if (value > 4294967295.0L) {
        throw CheckCastException(std::string("value bound error[4294967295.0L]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, int>(int value) {
    if (value < -32768) {
        throw CheckCastException(std::string("value bound error[-32768]"));
    }
    if (value > 32767) {
        throw CheckCastException(std::string("value bound error[32767]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, int>(int value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, int>(int value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, int>(int value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, int>(int value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    if (value > 255) {
        throw CheckCastException(std::string("value bound error[255]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, int>(int value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, int>(int value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, int>(int value) {
    if (value < -128) {
        throw CheckCastException(std::string("value bound error[-128]"));
    }
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, int>(int value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    if (value > 65535) {
        throw CheckCastException(std::string("value bound error[65535]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, int>(int value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, int>(int value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, int>(int value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, int>(int value) {
    if (value < -128) {
        throw CheckCastException(std::string("value bound error[-128]"));
    }
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, int>(int value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, double>(double value) {
    if (value < -32768.0) {
        throw CheckCastException(std::string("value bound error[-32768.0]"));
    }
    if (value > 32767.0) {
        throw CheckCastException(std::string("value bound error[32767.0]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, double>(double value) {
    return static_cast<long double>(value);
}


template<>
inline int check_cast<int, double>(double value) {
    if (value < -2147483648.0) {
        throw CheckCastException(std::string("value bound error[-2147483648.0]"));
    }
    if (value > 2147483647.0) {
        throw CheckCastException(std::string("value bound error[2147483647.0]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, double>(double value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, double>(double value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound error[0.0]"));
    }
    if (value > 255.0) {
        throw CheckCastException(std::string("value bound error[255.0]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, double>(double value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound error[0.0]"));
    }
    if (value > 18446744073709551615.0) {
        throw CheckCastException(std::string("value bound error[18446744073709551615.0]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, double>(double value) {
    if (value < -128.0) {
        throw CheckCastException(std::string("value bound error[-128.0]"));
    }
    if (value > 127.0) {
        throw CheckCastException(std::string("value bound error[127.0]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, double>(double value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound error[0.0]"));
    }
    if (value > 65535.0) {
        throw CheckCastException(std::string("value bound error[65535.0]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, double>(double value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, double>(double value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound error[0.0]"));
    }
    if (value > 18446744073709551615.0) {
        throw CheckCastException(std::string("value bound error[18446744073709551615.0]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, double>(double value) {
    if (value < -9223372036854775808.0) {
        throw CheckCastException(std::string("value bound error[-9223372036854775808.0]"));
    }
    if (value > 9223372036854775807.0) {
        throw CheckCastException(std::string("value bound error[9223372036854775807.0]"));
    }
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, double>(double value) {
    if (value < -128.0) {
        throw CheckCastException(std::string("value bound error[-128.0]"));
    }
    if (value > 127.0) {
        throw CheckCastException(std::string("value bound error[127.0]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, double>(double value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound error[0.0]"));
    }
    if (value > 4294967295.0) {
        throw CheckCastException(std::string("value bound error[4294967295.0]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, unsigned char>(unsigned char value) {
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, unsigned char>(unsigned char value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, unsigned char>(unsigned char value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, unsigned char>(unsigned char value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, unsigned char>(unsigned char value) {
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, unsigned char>(unsigned char value) {
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, unsigned char>(unsigned char value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, unsigned char>(unsigned char value) {
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, unsigned char>(unsigned char value) {
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, unsigned char>(unsigned char value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, unsigned char>(unsigned char value) {
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, unsigned char>(unsigned char value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, unsigned char>(unsigned char value) {
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, unsigned char>(unsigned char value) {
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, unsigned long long>(unsigned long long value) {
    if (value > 32767ULL) {
        throw CheckCastException(std::string("value bound error[32767ULL]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, unsigned long long>(unsigned long long value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, unsigned long long>(unsigned long long value) {
    if (value > 2147483647ULL) {
        throw CheckCastException(std::string("value bound error[2147483647ULL]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, unsigned long long>(unsigned long long value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, unsigned long long>(unsigned long long value) {
    if (value > 255ULL) {
        throw CheckCastException(std::string("value bound error[255ULL]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, unsigned long long>(unsigned long long value) {
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, unsigned long long>(unsigned long long value) {
    if (value > 127ULL) {
        throw CheckCastException(std::string("value bound error[127ULL]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, unsigned long long>(unsigned long long value) {
    if (value > 65535ULL) {
        throw CheckCastException(std::string("value bound error[65535ULL]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, unsigned long long>(unsigned long long value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, unsigned long long>(unsigned long long value) {
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, unsigned long long>(unsigned long long value) {
    if (value > 9223372036854775807ULL) {
        throw CheckCastException(std::string("value bound error[9223372036854775807ULL]"));
    }
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, unsigned long long>(unsigned long long value) {
    if (value > 127ULL) {
        throw CheckCastException(std::string("value bound error[127ULL]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, unsigned long long>(unsigned long long value) {
    if (value > 4294967295ULL) {
        throw CheckCastException(std::string("value bound error[4294967295ULL]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, long>(long value) {
    if (value < -32768L) {
        throw CheckCastException(std::string("value bound error[-32768L]"));
    }
    if (value > 32767L) {
        throw CheckCastException(std::string("value bound error[32767L]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, long>(long value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, long>(long value) {
    if (value < -2147483648LL) {
        throw CheckCastException(std::string("value bound error[-2147483648L]"));
    }
    if (value > 2147483647L) {
        throw CheckCastException(std::string("value bound error[2147483647L]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, long>(long value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, long>(long value) {
    if (value < 0L) {
        throw CheckCastException(std::string("value bound error[0L]"));
    }
    if (value > 255L) {
        throw CheckCastException(std::string("value bound error[255L]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, long>(long value) {
    if (value < 0L) {
        throw CheckCastException(std::string("value bound error[0L]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, long>(long value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, long>(long value) {
    if (value < -128L) {
        throw CheckCastException(std::string("value bound error[-128L]"));
    }
    if (value > 127L) {
        throw CheckCastException(std::string("value bound error[127L]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, long>(long value) {
    if (value < 0L) {
        throw CheckCastException(std::string("value bound error[0L]"));
    }
    if (value > 65535L) {
        throw CheckCastException(std::string("value bound error[65535L]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, long>(long value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, long>(long value) {
    if (value < 0L) {
        throw CheckCastException(std::string("value bound error[0L]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, long>(long value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, long>(long value) {
    if (value < -128L) {
        throw CheckCastException(std::string("value bound error[-128L]"));
    }
    if (value > 127L) {
        throw CheckCastException(std::string("value bound error[127L]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, long>(long value) {
    if (value < 0L) {
        throw CheckCastException(std::string("value bound error[0L]"));
    }
    if (value > 4294967295L) {
        throw CheckCastException(std::string("value bound error[4294967295L]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, char>(char value) {
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, char>(char value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, char>(char value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, char>(char value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, char>(char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, char>(char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, char>(char value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, char>(char value) {
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, char>(char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, char>(char value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, char>(char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, char>(char value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, char>(char value) {
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, char>(char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound error[0]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, unsigned short>(unsigned short value) {
    if (value > 32767) {
        throw CheckCastException(std::string("value bound error[32767]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, unsigned short>(unsigned short value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, unsigned short>(unsigned short value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, unsigned short>(unsigned short value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, unsigned short>(unsigned short value) {
    if (value > 255) {
        throw CheckCastException(std::string("value bound error[255]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, unsigned short>(unsigned short value) {
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, unsigned short>(unsigned short value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, unsigned short>(unsigned short value) {
    if (value > 127) {
        throw CheckCastException(std::string("value bound error[127]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, unsigned short>(unsigned short value) {
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, unsigned short>(unsigned short value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, unsigned short>(unsigned short value) {
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, unsigned short>(unsigned short value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, unsigned short>(unsigned short value) {
    if (value > 127) {
        throw CheckCastException(std::string("value bound[127]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, unsigned short>(unsigned short value) {
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, float>(float value) {
    if (value < -32768.0) {
        throw CheckCastException(std::string("value bound[-32768.0]"));
    }
    if (value > 32767.0) {
        throw CheckCastException(std::string("value bound[32767.0]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, float>(float value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, float>(float value) {
    if (value < -2147483648.0) {
        throw CheckCastException(std::string("value bound[-2147483648.0]"));
    }
    if (value > 2147483647.0) {
        throw CheckCastException(std::string("value bound[2147483647.0]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, float>(float value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, float>(float value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound[0.0]"));
    }
    if (value > 255.0) {
        throw CheckCastException(std::string("value bound[255.0]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, float>(float value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound[0.0]"));
    }
    if (value > 18446744073709551615.0) {
        throw CheckCastException(std::string("value bound[18446744073709551615.0]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, float>(float value) {
    if (value < -128.0) {
        throw CheckCastException(std::string("value bound[-128.0]"));
    }
    if (value > 127.0) {
        throw CheckCastException(std::string("value bound[127.0]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, float>(float value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound[0.0]"));
    }
    if (value > 65535.0) {
        throw CheckCastException(std::string("value bound[65535.0]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, float>(float value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, float>(float value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound[0.0]"));
    }
    if (value > 18446744073709551615.0) {
        throw CheckCastException(std::string("value bound[18446744073709551615.0]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, float>(float value) {
    if (value < -9223372036854775808.0) {
        throw CheckCastException(std::string("value bound[-9223372036854775808.0]"));
    }
    if (value > 9223372036854775807.0) {
        throw CheckCastException(std::string("value bound[9223372036854775807.0]"));
    }
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, float>(float value) {
    if (value < -128.0) {
        throw CheckCastException(std::string("value bound[-128.0]"));
    }
    if (value > 127.0) {
        throw CheckCastException(std::string("value bound[127.0]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, float>(float value) {
    if (value < 0.0) {
        throw CheckCastException(std::string("value bound[0.0]"));
    }
    if (value > 4294967295.0) {
        throw CheckCastException(std::string("value bound[4294967295.0]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, unsigned long>(unsigned long value) {
    if (value > 32767UL) {
        throw CheckCastException(std::string("value bound[32767UL]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, unsigned long>(unsigned long value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, unsigned long>(unsigned long value) {
    if (value > 2147483647UL) {
        throw CheckCastException(std::string("value bound[2147483647UL]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, unsigned long>(unsigned long value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, unsigned long>(unsigned long value) {
    if (value > 255UL) {
        throw CheckCastException(std::string("value bound[255UL]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, unsigned long>(unsigned long value) {
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, unsigned long>(unsigned long value) {
    if (value > 127UL) {
        throw CheckCastException(std::string("value bound[127UL]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, unsigned long>(unsigned long value) {
    if (value > 65535UL) {
        throw CheckCastException(std::string("value bound[65535UL]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, unsigned long>(unsigned long value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, unsigned long>(unsigned long value) {
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, unsigned long>(unsigned long value) {
    if (value > 9223372036854775807UL) {
        throw CheckCastException(std::string("value bound[9223372036854775807UL]"));
    }
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, unsigned long>(unsigned long value) {
    if (value > 127UL) {
        throw CheckCastException(std::string("value bound[127UL]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, unsigned long>(unsigned long value) {
    if (value > 4294967295UL) {
        throw CheckCastException(std::string("value bound[4294967295UL]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, long long>(long long value) {
    if (value < -32768LL) {
        throw CheckCastException(std::string("value bound[-32768LL]"));
    }
    if (value > 32767LL) {
        throw CheckCastException(std::string("value bound[32767LL]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, long long>(long long value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, long long>(long long value) {
    if (value < -2147483648LL) {
        throw CheckCastException(std::string("value bound[-2147483648LL]"));
    }
    if (value > 2147483647LL) {
        throw CheckCastException(std::string("value bound[2147483647LL]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, long long>(long long value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, long long>(long long value) {
    if (value < 0LL) {
        throw CheckCastException(std::string("value bound[0LL]"));
    }
    if (value > 255LL) {
        throw CheckCastException(std::string("value bound[255LL]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, long long>(long long value) {
    if (value < 0LL) {
        throw CheckCastException(std::string("value bound[0LL]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, long long>(long long value) {
    if (value < -128LL) {
        throw CheckCastException(std::string("value bound[-128LL]"));
    }
    if (value > 127LL) {
        throw CheckCastException(std::string("value bound[127LL]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, long long>(long long value) {
    if (value < 0LL) {
        throw CheckCastException(std::string("value bound[0LL]"));
    }
    if (value > 65535LL) {
        throw CheckCastException(std::string("value bound[65535LL]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, long long>(long long value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, long long>(long long value) {
    if (value < 0LL) {
        throw CheckCastException(std::string("value bound[0LL]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, long long>(long long value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, long long>(long long value) {
    if (value < -128LL) {
        throw CheckCastException(std::string("value bound[-128LL]"));
    }
    if (value > 127LL) {
        throw CheckCastException(std::string("value bound[127LL]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, long long>(long long value) {
    if (value < 0LL) {
        throw CheckCastException(std::string("value bound[0LL]"));
    }
    if (value > 4294967295LL) {
        throw CheckCastException(std::string("value bound[4294967295LL]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, signed char>(signed char value) {
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, signed char>(signed char value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, signed char>(signed char value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, signed char>(signed char value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, signed char>(signed char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound[0]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, signed char>(signed char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound[0]"));
    }
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, signed char>(signed char value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, signed char>(signed char value) {
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, signed char>(signed char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound[0]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, signed char>(signed char value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, signed char>(signed char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound[0]"));
    }
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, signed char>(signed char value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, signed char>(signed char value) {
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, signed char>(signed char value) {
    if (value < 0) {
        throw CheckCastException(std::string("value bound[0]"));
    }
    return static_cast<unsigned int>(value);
}

template<>
inline short check_cast<short, unsigned int>(unsigned int value) {
    if (value > 32767U) {
        throw CheckCastException(std::string("value bound[32767U]"));
    }
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, unsigned int>(unsigned int value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, unsigned int>(unsigned int value) {
    if (value > 2147483647U) {
        throw CheckCastException(std::string("value bound[2147483647U]"));
    }
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, unsigned int>(unsigned int value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, unsigned int>(unsigned int value) {
    if (value > 255U) {
        throw CheckCastException(std::string("value bound[255U]"));
    }
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, unsigned int>(unsigned int value) {
    return static_cast<unsigned long long>(value);
}

template<>
inline char check_cast<char, unsigned int>(unsigned int value) {
    if (value > 127U) {
        throw CheckCastException(std::string("value bound[127U]"));
    }
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, unsigned int>(unsigned int value) {
    if (value > 65535U) {
        throw CheckCastException(std::string("value bound[65535U]"));
    }
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, unsigned int>(unsigned int value) {
    return static_cast<float>(value);
}

template<>
inline unsigned long check_cast<unsigned long, unsigned int>(unsigned int value) {
    return static_cast<unsigned long>(value);
}

template<>
inline long long check_cast<long long, unsigned int>(unsigned int value) {
    return static_cast<long long>(value);
}

template<>
inline signed char check_cast<signed char, unsigned int>(unsigned int value) {
    if (value > 127U) {
        throw CheckCastException(std::string("value bound[127U]"));
    }
    return static_cast<signed char>(value);
}

template<>
inline unsigned int check_cast<unsigned int, unsigned int>(unsigned int value) {
    return static_cast<unsigned int>(value);
}

#if __WORDSIZE == 32

template<>
inline long check_cast<long, short>(short value) {
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, long double>(long double value) {
    if (value < -2147483648.0L) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    if (value > 2147483647.0L) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, double>(double value) {
    if (value < -2147483648.0) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    if (value > 2147483647.0) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, unsigned long long>(unsigned long long value) {
    if (value > 2147483647ULL) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, float>(float value) {
    if (value < -2147483648.0) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    if (value > 2147483647.0) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, unsigned long>(unsigned long value) {
    if (value > 2147483647UL) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, long long>(long long value) {
    if (value < -2147483648LL) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    if (value > 2147483647LL) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, unsigned int>(unsigned int value) {
    if (value > 2147483647U) {
        throw CheckCastException(std::string("value bound error[]"));
    }
    return static_cast<long>(value);
}

#else

template<>
inline long check_cast<long, short>(short value) {
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, long double>(long double value) {
    if (value < -9223372036854775808.0L) {
        throw CheckCastException(std::string("value bound error[-9223372036854775808.0L]"));
    }
    if (value > 9223372036854775807.0L) {
        throw CheckCastException(std::string("value bound error[9223372036854775807.0L]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, double>(double value) {
    if (value < -9223372036854775808.0) {
        throw CheckCastException(std::string("value bound error[-9223372036854775808.0]"));
    }
    if (value > 9223372036854775807.0) {
        throw CheckCastException(std::string("value bound error[9223372036854775807.0]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, unsigned long long>(unsigned long long value) {
    if (value > 9223372036854775807ULL) {
        throw CheckCastException(std::string("value bound error[9223372036854775807ULL]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, float>(float value) {
    if (value < -9223372036854775808.0) {
        throw CheckCastException(std::string("value bound[-9223372036854775808.0]"));
    }
    if (value > 9223372036854775807.0) {
        throw CheckCastException(std::string("value bound[9223372036854775807.0]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, unsigned long>(unsigned long value) {
    if (value > 9223372036854775807UL) {
        throw CheckCastException(std::string("value bound[9223372036854775807UL]"));
    }
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, long long>(long long value) {
    return static_cast<long>(value);
}

template<>
inline long check_cast<long, unsigned int>(unsigned int value) {
    return static_cast<long>(value);
}

#endif

template<>
inline bool check_cast<bool, short>(short value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, long double>(long double value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, int>(int value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, double>(double value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, unsigned char>(unsigned char value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, unsigned long long>(unsigned long long value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, long>(long value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, char>(char value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, unsigned short>(unsigned short value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, float>(float value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, unsigned long>(unsigned long value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, long long>(long long value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, signed char>(signed char value) {
    return static_cast<bool>(value);
}

template<>
inline bool check_cast<bool, unsigned int>(unsigned int value) {
    return static_cast<bool>(value);
}

template<>
inline short check_cast<short, bool>(bool value) {
    return static_cast<short>(value);
}

template<>
inline long double check_cast<long double, bool>(bool value) {
    return static_cast<long double>(value);
}

template<>
inline int check_cast<int, bool>(bool value) {
    return static_cast<int>(value);
}

template<>
inline double check_cast<double, bool>(bool value) {
    return static_cast<double>(value);
}

template<>
inline unsigned char check_cast<unsigned char, bool>(bool value) {
    return static_cast<unsigned char>(value);
}

template<>
inline unsigned long long check_cast<unsigned long long, bool>(bool value) {
    return static_cast<unsigned long long>(value);
}

template<>
inline long check_cast<long, bool>(bool value) {
    return static_cast<long>(value);
}

template<>
inline char check_cast<char, bool>(bool value) {
    return static_cast<char>(value);
}

template<>
inline unsigned short check_cast<unsigned short, bool>(bool value) {
    return static_cast<unsigned short>(value);
}

template<>
inline float check_cast<float, bool>(bool value) {
    return static_cast<float>(value);
}


template<>
inline unsigned long check_cast<unsigned long, bool>(bool value) {
    return static_cast<unsigned long>(value);
}


template<>
inline long long check_cast<long long, bool>(bool value) {
    return static_cast<long long>(value);
}


template<>
inline signed char check_cast<signed char, bool>(bool value) {
    return static_cast<signed char>(value);
}


template<>
inline unsigned int check_cast<unsigned int, bool>(bool value) {
    return static_cast<unsigned int>(value);
}


template<>
inline bool check_cast<bool, bool>(bool value) {
    return static_cast<bool>(value);
}

template<>
long check_cast<long, const char*>(const char* s);

template<>
inline char check_cast<char, const char*>(const char* s) {
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    return s[0];
}

template<>
inline signed char check_cast<signed char, const char*>(const char* s) {
    return check_cast<signed char>(check_cast<long>(s));
}

template<>
inline unsigned char check_cast<unsigned char, const char*>(const char* s) {
    return check_cast<unsigned char>(check_cast<long>(s));
}

template<>
inline short check_cast<short, const char*>(const char* s) {
    return check_cast<short>(check_cast<long>(s));
}

template<>
inline int check_cast<int, const char*>(const char* s) {
    return check_cast<int>(check_cast<long>(s));
}

template<>
long long check_cast<long long, const char*>(const char* s);

template<>
unsigned long check_cast<unsigned long, const char*>(const char* s);

template<>
inline unsigned short check_cast<unsigned short, const char*>(const char* s) {
    return check_cast<unsigned short>(check_cast<unsigned long>(s));
}

template<>
inline unsigned int check_cast<unsigned int, const char*>(const char* s) {
    return check_cast<unsigned int>(check_cast<unsigned long>(s));
}

template<>
unsigned long long check_cast<unsigned long long, const char*>(const char* s);

template<>
float check_cast<float, const char*>(const char* s);

template<>
double check_cast<double, const char*>(const char* s);

template<>
long double check_cast<long double, const char*>(const char* s);

template<>
inline char check_cast<char, char*>(char* s) {
    if (!s) {
        throw CheckCastException("ERROR: in param is NULL.");
    }
    return s[0];
}

template<>
inline long check_cast<long, char*>(char* s) {
    return check_cast<long, const char*>(s);
}

template<>
inline bool check_cast<bool, char*>(char* s) {
    return check_cast<bool>(check_cast<long>(s));
}

template<>
inline bool check_cast<bool, const char*>(const char* s) {
    return check_cast<bool>(check_cast<long>(s));
}

template<>
inline short check_cast<short, char*>(char* s) {
    return check_cast<short>(check_cast<long>(s));
}

template<>
inline signed char check_cast<signed char, char*>(char* s) {
    return check_cast<signed char>(check_cast<long>(s));
}

template<>
inline unsigned char check_cast<unsigned char, char*>(char* s) {
    return check_cast<unsigned char>(check_cast<long>(s));
}

template<>
inline int check_cast<int, char*>(char* s) {
    return check_cast<int>(check_cast<long>(s));
}

template<>
inline long long check_cast<long long, char*>(char* s) {
    return check_cast<long long, const char*>(s);
}

template<>
inline unsigned long check_cast<unsigned long, char*>(char* s) {
    return check_cast<unsigned long, const char*>(s);
}

template<>
inline unsigned short check_cast<unsigned short, char*>(char* s) {
    return check_cast<unsigned short>(check_cast<unsigned long>(s));
}

template<>
inline unsigned int check_cast<unsigned int, char*>(char* s) {
    return check_cast<unsigned int>(check_cast<unsigned long>(s));
}

template<>
inline unsigned long long check_cast<unsigned long long, char*>(char* s) {
    return check_cast<unsigned long long, const char*>(s);
}

template<>
inline float check_cast<float, char*>(char* s) {
    return check_cast<float, const char*>(s);
}

template<>
inline double check_cast<double, char*>(char* s) {
    return check_cast<double, const char*>(s);
}

template<>
inline long double check_cast<long double, char*>(char* s) {
    return check_cast<long double, const char*>(s);
}

template<typename T>
std::string TypeToString(T value) {
    std::ostringstream os;
    os << value;
    std::string result;
    std::istringstream is(os.str());
    is >> result;
    return result;
}

template<>
inline std::string check_cast<std::string, bool>(bool value) {
    return TypeToString<bool>(value);
}

template<>
inline std::string check_cast<std::string, char>(char value) {
    return TypeToString<char>(value);
}

template<>
inline std::string check_cast<std::string, signed char>(signed char value) {
    return TypeToString<signed char>(value);
}

template<>
inline std::string check_cast<std::string, unsigned char>(unsigned char value) {
    return TypeToString<unsigned char>(value);
}

template<>
inline std::string check_cast<std::string, short>(short value) {
    return TypeToString<short>(value);
}

template<>
inline std::string check_cast<std::string, unsigned short>(unsigned short value) {
    return TypeToString<unsigned short>(value);
}

template<>
inline std::string check_cast<std::string, int>(int value) {
    return TypeToString<int>(value);
}

template<>
inline std::string check_cast<std::string, unsigned int>(unsigned int value) {
    return TypeToString<unsigned int>(value);
}

template<>
inline std::string check_cast<std::string, long>(long value) {
    return TypeToString<long>(value);
}

template<>
inline std::string check_cast<std::string, unsigned long>(unsigned long value) {
    return TypeToString<unsigned long>(value);
}

template<>
inline std::string check_cast<std::string, long long>(long long value) {
    return TypeToString<long long>(value);
}

template<>
inline std::string check_cast<std::string, unsigned long long>(unsigned long long value) {
    return TypeToString<unsigned long long>(value);
}

template<>
inline std::string check_cast<std::string, float>(float value) {
    return TypeToString<float>(value);
}

template<>
inline std::string check_cast<std::string, double>(double value) {
    return TypeToString<double>(value);
}

template<>
inline std::string check_cast<std::string, long double>(long double value) {
    return TypeToString<long double>(value);
}

template<>
inline std::string check_cast<std::string, char*>(char* value) {
    return value;
}

template<>
inline std::string check_cast<std::string, const char*>(const char* value) {
    return value;
}

}  // namespace top