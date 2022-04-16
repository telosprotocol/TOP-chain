// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <map>
#include <set>
#include <assert.h>
#include <cmath>
#include <cstring>
#include <type_traits>
#include <iostream>
#include "xevm_common/common.h"

namespace top {
namespace evm_common {

#if __cplusplus >= 201703L
    #define xborsh_const constexpr
#else 
    #define xborsh_const 
#endif

    namespace xBorshInternals {
            template <typename T>
            struct is_string {
                static const bool value = false;
            };
            template <class T, class Traits, class Alloc>
            struct is_string<std::basic_string<T, Traits, Alloc>> {
                static const bool value = true;
            };

    }

    class xBorshEncoder
    {
    public:
            template <typename T>
            xborsh_const xBorshEncoder &EncodeInteger(const T integer) {
                static_assert(std::is_integral<T>::value || std::is_same<bigint, T>::value || !std::numeric_limits<T>::is_signed, "Integer value only");
                size_t typeSize = sizeof(T);
                uint8_t offset = 0;

                if(std::is_same<u256, T>::value) {
                    typeSize = 32;
                }

                for (size_t i = 0; i < typeSize; i++) {
                    uint8_t v = (integer>>offset) & (T)0xff;
                    m_Buffer.push_back(v);
                    offset += 8;
                }
                return *this;
            }

            xBorshEncoder &EncodeString(const std::string &str) {
                // Write the size of the string as an u32 integer
                EncodeInteger(static_cast<uint32_t>(str.size()));
                m_Buffer.insert(m_Buffer.begin() + m_Buffer.size(), str.begin(), str.end());
                return *this;
            }

            xBorshEncoder &EncodeString(const char *str) {
                const uint32_t size = std::strlen(str);
                // Write the size of the string as an u32 integer
                EncodeInteger(size);
                auto bytes = reinterpret_cast<const uint8_t *>(str);
                m_Buffer.insert(m_Buffer.begin() + m_Buffer.size(), bytes, bytes + size);
                return *this;
            }

            template <typename T>
            xborsh_const xBorshEncoder &EncodeFixArray(const std::initializer_list<T> &initList) {
                return EncodeFixArray<T>(initList.begin(), initList.size());
            }

            template <typename T>
            xborsh_const xBorshEncoder &EncodeFixArray(const T *array, size_t size) {
                if (std::is_array<T>::value) {
                        printf("Es array");
                }

                if xborsh_const (std::is_integral<T>::value) {
                        for (size_t i = 0; i < size; ++i) {
                            EncodeInteger(*array);
                            ++array;
                        }
                }
                else if xborsh_const (xBorshInternals::is_string<T>::value) {
                        for (size_t i = 0; i < size; ++i) {
                            EncodeString(reinterpret_cast<const char*>(*array));
                            ++array;
                        }
                }
                else if xborsh_const (std::is_same<const char *, T>::value) {
                        for (size_t i = 0; i < size; ++i) {
                            EncodeString(reinterpret_cast<const char*>(*array));
                            ++array;
                        }
                }
                else {
                        assert(false || "The type of the array is not supported");
                }

                return *this;
            }

            template <typename T>
            xborsh_const xBorshEncoder &EncodeDynamicArray(const std::vector<T> &vector) {
                EncodeInteger((uint32_t)vector.size());
                EncodeFixArray(vector.data(), vector.size());

                return *this;
            }

            const std::vector<uint8_t> &GetBuffer() const {
                return m_Buffer;
            }

    private:
            std::vector<uint8_t> m_Buffer;
    };

    class xBorshDecoder
    {
    public:

            template < class _In, class T>
            inline void getInteger(_In const &_bytes, T &out) {
                int offset = 0;
                for (auto i : _bytes){
                    out = (T)(out | ((T)(typename std::make_unsigned<decltype(i)>::type)i << offset ));    
                    offset += 8;
                }
            }   

            inline bool getBool(const std::string &str) {
                uint8_t value = 0 ;
                getInteger(str, value);
                return (value > 0);
            }

           inline  void getString(const std::string &srcStr, std::string  &resultStr) {
                uint32_t strLen = srcStr.length();
                uint32_t dataLen = 0 ;
                std::string strpre = srcStr.substr(0,4);
                getInteger(strpre, dataLen);
                if ((dataLen + 4) ==   strLen) {
                    resultStr = srcStr.substr(4, dataLen);
                }else {
                     assert(false || "The string len is error");
                }
            }
    };

}
}