#include "utility.h"

#include <time.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <sstream>
#include <memory>
#include <thread>


namespace xChainSDK {

    std::string utility::urlencode(const std::string& input) noexcept {
        auto hex_chars = "0123456789ABCDEF";

        std::string result;
        result.reserve(input.size()); // Minimum size of result

        for (auto& chr : input) {
            if (!((chr >= '0' && chr <= '9') ||
                (chr >= 'A' && chr <= 'Z') ||
                (chr >= 'a' && chr <= 'z') ||
                chr == '-' ||
                chr == '.' ||
                chr == '_' ||
                chr == '~')) {
                result += std::string("%") +
                    hex_chars[static_cast<unsigned char>(chr) >> 4] +
                    hex_chars[static_cast<unsigned char>(chr) & 15];
            }
            else {
                result += chr;
            }
        }
        return result;
    }

    std::string utility::urldecode(const std::string& input) noexcept {
        std::string result;
        result.reserve(input.size() / 3 + (input.size() % 3)); // Minimum size of result

        for (std::size_t i = 0; i < input.size(); ++i) {
            auto& chr = input[i];
            if (chr == '%' && i + 2 < input.size()) {
                auto hex = input.substr(i + 1, 2);
                auto decoded_chr = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
                result += decoded_chr;
                i += 2;
            }
            else if (chr == '+')
                result += ' ';
            else
                result += chr;
        }

        return result;
    }

    std::wstring s2ws(const std::string& str) {
        if (str.empty())
            return L"";

        unsigned len = str.size() + 1;
        setlocale(LC_CTYPE, "en_US.UTF-8");
        std::unique_ptr<wchar_t[]> p(new wchar_t[len]);
        mbstowcs(p.get(), str.c_str(), len);
        std::wstring w_str(p.get());
        return w_str;
    }



    std::string ws2s(const std::wstring& w_str) {
        if (w_str.empty())
            return "";

        unsigned len = w_str.size() * 4 + 1;
        setlocale(LC_CTYPE, "en_US.UTF-8");
        std::unique_ptr<char[]> p(new char[len]);
        wcstombs(p.get(), w_str.c_str(), len);
        std::string str(p.get());
        return str;
    }

    std::string utility::base64_encode(unsigned char const* str, unsigned int len) {
        auto _base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string _encode_result;
        const unsigned char* current;
        current = str;
        while (len > 2) {
            _encode_result += _base64_table[current[0] >> 2];
            _encode_result += _base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
            _encode_result += _base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
            _encode_result += _base64_table[current[2] & 0x3f];

            current += 3;
            len -= 3;
        }
        if (len > 0)
        {
            _encode_result += _base64_table[current[0] >> 2];
            if (len % 3 == 1) {
                _encode_result += _base64_table[(current[0] & 0x03) << 4];
                _encode_result += "==";
            }
            else if (len % 3 == 2) {
                _encode_result += _base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
                _encode_result += _base64_table[(current[1] & 0x0f) << 2];
                _encode_result += "=";
            }
        }
        return _encode_result;
    }

    std::string utility::base64_decode(const std::string& str) {

        const char base64_pad = '=';
        const char DecodeTable[] = {
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
            -2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
            -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
            -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
        };
        int bin = 0, i = 0;
        std::string _decode_result;
        size_t length = str.length();
        const char* current = str.c_str();
        char ch;
        while ((ch = *current++) != '\0' && length-- > 0) {
            if (ch == base64_pad) {
                if (*current != '=' && (i % 4) == 1) {
                    return NULL;
                }
                continue;
            }
            ch = DecodeTable[static_cast<size_t>(ch)];
            
            if (ch < 0) {
                continue;
            }
            switch (i % 4) {
            case 0:
                bin = ch << 2;
                break;
            case 1:
                bin |= ch >> 4;
                _decode_result += bin;
                bin = (ch & 0x0f) << 4;
                break;
            case 2:
                bin |= ch >> 2;
                _decode_result += bin;
                bin = (ch & 0x03) << 6;
                break;
            case 3:
                bin |= ch;
                _decode_result += bin;
                break;
            }
            i++;
        }
        return _decode_result;
    }

    void utility::str_to_hex(char* string, unsigned char* cbuf, int len) {
        char str[4];
        str[0] = '0';
        str[1] = 'x';
        for (int i = 0; i < len; ++i) {
            char* pEnd;
            str[2] = cbuf[2 * i];
            str[3] = cbuf[2 * i + 1];
            long val = strtol(str, &pEnd, 16);
            string[i] = val;
        }
    }

    void utility::hex_to_str(char* ptr, unsigned char* buf, int len) {
        for (int i = 0; i < len; i++) {
            sprintf(ptr, "%02x", buf[i]);
            ptr += 2;
        }
    }

    int64_t utility::gmttime_ms() {
        std::chrono::time_point<
            std::chrono::system_clock,
            std::chrono::milliseconds> tp = 
            std::chrono::time_point_cast<
            std::chrono::milliseconds>(
                std::chrono::system_clock::now());
        auto tmp = std::chrono::duration_cast<
            std::chrono::milliseconds>(
                tp.time_since_epoch());
        auto timestamp = tmp.count();
        return timestamp;
    }

    void utility::split_string(const std::string& source,
        char key, std::vector<std::string>& result) {

        size_t len = source.length();
        size_t pos_beg{ 0 };
        size_t pos_end = pos_beg;
        while (pos_end != std::string::npos && pos_beg < len) {
            pos_end = source.find(key, pos_beg);
            if (pos_end == std::string::npos) {
                result.push_back(source.substr(pos_beg, len - pos_beg));
            }
            else {
                result.push_back(source.substr(pos_beg, pos_end - pos_beg));
                pos_beg = pos_end + 1;
            }
        }
    }

    bool utility::is_number(const std::string& str) {
        for (size_t i = 0; i < str.size(); i++) {
            int tmp = (int)str[i];
            if (tmp < 48 || tmp > 57)
                return false;
        }
        return true;
    }

    bool utility::is_ipaddress_valid(const std::string& str) {       
        std::string port_str{ "" };
        std::vector<std::string> vc_port;
        split_string(str, ':', vc_port);
        if (vc_port.size() > 2)
            return false;

        std::string ip_str = vc_port[0];
        if (vc_port.size() == 2)
            port_str = vc_port[1];

        // check port
        if (!port_str.empty() && !is_number(port_str))
            return false;

        // check ip
        std::vector<std::string> vc_ip;
        split_string(ip_str, '.', vc_ip);
        if (vc_ip.size() != 4)
            return false;
        for (auto i : vc_ip) {
            if (!is_number(i))
                return false;
            auto num = atol(i.c_str());
            if (num > 255 || num < 0)
                return false;
        }
        return true;
    }

    std::string utility::num_to_str(uint64_t num) {
        std::ostringstream out;
        out << std::dec;
        out << num;
        return out.str();
    }

    void utility::usleep(int us) {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
}
