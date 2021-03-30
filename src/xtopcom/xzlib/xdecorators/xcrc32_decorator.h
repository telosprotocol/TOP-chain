#pragma once

#include "xbasic/xbyte_buffer.h"

NS_BEG3(top, zlib, decorators)

struct xtop_crc32_decorator final
{
    xtop_crc32_decorator()                                         = delete;
    xtop_crc32_decorator(xtop_crc32_decorator const &)             = delete;
    xtop_crc32_decorator & operator=(xtop_crc32_decorator const &) = delete;
    xtop_crc32_decorator(xtop_crc32_decorator &&)                  = delete;
    xtop_crc32_decorator & operator=(xtop_crc32_decorator &&)      = delete;
    ~xtop_crc32_decorator()                                        = delete;

    static
    xbyte_buffer_t
    encode(xbyte_buffer_t const & in);

    static
    xbyte_buffer_t
    decode(xbyte_buffer_t const & in);
};

using xcrc32_decorator_t = xtop_crc32_decorator;

NS_END3
