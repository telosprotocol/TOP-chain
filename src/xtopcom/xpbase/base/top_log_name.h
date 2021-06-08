//
//  top_log.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once
#if 0
#include "top_log.h"

#define TOP_DEBUG_NAME(fmt, ...) TOP_DEBUG("%s " fmt, name_.c_str(), ## __VA_ARGS__)
#define TOP_INFO_NAME(fmt, ...) TOP_INFO("%s " fmt, name_.c_str(), ## __VA_ARGS__)
#define TOP_KINFO_NAME(fmt, ...) TOP_KINFO("%s " fmt, name_.c_str(), ## __VA_ARGS__)
#define TOP_WARN_NAME(fmt, ...) TOP_WARN("%s " fmt, name_.c_str(), ## __VA_ARGS__)
#define TOP_ERROR_NAME(fmt, ...) TOP_ERROR("%s " fmt, name_.c_str(), ## __VA_ARGS__)
#define TOP_FATAL_NAME(fmt, ...) TOP_FATAL("%s " fmt, name_.c_str(), ## __VA_ARGS__)
#endif