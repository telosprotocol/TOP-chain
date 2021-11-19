// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xstring.h"

#include <cassert>

NS_BEG1(top)

template <>
std::string to_string<int>(int const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<long>(long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<long long>(long long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<unsigned int>(unsigned int const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<unsigned long>(unsigned long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<unsigned long long>(unsigned long long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<std::string>(std::string const & input) {
    return input;
}

template <>
std::string to_string<xbytes_t>(xbytes_t const & input) {
    return {input.begin(), input.end()};
}

template <>
short int from_string<short int>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        do {
            auto i = std::stoi(input);
            if (i > static_cast<int>(std::numeric_limits<short int>::max()) || i < static_cast<int>(std::numeric_limits<short int>::min())) {
                ec = std::make_error_code(std::errc::result_out_of_range);
                break;
            }

            return static_cast<short int>(i);
        } while (false);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const &) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
int from_string<int>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        return std::stoi(input);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const &) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
long from_string<long>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        return std::stol(input);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const &) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
long long from_string<long long>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        return std::stoll(input);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const & eh) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
unsigned short int from_string<unsigned short int>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        do {
            auto i = std::stoul(input);
            if (i > static_cast<unsigned long>(std::numeric_limits<unsigned short int>::max()) || i < static_cast<unsigned long>(std::numeric_limits<unsigned short int>::min())) {
                ec = std::make_error_code(std::errc::result_out_of_range);
                break;
            }

            return static_cast<unsigned short int>(i);
        } while (false);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const &) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
unsigned int from_string<unsigned int>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        do {
            auto i = std::stoul(input);
            if (i > static_cast<unsigned long>(std::numeric_limits<unsigned int>::max()) || i < static_cast<unsigned long>(std::numeric_limits<unsigned int>::min())) {
                ec = std::make_error_code(std::errc::result_out_of_range);
                break;
            }

            return static_cast<unsigned int>(i);
        } while (false);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const &) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
unsigned long from_string<unsigned long>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        return std::stoul(input);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const & eh) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }

    return {};
}

template <>
unsigned long long from_string<unsigned long long>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    try {
        return std::stoull(input);
    } catch (std::invalid_argument const &) {
        ec = std::make_error_code(std::errc::invalid_argument);
    } catch (std::out_of_range const & eh) {
        ec = std::make_error_code(std::errc::result_out_of_range);
    }
    return {};
}

template <>
xbytes_t from_string<xbytes_t>(std::string const & input, std::error_code & /*ec*/) {
    return {input.begin(), input.end()};
}

NS_END1
