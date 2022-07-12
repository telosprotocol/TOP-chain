#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace xChainSDK {

    class utility {

    public:
        static std::string urlencode(const std::string& input) noexcept;
        static std::string urldecode(const std::string& input) noexcept;

        static std::string base64_encode(unsigned char const* str, unsigned int len);
        static std::string base64_decode(const std::string& str);

        static void str_to_hex(char* string, unsigned char* cbuf, int len);
        static void hex_to_str(char* ptr, unsigned char* buf, int len);

        static int64_t gmttime_ms();   //at ms level of UTC base time

        static void split_string(const std::string& source,
            char key, std::vector<std::string>& result);
        static bool is_number(const std::string& str);
        static bool is_ipaddress_valid(const std::string& str);

        static std::string num_to_str(uint64_t num);

        static void usleep(int us);
    };
}

