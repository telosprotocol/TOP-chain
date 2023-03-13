// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xtop_log.h"

#include <string>
#include <vector>

NS_BEG2(top, common)

enum class enum_event_data_type {
    invalid = 0,
    string = 1,
    uint64 = 2,
    address = 3,
};

class argument_t {
public:
    argument_t() = default;
    argument_t(argument_t const &) = default;
    argument_t & operator=(argument_t const &) = default;
    argument_t(argument_t &&) = default;
    argument_t & operator=(argument_t &&) = default;
    ~argument_t() = default;

    argument_t(std::string const & str_data, enum_event_data_type type, bool indexed) : str_data(str_data), type(type), indexed(indexed){};
    argument_t(uint64_t const & uint64_t_data, enum_event_data_type type, bool indexed) : uint64_t_data(uint64_t_data), type(type), indexed(indexed){};

    std::string str_data;
    uint64_t uint64_t_data;
    enum_event_data_type type;
    bool indexed;
};

using arguments_t = std::vector<argument_t>;

class xtop_event_t {
public:
    xtop_event_t() = default;
    xtop_event_t(xtop_event_t const &) = default;
    xtop_event_t & operator=(xtop_event_t const &) = default;
    xtop_event_t(xtop_event_t &&) = default;
    xtop_event_t & operator=(xtop_event_t &&) = default;
    ~xtop_event_t() = default;

    std::string get_name() {
        return name;
    };

    xtop_event_t(std::string const & name, xnode_id_t const & address, arguments_t const & inputs) : name(name), address(address), inputs(inputs){};
    xtop_log_t pack();

private:
    std::string name;
    xnode_id_t address;
    arguments_t inputs;
};

NS_END2
