// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xutility/xhash.h"

#define XXH_INLINE_ALL  //define it first,then include xxhash.h
#include "xutility/xxHash/xxhash.h" //from xxhash lib

extern "C"
{
    #include "trezor-crypto/ripemd160.h"
    #include "trezor-crypto/blake256.h"
    #include "trezor-crypto/sha2.h"
    #include "trezor-crypto/sha3.h"
}
#include "xmetrics/xmetrics.h"

#define ENABLE_HASH_METRICS

namespace top
{
    namespace utl
    {
        /// convert litte vs big endian
        uint32_t swap32(uint32_t x)
        {
            #if defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap32(x);
            #endif
            #ifdef MSC_VER
            return _byteswap_ulong(x);
            #endif

            return (x >> 24) |
            ((x >>  8) & 0x0000FF00) |
            ((x <<  8) & 0x00FF0000) |
            (x << 24);
        }

        /// convert litte vs big endian
        uint64_t swap64(uint64_t x)
        {
            #if defined(__GNUC__) || defined(__clang__)
            return __builtin_bswap64(x);
            #endif
            #ifdef MSC_VER
            return _byteswap_uint64(x);
            #endif

            return  (x >> 56) |
            ((x >> 40) & 0x000000000000FF00ULL) |
            ((x >> 24) & 0x0000000000FF0000ULL) |
            ((x >>  8) & 0x00000000FF000000ULL) |
            ((x <<  8) & 0x000000FF00000000ULL) |
            ((x << 24) & 0x0000FF0000000000ULL) |
            ((x << 40) & 0x00FF000000000000ULL) |
            (x << 56);
        }

        extern "C"
        {
            //ripemd 160
            void c_ripemd_160_init(void * context)
            {
                ripemd160_Init((RIPEMD160_CTX*)context);
            }
            void c_ripemd_160_update(void * context, const void *data, size_t length)
            {
                ripemd160_Update((RIPEMD160_CTX*)context,(const uint8_t*)data, (uint32_t)length);
            }
            void c_ripemd_160_final(void *context, unsigned char* result)
            {
                ripemd160_Final((RIPEMD160_CTX*)context, result);
            }

            //blake2 256
            void c_blake2_256_init(void * context)
            {
                blake256_Init((BLAKE256_CTX*)context);
            }
            void c_blake2_256_update(void * context, const void *data, size_t length)
            {
                blake256_Update((BLAKE256_CTX *)context,(const uint8_t*)data, length);
            }
            void c_blake2_256_final(void *context, unsigned char* result)
            {
                blake256_Final((BLAKE256_CTX *)context, result);
            }

            //sha2 256
            void c_sha2_256_init(void * context)
            {
                sha256_Init((SHA256_CTX *)context);
            }
            void c_sha2_256_update(void * context, const void *data, size_t length)
            {
                sha256_Update((SHA256_CTX *)context, (const uint8_t*)data, length);
            }
            void c_sha2_256_final(void *context, unsigned char* result)
            {
                sha256_Final((SHA256_CTX *)context, result);
            }
            //sha 512
            void c_sha2_512_init(void * context)
            {
                sha512_Init((SHA512_CTX *)context);
            }
            void c_sha2_512_update(void * context, const void *data, size_t length)
            {
                sha512_Update((SHA512_CTX *)context, (const uint8_t*)data, length);
            }
            void c_sha2_512_final(void *context, unsigned char* result)
            {
                sha512_Final((SHA512_CTX *)context, result);
            }

            //sha3 256 and 512
            void c_sha3_256_init(void * context)
            {
                sha3_256_Init((SHA3_CTX*)context);
            }
            void c_sha3_512_init(void * context)
            {
                sha3_512_Init((SHA3_CTX*)context);
            }
            void c_sha3_update(void * context, const void *data, size_t length)
            {
                sha3_Update((SHA3_CTX*)context, (const unsigned char*)data, length);
            }
            void c_sha3_final(void *context, unsigned char* result)
            {
                sha3_Final((SHA3_CTX*)context, result);
            }
            void c_keccak_final(void *context, unsigned char* result)
            {
                keccak_Final((SHA3_CTX*)context, result);
            }
        }

        ////////////////////////////////////xripemd160_t////////////////////////////////////////////////
        xripemd160_t::xripemd160_t()
        {
            _context = malloc(sizeof(RIPEMD160_CTX));
            reset();
        }
        xripemd160_t::~xripemd160_t()
        {
            free(_context);
        }

        bool     xripemd160_t::reset()/// restart
        {
            c_ripemd_160_init(_context);
            return true;
        }
        int     xripemd160_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }
        int     xripemd160_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_ripemd_160_update(_context,data,length);
            return (int)length;
        }
        bool    xripemd160_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint160_t hash;
            get_hash(hash);

            raw_bytes.resize(uint160_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint160_t::enum_xint_size_bytes);
            return true;
        }
        bool    xripemd160_t::get_hash(uint160_t & hash) //raw hash as 160bit
        {
            c_ripemd_160_final(_context, hash.raw_uint8);
            return true;
        }
        uint160_t  xripemd160_t::digest(const void* data, size_t numBytes)
        {
            xripemd160_t   hasher;
            uint160_t      output;
            hasher.update(data, numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint160_t  xripemd160_t::digest(const std::string & text)
        {
            xripemd160_t   hasher;
            uint160_t      output;
            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////xblake2_256_t////////////////////////////////////////////////
        xblake2_256_t::xblake2_256_t()
        {
            _context = malloc(sizeof(BLAKE256_CTX));
            reset();
        }
        xblake2_256_t::~xblake2_256_t()
        {
            free(_context);
        }

        bool     xblake2_256_t::reset()/// restart
        {
            c_blake2_256_init(_context);
            return true;
        }
        int     xblake2_256_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }
        int     xblake2_256_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_blake2_256_update(_context,data,length);
            return (int)length;
        }
        bool    xblake2_256_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint256_t hash;
            get_hash(hash);

            raw_bytes.resize(uint256_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint256_t::enum_xint_size_bytes);
            return true;
        }
        bool    xblake2_256_t::get_hash(uint256_t & hash) //raw hash as 256bit
        {
            c_blake2_256_final(_context, hash.raw_uint8);
            return true;
        }
        uint256_t  xblake2_256_t::digest(const void* data, size_t numBytes)
        {
            xblake2_256_t hasher;
            uint256_t     output;
            hasher.update(data, numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint256_t  xblake2_256_t::digest(const std::string & text)
        {
            xblake2_256_t hasher;
            uint256_t     output;
            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////xsha2_256_t////////////////////////////////////////////////
        xsha2_256_t::xsha2_256_t()
        {
            _context = malloc(sizeof(SHA256_CTX));
            reset();
        }
        xsha2_256_t::~xsha2_256_t()
        {
            free(_context);
        }

        bool     xsha2_256_t::reset()/// restart
        {
            c_sha2_256_init(_context);
            return true;
        }
        int     xsha2_256_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }
        int     xsha2_256_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_sha2_256_update(_context,data,length);
            return (int)length;
        }
        bool    xsha2_256_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint256_t hash;
            get_hash(hash);

            raw_bytes.resize(uint256_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint256_t::enum_xint_size_bytes);
            return true;
        }
        bool    xsha2_256_t::get_hash(uint256_t & hash) //raw hash as 256bit
        {
            c_sha2_256_final(_context, hash.raw_uint8);
            XMETRICS_GAUGE(metrics::cpu_hash_256_calc, 1);
            return true;
        }
        uint256_t  xsha2_256_t::digest(const void* data, size_t numBytes)
        {
            xsha2_256_t hasher;
            uint256_t   output;
            hasher.update(data, numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint256_t  xsha2_256_t::digest(const std::string & text)
        {
            xsha2_256_t hasher;
            uint256_t   output;
            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }
        ////////////////////////////////////xsha2_512_t////////////////////////////////////////////////
        xsha2_512_t::xsha2_512_t()
        {
            _context = malloc(sizeof(SHA512_CTX));
            reset();
        }
        xsha2_512_t::~xsha2_512_t()
        {
            free(_context);
        }

        bool     xsha2_512_t::reset()/// restart
        {
            c_sha2_512_init(_context);
            return true;
        }
        int     xsha2_512_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }
        int     xsha2_512_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_sha2_512_update(_context,data,length);
            return (int)length;
        }
        bool    xsha2_512_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint512_t hash;
            get_hash(hash);

            raw_bytes.resize(uint512_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint512_t::enum_xint_size_bytes);
            return true;
        }
        bool    xsha2_512_t::get_hash(uint512_t & hash) //raw hash as 256bit
        {
            c_sha2_512_final(_context, hash.raw_uint8);
            return true;
        }
        uint512_t  xsha2_512_t::digest(const void* data, size_t numBytes)
        {
            xsha2_512_t   hasher;
            uint512_t     output;
            hasher.update(data, numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint512_t  xsha2_512_t::digest(const std::string & text)
        {
            xsha2_512_t   hasher;
            uint512_t     output;
            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////xsha3_256_t////////////////////////////////////////////////
        xsha3_256_t::xsha3_256_t()
        {
            _context = malloc(sizeof(SHA3_CTX));
            reset();
        }
        xsha3_256_t::~xsha3_256_t()
        {
            free(_context);
        }

        bool     xsha3_256_t::reset()/// restart
        {
            c_sha3_256_init(_context);
            return true;
        }
        int     xsha3_256_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }
        int     xsha3_256_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_sha3_update(_context,data,length);
            return (int)length;
        }
        bool    xsha3_256_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint256_t hash;
            get_hash(hash);

            raw_bytes.resize(uint256_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint256_t::enum_xint_size_bytes);
            return true;
        }
        bool    xsha3_256_t::get_hash(uint256_t & hash) //raw hash as 256bit
        {
            c_sha3_final(_context, hash.raw_uint8);
            return true;
        }
        uint256_t  xsha3_256_t::digest(const void* data, size_t numBytes)
        {
            xsha3_256_t hasher;
            uint256_t   output;
            hasher.update(data, numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint256_t  xsha3_256_t::digest(const std::string & text)
        {
            xsha3_256_t hasher;
            uint256_t   output;
            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////xsha3_512_t////////////////////////////////////////////////
        xsha3_512_t::xsha3_512_t()
        {
            _context = malloc(sizeof(SHA3_CTX));
            reset();
        }
        xsha3_512_t::~xsha3_512_t()
        {
            free(_context);
        }

        bool     xsha3_512_t::reset()/// restart
        {
            c_sha3_512_init(_context);
            return true;
        }
        int     xsha3_512_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }
        int     xsha3_512_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_sha3_update(_context,data,length);
            return (int)length;
        }
        bool    xsha3_512_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint512_t hash;
            get_hash(hash);

            raw_bytes.resize(uint512_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint512_t::enum_xint_size_bytes);
            return true;
        }
        bool    xsha3_512_t::get_hash(uint512_t & hash) //raw hash as 256bit
        {
            c_sha3_final(_context, hash.raw_uint8);
            return true;
        }
        uint512_t  xsha3_512_t::digest(const void* data, size_t numBytes)
        {
            xsha3_512_t hasher;
            uint512_t   output;
            hasher.update(data, numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint512_t  xsha3_512_t::digest(const std::string & text)
        {
            xsha3_512_t hasher;
            uint512_t   output;
            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////keccak256_t////////////////////////////////////////////////
        xkeccak256_t::xkeccak256_t()
        {
            _context = malloc(sizeof(SHA3_CTX));
            reset();
        }
        xkeccak256_t::~xkeccak256_t()
        {
            free(_context);
        }

        bool     xkeccak256_t::reset()/// restart
        {
            c_sha3_256_init(_context);
            return true;
        }

        int     xkeccak256_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }

        int     xkeccak256_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_sha3_update(_context,data,length);
            return (int)length;
        }
        bool    xkeccak256_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint256_t hash;
            get_hash(hash);

            raw_bytes.resize(uint256_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint256_t::enum_xint_size_bytes);
            return true;
        }

        bool    xkeccak256_t::get_hash(uint256_t & hash) //raw hash as 256bit
        {
            c_keccak_final(_context, hash.raw_uint8);
            return true;
        }

        uint256_t  xkeccak256_t::digest(const void* data, size_t numBytes)
        {
            xkeccak256_t hasher;
            uint256_t    output;

            hasher.update(data,numBytes);
            hasher.get_hash(output);
            return output;
        }
        uint256_t  xkeccak256_t::digest(const std::string & text)
        {
            xkeccak256_t hasher;
            uint256_t    output;

            hasher.update(text);
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////keccak512_t////////////////////////////////////////////////
        xkeccak512_t::xkeccak512_t()
        {
            _context = malloc(sizeof(SHA3_CTX));
            reset();
        }
        xkeccak512_t::~xkeccak512_t()
        {
            free(_context);
        }

        bool     xkeccak512_t::reset()/// restart
        {
            c_sha3_512_init(_context);
            return true;
        }

        int     xkeccak512_t::update(const std::string & text)
        {
            return update(text.data(),text.size());
        }

        int     xkeccak512_t::update(const void* data, size_t length)
        {
            if( (nullptr == data) || (length <= 0) )
                return 0;

            c_sha3_update(_context,data,length);
            return (int)length;
        }
        bool    xkeccak512_t::get_hash(std::vector<uint8_t> & raw_bytes)//get raw hash data
        {
            uint512_t hash;
            get_hash(hash);

            raw_bytes.resize(uint512_t::enum_xint_size_bytes);
            memcpy(raw_bytes.data(),hash.raw_uint8,uint512_t::enum_xint_size_bytes);
            return true;
        }

        bool    xkeccak512_t::get_hash(uint512_t & hash) //raw hash as 256bit
        {
            c_keccak_final(_context, hash.raw_uint8);
            return true;
        }

        uint512_t  xkeccak512_t::digest(const void* data, size_t numBytes)
        {
            xkeccak512_t hasher;
            hasher.update(data, numBytes);

            uint512_t output;
            hasher.get_hash(output);
            return output;
        }
        uint512_t  xkeccak512_t::digest(const std::string & text)
        {
            xkeccak512_t hasher;
            hasher.update(text);

            uint512_t output;
            hasher.get_hash(output);
            return output;
        }

        ////////////////////////////////////xxh32_t////////////////////////////////////////////////////
        uint32_t  xxh32_t::digest(const void* data, size_t numBytes)
        {
            if( (nullptr == data) || (numBytes <= 0) )
                return 0;

            XXH32_state_t hash;
            XXH32_reset(&hash, 0);
            XXH32_update(&hash,data,numBytes);
            return XXH32_digest(&hash);
        }

        uint32_t  xxh32_t::digest(const std::string & text)
        {
            if(text.empty())
                return 0;

            XXH32_state_t hash;
            XXH32_reset(&hash, 0);
            XXH32_update(&hash,text.data(),text.size());
            return XXH32_digest(&hash);
        }

        xxh32_t::xxh32_t()
        {
            xxhash_state = XXH32_createState();
            XXH32_reset((XXH32_state_t*)xxhash_state, 0);
        }

        xxh32_t::~xxh32_t()
        {
            XXH32_freeState((XXH32_state_t*)xxhash_state);
        }

        /// add arbitrary number of bytes
        int         xxh32_t::update(const void* data, size_t numBytes)
        {
            if( (nullptr == data) || (numBytes <= 0) )
                return 0;

            XXH32_update((XXH32_state_t*)xxhash_state,data,numBytes);
            return (int)numBytes;
        }

        int        xxh32_t::update(const std::string & text)
        {
            if(text.empty())
                return 0;

            XXH32_update((XXH32_state_t*)xxhash_state,text.data(),text.size());
            return (int)text.size();
        }

        bool    xxh32_t::get_hash(std::vector<uint8_t> & raw_bytes)
        {
            const uint32_t raw_hash_value = get_hash();
            raw_bytes.resize(sizeof(raw_hash_value));
            memcpy(raw_bytes.data(), &raw_hash_value, sizeof(raw_hash_value));
            return true;
        }

        uint32_t    xxh32_t::get_hash()
        {
            return XXH32_digest((XXH32_state_t*)xxhash_state);
        }
        /// restart
        bool        xxh32_t::reset()
        {
            XXH32_reset((XXH32_state_t*)xxhash_state, 0);
            return true;
        }

        ////////////////////////////////////xxh64_t////////////////////////////////////////////////////
        uint64_t  xxh64_t::digest(const void* data, size_t numBytes)
        {
            if( (nullptr == data) || (numBytes <= 0) )
                 return 0;

            XXH64_state_t hash;
            XXH64_reset(&hash, 0);
            XXH64_update(&hash,data,numBytes);
            return XXH64_digest(&hash);
        }

        uint64_t  xxh64_t::digest(const std::string & text)
        {
            if(text.empty())
                return 0;

            XXH64_state_t hash;
            XXH64_reset(&hash, 0);
            XXH64_update(&hash,text.data(),text.size());
            return XXH64_digest(&hash);
        }

        xxh64_t::xxh64_t()
        {
            xxhash_state = XXH64_createState();
            XXH64_reset((XXH64_state_t*)xxhash_state, 0);
        }

        xxh64_t::~xxh64_t()
        {
            XXH64_freeState((XXH64_state_t*)xxhash_state);
        }

        /// add arbitrary number of bytes
        int   xxh64_t::update(const void* data, size_t numBytes)
        {
            if( (nullptr == data) || (numBytes <= 0) )
                return 0;

            XXH64_update((XXH64_state_t*)xxhash_state,data,numBytes);
            return (int)numBytes;
        }

        int   xxh64_t::update(const std::string & text)
        {
            if(text.empty())
                return 0;

            XXH64_update((XXH64_state_t*)xxhash_state,text.data(),text.size());
            return (int)text.size();
        }

        bool  xxh64_t::get_hash(std::vector<uint8_t> & raw_bytes)
        {
            const uint64_t hash = get_hash();
            raw_bytes.resize(sizeof(hash));
            memcpy(raw_bytes.data(), &hash, sizeof(hash));
            return true;
        }

        uint64_t    xxh64_t::get_hash()
        {
            return XXH64_digest((XXH64_state_t*)xxhash_state);
        }
        /// restart
        bool        xxh64_t::reset()
        {
            XXH64_reset((XXH64_state_t*)xxhash_state, 0);
            return true;
        }
    }
} //end of namespace top

