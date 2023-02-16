// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xcodec_error.h"

#include <string>

NS_BEG2(top, codec)

xtop_codec_error::xtop_codec_error(xcodec_errc_t const errc, std::size_t const line, char const * file)
    : xtop_codec_error{ std::string{}, make_error_code(errc), line, file }
{
}

xtop_codec_error::xtop_codec_error(std::string const & msg,
                                   xcodec_errc_t const errc,
                                   std::size_t const line,
                                   char const * file)
    : xtop_codec_error{ std::move(msg), make_error_code(errc), line, file }
{
}

xtop_codec_error::xtop_codec_error(std::string const & msg,
                                   std::error_code const & ec,
                                   std::size_t const line,
                                   std::string file)
    : std::runtime_error{ std::move(file) + ":" + std::to_string(line) + ":" + ec.message() + (msg.empty() ? msg : (". extra info: " + msg)) }
    , m_ec{ std::move(ec) }
{
}

std::error_code const &
xtop_codec_error::code() const noexcept {
    return m_ec;
}

char const *
xtop_codec_error::what() const noexcept {
    return std::runtime_error::what();
}

NS_END2
