// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstatistic/xstatistic.h"

NS_BEG2(top, xstatistic)

#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
enum_statistic_class_type message_id_to_class_type(uint32_t msgid);
#else
enum_statistic_class_type message_id_to_class_type(uint32_t msgid);
#endif

NS_END2
