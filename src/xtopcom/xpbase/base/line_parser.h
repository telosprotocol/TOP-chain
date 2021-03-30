//
//  line_parser.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <stdint.h>
#include <string.h>
#include <iostream>

namespace top {
namespace base {

static const uint32_t kMaxSplitNum = 1024;
static const uint32_t kMaxLineLen = 1024 * 1024;

class LineParser {
public:
    explicit LineParser(const char* in, char del = '\t', uint32_t len = 0) : div_(del), buf_(NULL) {
        buf_ = new char[kMaxLineLen];
        Split(in, len);
    }

    ~LineParser() {
        if (buf_ != NULL) {
            delete []buf_;
        }
    }

    inline char* operator[](uint32_t i) const {
        if (cnt_ <= i) {
            return NULL;
        }

        return pt_[i];
    }

    inline uint32_t Count() const {
        return cnt_;
    }

    inline int32_t SubLen(uint32_t i) {
        if (cnt_ <= i) {
            return -1;
        }

        return str_len_[i];
    }

private:
    const LineParser& operator = (const LineParser& src);
    LineParser(const LineParser&);

    void Split(const char* in, uint32_t len) {
        cnt_ = 0;
        if (NULL == in) {
            return;
        }

        uint32_t in_len = len;
        if (in_len <= 0) {
            in_len = strlen(in);
        }

        if (0 == in_len) {
            return;
        }

        const char* ptr = in;
        char* p_buf = buf_;
        uint32_t tmp_div_cnt = 0;
        pt_[tmp_div_cnt] = p_buf;
        uint32_t str_len = 0;
        for (uint32_t i = 0; i < in_len; ++i) {
            *p_buf = *ptr;
            ++str_len;
            if (div_ == *ptr) {
                if (kMaxSplitNum > tmp_div_cnt) {
                    str_len_[tmp_div_cnt] = str_len - 1;
                    str_len = 0;
                    ++tmp_div_cnt;
                    if (tmp_div_cnt >= kMaxSplitNum) {
                        break;
                    }

                    pt_[tmp_div_cnt] = p_buf + 1;
                    *p_buf = '\0';
                } else {
                    break;
                }
            }
            ++p_buf;
            ++ptr;
        }
        *p_buf = '\0';
        str_len_[tmp_div_cnt] = str_len;
        cnt_ = tmp_div_cnt + 1;
    }

    uint32_t cnt_;
    char div_;
    char* pt_[kMaxSplitNum];
    uint32_t str_len_[kMaxSplitNum];
    char* buf_;
};

}  // namespace base
}  // namespace top
