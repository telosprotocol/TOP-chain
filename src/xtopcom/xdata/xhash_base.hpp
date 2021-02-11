// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <assert.h>

#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xdatautil.h"
#include "xutility/xhash.h"

namespace top { namespace data {

// TODO(jimmy) move hash outside of serialized stream, hash is local
class xhash_base_t {
 public:
    //virtual int32_t write_hash_str(base::xstream_t & stream) const = 0;

    //const uint256_t & get_hash() const {return m_current_hash;}
    //std::string get_hash_hex_str() const {return to_hex_str(m_current_hash);}
    // std::string get_hash_str() const {
    //     return std::string(reinterpret_cast<char*>(m_current_hash.data()), m_current_hash.size());
    // }
    // std::string calc_and_get_hash_str() {
    //     calc_hash();
    //     return std::string(reinterpret_cast<char*>(m_current_hash.data()), m_current_hash.size());
    // }
    // uint32_t calc_stream_hash(const base::xstream_t & stream) {
    //     assert(stream.size() != 0);
    //     m_current_hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    //     return (uint32_t)stream.size();
    // }
    // uint32_t get_data_size() {
    //     base::xstream_t stream(base::xcontext_t::instance());
    //     write_hash_str(stream);
    //     return (uint32_t)stream.size();
    // }
    // uint32_t calc_hash() {
    //     base::xstream_t stream(base::xcontext_t::instance());
    //     write_hash_str(stream);
    //     assert(stream.size() != 0);
    //     m_current_hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    //     return (uint32_t)stream.size();
    // }
    // bool check_hash() {
    //     base::xstream_t stream(base::xcontext_t::instance());
    //     write_hash_str(stream);
    //     assert(stream.size() != 0);
    //     uint256_t hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    //     return hash == m_current_hash;
    // }

    static std::string calc_dataunit_hash(base::xdataunit_t* dataunit) {
        base::xstream_t stream(base::xcontext_t::instance());
        int32_t ret = dataunit->do_write(stream);
        xassert(ret > 0);
        if (ret <= 0) {
            return {};
        }
        uint256_t hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
        std::string hash_str = std::string(reinterpret_cast<char*>(hash.data()), hash.size());
        return hash_str;
    }

    static uint256_t calc_dataunit_hash_uint256(base::xdataunit_t* dataunit) {
        base::xstream_t stream(base::xcontext_t::instance());
        int32_t ret = dataunit->do_write(stream);
        xassert(ret > 0);
        if (ret <= 0) {
            return {};
        }
        uint256_t hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
        return hash;
    }

 public:
    //uint256_t m_current_hash;
};

}  // namespace data
}  // namespace top
