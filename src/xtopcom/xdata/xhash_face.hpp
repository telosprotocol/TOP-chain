// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xutility/xhash.h"

namespace top { namespace data {

template<class T>
class xhash_decorate :public T{
 public:
    using T::T;

    virtual int32_t do_write(base::xstream_t & stream) {
        const int32_t begin_size = stream.size();
        T::do_write(stream);
        stream << m_current_hash;
        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }
    virtual int32_t do_read(base::xstream_t & stream) {
        const int32_t begin_size = stream.size();
        T::do_read(stream);
        stream >> m_current_hash;
        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }

    uint256_t get_hash() const {return m_current_hash;}
    void calc_hash256() {
        base::xstream_t stream(base::xcontext_t::instance());
        T::do_write(stream);
        m_current_hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    }

 private:
    uint256_t m_current_hash;
};

}  // namespace data
}  // namespace top
