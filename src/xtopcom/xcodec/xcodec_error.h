// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcodec/xcodec_errc.h"

#include <stdexcept>

NS_BEG2(top, codec)

class xtop_codec_error final : public std::runtime_error
{
private:
    std::error_code m_ec;

public:
    xtop_codec_error(xcodec_errc_t const errc, std::size_t const line, char const * file);
    xtop_codec_error(std::string const & msg, xcodec_errc_t const errc, std::size_t const line, char const * file);

    std::error_code const &
    code() const noexcept;

    char const *
    what() const noexcept override;

private:
    xtop_codec_error(std::string const & msg, std::error_code const & ec, std::size_t const line, std::string file);
};

using xcodec_error_t = xtop_codec_error;

NS_END2
