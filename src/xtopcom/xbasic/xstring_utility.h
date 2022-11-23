// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xstring_view.h"

#include <string>
#include <vector>

NS_BEG1(top)

std::vector<xstring_view_t> split(xstring_view_t input, char delimiter);
std::vector<xstring_view_t> split(std::string const & input, char delimiter);

NS_END1
