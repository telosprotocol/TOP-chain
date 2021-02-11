// //////////////////////////////////////////////////////////
// crc32.h
// Copyright (c) 2014 Stephan Brumme. All rights reserved.
// see http://create.stephan-brumme.com/disclaimer.html
/*
 This software is provided 'as-is', without any express or implied warranty. In no event will the author be held liable for any damages arising from the use of this software.
 Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
 If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 */

#pragma once

#include <string>
#include <stdint.h>

//introduce namesapce control based on original code
namespace top {
    namespace utl {
        
        /// compute CRC32 hash, based on Intel's Slicing-by-8 algorithm
        /** Usage:
         xcrc32_t obj;
         std::string myHash  = obj("Hello World");     // std::string
         std::string myHash2 = obj("How are you", 11); // arbitrary data, 11 bytes
         
         // or in a streaming fashion:
         
         xcrc32_t obj;
         while (more data available)
         obj.add(pointer to fresh data, number of new bytes);
         std::string myHash3 = obj.getHash();
         */
        class xcrc32_t
        {
        public:
            static uint32_t    crc32(const void* data, size_t numBytes);
            static uint32_t    crc32(const std::string& text);
            static std::string crc32_to_string(const uint32_t int_hash);
        public:
            /// same as reset()
            xcrc32_t();

            /// add arbitrary number of bytes
            void add(const void* data, size_t numBytes);
            void add(const std::string& text);
            
            /// return latest hash as hex characters
            std::string get_hash_string();
            uint32_t    get_hash_int();
            
            /// restart
            void reset();
        private:
            /// hash
            uint32_t m_hash;
        };
    };  //end of namespace utl
};  //end of namespace of top
