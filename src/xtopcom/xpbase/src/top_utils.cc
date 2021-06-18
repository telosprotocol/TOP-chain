// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xpbase/base/top_utils.h"

#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <array>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <sstream>
#include <locale>

#ifdef _WIN32
#include <winsock2.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

#ifdef _MSC_VER
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "common/xxhash.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/check_cast.h"

namespace top {

std::shared_ptr<base::KadmliaKey> global_xid;
std::string global_node_id("");
std::string global_node_signkey("");

using byte = unsigned char;
const char kHexAlphabet[] = "0123456789abcdef";
const char kHexLookup[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
const char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char kPadCharacter('=');

uint32_t& rng_seed() {
#ifdef _MSC_VER
    // Work around high_resolution_clock being the lowest resolution clock pre-VS14
    static uint32_t seed([] {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return static_cast<uint32_t>(t.LowPart);
    }());
#else
    static uint32_t seed(
        static_cast<uint32_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()));
#endif
    return seed;
}

std::string RandomString(size_t size) { return GetRandomString<std::string>(size); }

std::mt19937& random_number_generator() {
    static std::mt19937 random_number_generator(rng_seed());
    return random_number_generator;
}

std::mutex& random_number_generator_mutex() {
    static std::mutex random_number_generator_mutex;
    return random_number_generator_mutex;
}

template <typename IntType>
IntType RandomInt() {
  static std::uniform_int_distribution<IntType> distribution(std::numeric_limits<IntType>::min(),
          std::numeric_limits<IntType>::max());
  std::lock_guard<std::mutex> lock(random_number_generator_mutex());
  return distribution(random_number_generator());
}

int32_t RandomInt32() { return RandomInt<int32_t>();}

uint32_t RandomUint32() { return RandomInt<uint32_t>(); }

uint64_t RandomUint64() { return RandomInt<uint64_t>(); }

uint16_t RandomUint16() { return RandomInt<uint16_t>(); }

std::string HexEncode(const std::string& str) {
    auto size(str.size());
    std::string hex_output(size * 2, 0);
    for (size_t i(0), j(0); i != size; ++i) {
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) / 16];
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) % 16];
    }
    return hex_output;
}

std::string HexDecode(const std::string& str) {
    auto size(str.size());
    if (size % 2) return "";

    std::string non_hex_output(size / 2, 0);
    for (size_t i(0), j(0); i != size / 2; ++i) {
        non_hex_output[i] = (kHexLookup[static_cast<int>(str[j++])] << 4);
        non_hex_output[i] |= kHexLookup[static_cast<int>(str[j++])];
    }
    return non_hex_output;
}

std::string Base64Encode(const std::string& str) {
    std::basic_string<byte> encoded_string(
        ((str.size() / 3) + (str.size() % 3 > 0)) * 4, 0);
    int32_t temp;
    auto cursor = std::begin(reinterpret_cast<const std::basic_string<byte>&>(str));
    size_t i = 0;
    size_t common_output_size((str.size() / 3) * 4);
    while (i < common_output_size) {
        temp = (*cursor++) << 16;
        temp += (*cursor++) << 8;
        temp += (*cursor++);
        encoded_string[i++] = kBase64Alphabet[(temp & 0x00FC0000) >> 18];
        encoded_string[i++] = kBase64Alphabet[(temp & 0x0003F000) >> 12];
        encoded_string[i++] = kBase64Alphabet[(temp & 0x00000FC0) >> 6];
        encoded_string[i++] = kBase64Alphabet[(temp & 0x0000003F)];
    }
    switch (str.size() % 3) {
    case 1:
        temp = (*cursor++) << 16;
        encoded_string[i++] = kBase64Alphabet[(temp & 0x00FC0000) >> 18];
        encoded_string[i++] = kBase64Alphabet[(temp & 0x0003F000) >> 12];
        encoded_string[i++] = kPadCharacter;
        encoded_string[i++] = kPadCharacter;
        break;
    case 2:
        temp = (*cursor++) << 16;
        temp += (*cursor++) << 8;
        encoded_string[i++] = kBase64Alphabet[(temp & 0x00FC0000) >> 18];
        encoded_string[i++] = kBase64Alphabet[(temp & 0x0003F000) >> 12];
        encoded_string[i++] = kBase64Alphabet[(temp & 0x00000FC0) >> 6];
        encoded_string[i++] = kPadCharacter;
        break;
    }
    return std::string(std::begin(encoded_string), std::end(encoded_string));
}

std::string Base64Decode(const std::string& str) {
    if (str.size() % 4)
        return "";

    size_t padding = 0;
    if (str.size()) {
        if (str[str.size() - 1] == static_cast<size_t>(kPadCharacter))
            ++padding;
        if (str[str.size() - 2] == static_cast<size_t>(kPadCharacter))
            ++padding;
    }

    std::string decoded_bytes;
    decoded_bytes.reserve(((str.size() / 4) * 3) - padding);
    uint32_t temp = 0;
    auto cursor = std::begin(str);
    while (cursor < std::end(str)) {
        for (size_t quantum_position = 0; quantum_position < 4; ++quantum_position) {
            temp <<= 6;
            if (*cursor >= 0x41 && *cursor <= 0x5A) {
                temp |= *cursor - 0x41;
            } else if (*cursor >= 0x61 && *cursor <= 0x7A) {
                temp |= *cursor - 0x47;
            } else if (*cursor >= 0x30 && *cursor <= 0x39) {
                temp |= *cursor + 0x04;
            } else if (*cursor == 0x2B) {
                temp |= 0x3E;
            } else if (*cursor == 0x2F) {
                temp |= 0x3F;
            }  else if (*cursor == kPadCharacter) {
                switch (std::end(str) - cursor) {
                case 1:
                    decoded_bytes.push_back((temp >> 16) & 0x000000FF);
                    decoded_bytes.push_back((temp >> 8) & 0x000000FF);
                    return decoded_bytes;
                case 2:
                    decoded_bytes.push_back((temp >> 10) & 0x000000FF);
                    return decoded_bytes;
                default:
                    return "";
                }
            } else {
                return "";
            }
            ++cursor;
        }
        decoded_bytes.push_back((temp >> 16) & 0x000000FF);
        decoded_bytes.push_back((temp >> 8) & 0x000000FF);
        decoded_bytes.push_back(temp & 0x000000FF);
    }
    return decoded_bytes;
}

std::string HexSubstr(const std::string& str) {
    size_t non_hex_size(str.size());
    if (non_hex_size < 7)
        return HexEncode(str);

    std::string hex(14, 0);
    size_t non_hex_index(0), hex_index(0);
    for (; non_hex_index != 3; ++non_hex_index) {
        hex[hex_index++] = kHexAlphabet[static_cast<unsigned char>(str[non_hex_index]) / 16];
        hex[hex_index++] = kHexAlphabet[static_cast<unsigned char>(str[non_hex_index]) % 16];
    }
    hex[hex_index++] = '.';
    hex[hex_index++] = '.';
    non_hex_index = non_hex_size - 3;
    for (; non_hex_index != non_hex_size; ++non_hex_index) {
        hex[hex_index++] = kHexAlphabet[static_cast<unsigned char>(str[non_hex_index]) / 16];
        hex[hex_index++] = kHexAlphabet[static_cast<unsigned char>(str[non_hex_index]) % 16];
    }
    return hex;
}

std::string Base64Substr(const std::string& str) {
    std::string base64(Base64Encode(str));
    if (base64.size() > 16)
        return (base64.substr(0, 7) + ".." + base64.substr(base64.size() - 7));
    else
        return base64;
}

uint64_t GetCurrentTimeMsec() {
#ifdef _WIN32
    struct timeval tv;
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tv.tv_sec = clock;
    tv.tv_usec = wtm.wMilliseconds * 1000;
    return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000);
#endif
}

uint64_t GetCurrentTimeMicSec() {
#ifdef _WIN32
    struct timeval tv;
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tv.tv_sec = clock;
    tv.tv_usec = wtm.wMilliseconds * 1000;
    return ((uint64_t)tv.tv_sec * 1000 * 1000 + (uint64_t)tv.tv_usec);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000 * 1000 + (uint64_t)tv.tv_usec);
#endif
}

void TrimString(std::string& in_str) {
    if (in_str.empty()) {
        return;
    }

    in_str.erase(0, in_str.find_first_not_of(" "));
    in_str.erase(in_str.find_last_not_of(" ") + 1);
}

void SleepUs(uint64_t time_us) {
    std::this_thread::sleep_for(std::chrono::microseconds(time_us));
}

void SleepMs(uint64_t time_ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
}
std::string DecodePrivateString(const std::string & pri_key)
{
    constexpr int HEX_PRI_KEY_LEN = 64;
    if (pri_key.size() != HEX_PRI_KEY_LEN)
        return Base64Decode(pri_key);
    else
        return top::HexDecode(pri_key);
}

}  // namespace top
