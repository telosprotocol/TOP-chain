// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xint.h"
#include "xbase/xns_macro.h"

#include <cstdint>
#include <string>
#include <vector>


NS_BEG2(top, utl)

/// convert litte vs big endian
uint32_t swap32(uint32_t x);
uint64_t swap64(uint64_t x);

class xhash_t
{
protected:
    xhash_t() = default;
    virtual ~xhash_t() = default;

public:
    enum class xhash_type_t : std::uint8_t
    {
        md5        = 0x01,
        sha1       = 0x10,
        sha2_256   = 0x21,
        sha2_512   = 0x22,
        blake2_256 = 0x23,
        ripemd_160 = 0x28,
        sha3_256   = 0x31,
        sha3_512   = 0x32,
        keccak_256 = 0x33,
        keccak_512 = 0x34,
        xxh_32     = 0x70,
        xxh_64     = 0x71,
    };
public:
    virtual xhash_type_t        get_type() const = 0;
    virtual int                     get_digest_length() const = 0; //return how many bytes of digest
    virtual int                     update(const std::string & text) = 0; //return howmany bytes added
    virtual int                     update(const void* data, size_t numBytes) = 0; //return howmany bytes added
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) = 0; //get raw hash data
};

/// compute MD5 hash
/** Usage:
 xmd5_t md5;
 std::string myHash  = md5("Hello World");     // std::string
 std::string myHash2 = md5("How are you", 11); // arbitrary data, 11 bytes

 // or in a streaming fashion:

 xmd5_t md5;
 while (more data available)
 md5.add(pointer to fresh data, number of new bytes);
 std::string myHash3 = md5.getHash();
 */
 //md5 is 128bit' hash
class xmd5_t final : public xhash_t
{
public:
    /// compute MD5 of a memory block
    static std::string digest(const void* data, size_t numBytes);
    /// compute MD5 of a string, excluding final zero
    static std::string digest(const std::string& text);
public:
    /// same as reset()
    xmd5_t();

    xhash_type_t
    get_type() const override
    {
        return xhash_type_t::md5;
    }

    //return how many bytes of digest
    int
    get_digest_length() const override
    {
        return 16;
    }

    bool reset() ;

    int
    update(const std::string & text) override;

    int
    update(const void* data, size_t numBytes) override;

    //get raw hash data
    bool
    get_hash(std::vector<uint8_t> & raw_bytes) override;

    // return latest hash as 16 hex characters
    std::string
    get_hex_hash();

private:
    /// process 64 bytes
    void processBlock(const void* data);
    /// process everything left in the internal buffer
    void processBuffer();

    /// split into 64 byte blocks (=> 512 bits)
    enum { BlockSize = 512 / 8 };

    /// size of processed data in bytes
    uint64_t m_numBytes;
    /// valid bytes in m_buffer
    size_t   m_bufferSize;
    /// bytes not processed yet
    uint8_t  m_buffer[BlockSize];
    /// hash, stored as integers
    uint32_t m_hash[4];
};

//
//160bit RIPEMD160 hash
class xripemd160_t final : public xhash_t
{
public:
    static uint160_t  digest(const void* data, size_t numBytes);
    static uint160_t  digest(const std::string & text);

    xripemd160_t();
    ~xripemd160_t();

    xhash_type_t
    get_type() const override
    {
        return xhash_type_t::ripemd_160;
    }

    int
    get_digest_length() const override
    {
        return 20;
    } //return how many bytes of digest

    bool reset() ;/// restart

    int
    update(const std::string & text) override;

    int
    update(const void* data, size_t numBytes) override;

    bool
    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data

    bool
    get_hash(uint160_t & hash); //raw hash as 160bit

private:
    void* _context;
};

//256bit blake2 hash
class xblake2_256_t : public xhash_t
{
public:
    static uint256_t  digest(const void* data, size_t numBytes);
    static uint256_t  digest(const std::string & text);
public:
    xblake2_256_t();
    ~xblake2_256_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::blake2_256; }
    virtual int                     get_digest_length() const override { return 32; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint256_t & hash); //raw hash as 256bit
private:
    void* _context;
};

//256bit standard sha2
class xsha2_256_t : public xhash_t
{
public:
    static uint256_t  digest(const void* data, size_t numBytes);
    static uint256_t  digest(const std::string & text);
public:
    xsha2_256_t();
    ~xsha2_256_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::sha2_256; }
    virtual int                     get_digest_length() const override { return 32; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint256_t & hash); //raw hash as 256bit
private:
    void* _context;
};
//512bit    standard sha2
class xsha2_512_t : public xhash_t
{
public:
    static uint512_t  digest(const void* data, size_t numBytes);
    static uint512_t  digest(const std::string & text);
public:
    xsha2_512_t();
    ~xsha2_512_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::sha2_512; }
    virtual int                     get_digest_length() const override { return 64; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint512_t & hash); //raw hash as 512bit
private:
    void* _context;
};

//256bit standard sha3
class xsha3_256_t : public xhash_t
{
public:
    static uint256_t  digest(const void* data, size_t numBytes);
    static uint256_t  digest(const std::string & text);
public:
    xsha3_256_t();
    ~xsha3_256_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::sha3_256; }
    virtual int                     get_digest_length() const override { return 32; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint256_t & hash); //raw hash as 256bit
private:
    void* _context;
};
//512bit    standard sha3
class xsha3_512_t : public xhash_t
{
public:
    static uint512_t  digest(const void* data, size_t numBytes);
    static uint512_t  digest(const std::string & text);
public:
    xsha3_512_t();
    ~xsha3_512_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::sha3_512; }
    virtual int                     get_digest_length() const override { return 64; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint512_t & hash); //raw hash as 512bit
private:
    void* _context;
};

//256bit keccak(sha3 family)
class xkeccak256_t : public xhash_t
{
public:
    static uint256_t  digest(const void* data, size_t numBytes);
    static uint256_t  digest(const std::string & text);
public:
    xkeccak256_t();
    ~xkeccak256_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::keccak_256; }
    virtual int                     get_digest_length() const override { return 32; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint256_t & hash); //raw hash as 256bit
private:
    void* _context;
};
//512bit keccak(sha3 family)
class xkeccak512_t : public xhash_t
{
public:
    static uint512_t  digest(const void* data, size_t numBytes);
    static uint512_t  digest(const std::string & text);
public:
    xkeccak512_t();
    ~xkeccak512_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::keccak_512; }
    virtual int                     get_digest_length() const override { return 64; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    bool                            get_hash(uint512_t & hash); //raw hash as 512bit
private:
    void* _context;
};

//xxh32_t and xxh64_t that is the most fast hash algorithm
//note: xxh32_t and xxh64_t  get 32/64bit' digest instead of 128/256bits(e.g. sha256)
/* benchmark
 Comparison (single thread, Windows Seven 32 bits, using SMHasher on a Core 2 Duo @3GHz)
 Name            Speed       Q.Score   Author
 xxHash          5.4 GB/s     10
 CrapWow         3.2 GB/s      2       Andrew
 MumurHash 3a    2.7 GB/s     10       Austin Appleby
 SpookyHash      2.0 GB/s     10       Bob Jenkins
 SBox            1.4 GB/s      9       Bret Mulvey
 Lookup3         1.2 GB/s      9       Bob Jenkins
 SuperFastHash   1.2 GB/s      1       Paul Hsieh
 CityHash64      1.05 GB/s    10       Pike & Alakuijala
 FNV             0.55 GB/s     5       Fowler, Noll, Vo
 CRC32           0.43 GB/s     9
 MD5-32          0.33 GB/s    10       Ronald L. Rivest
 SHA1-32         0.28 GB/s    10

 Q.Score is a measure of quality of the hash function.
 It depends on successfully passing SMHasher test set.
 10 is a perfect score.

 A 64-bit version, named XXH64, is available since r35.
 It offers much better speed, but for 64-bit applications only.
 Name     Speed on 64 bits    Speed on 32 bits
 XXH64       13.8 GB/s            1.9 GB/s
 XXH32        6.8 GB/s            6.0 GB/s
 */
class xxh32_t : public xhash_t
{
public:
    static uint32_t  digest(const void* data, size_t numBytes);
    static uint32_t  digest(const std::string & text);
public:
    xxh32_t();
    ~xxh32_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::xxh_32; }
    virtual int                     get_digest_length() const override { return 4; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    uint32_t                        get_hash(); //raw hash as 32bit
private:
    void* xxhash_state;
};

class xxh64_t : public xhash_t
{
public:
    static uint64_t  digest(const void* data, size_t numBytes);
    static uint64_t  digest(const std::string & text);
public:
    xxh64_t();
    ~xxh64_t();
public:
    virtual xhash_type_t        get_type() const override { return xhash_type_t::xxh_64; }
    virtual int                     get_digest_length() const override { return 8; } //return how many bytes of digest
    bool                            reset() ;/// restart
    virtual int                     update(const std::string & text)  override;
    virtual int                     update(const void* data, size_t numBytes)  override;
    virtual bool                    get_hash(std::vector<uint8_t> & raw_bytes) override; //get raw hash data
    uint64_t                        get_hash(); //raw data as 64bit
private:
    void* xxhash_state;
};
NS_END2

