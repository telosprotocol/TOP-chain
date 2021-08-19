// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbase/xbase.h"

#include <inttypes.h>

#include <deque>

NS_BEG2(top, data)

class xprocessed_height_record_t {
public:
    void update_min_height(uint64_t height) {
        uint64_t new_min_height = (height & 0xFFFFFFFFFFFFFFC0);
        if (new_min_height > m_min_height) {
            uint32_t delete_record_num = ((new_min_height - m_min_height) >> 6);
            if (m_bit_record.size() > delete_record_num) {
                m_bit_record.erase(m_bit_record.begin(), m_bit_record.begin() + delete_record_num);
            } else {
                m_bit_record.clear();
            }

            m_min_height = new_min_height;
        }
        if (m_min_record_height < height) {
            m_min_record_height = height;
        }
        if (m_max_record_height < height) {
            m_max_record_height = height;
        }

        if (m_max_height < new_min_height) {
            m_max_height = new_min_height;
        }
        print();
    }
    void record_height(uint64_t height) {
        uint64_t new_min_height = (height & 0xFFFFFFFFFFFFFFC0);
        if (new_min_height < m_min_height) {
            uint32_t add_record_num = (m_min_height - new_min_height) >> 6;
            for (uint32_t i = 0; i < add_record_num; i++) {
                m_bit_record.push_front(0);
            }
            m_min_height = new_min_height;
        }

        uint64_t new_max_height = (height & 0xFFFFFFFFFFFFFFC0) + 64;
        if (new_max_height > m_max_height) {
            uint32_t add_record_num = (new_max_height - m_max_height) >> 6;
            for (uint32_t i = 0; i < add_record_num; i++) {
                m_bit_record.push_back(0);
            }
            m_max_height = new_max_height;
        }

        uint32_t recordidx = ((new_min_height - m_min_height) >> 6);
        uint64_t add_bit = (1UL << (height - new_min_height));
        xdbg("before add height:%llu,new_min_height:%llu,m_bit_record[%u]:0x%" PRIx64 " ,add num:0x%" PRIx64 " ",
             height,
             new_min_height,
             recordidx,
             m_bit_record[recordidx],
             add_bit);
        m_bit_record[recordidx] |= add_bit;

        if (height > m_max_record_height) {
            m_max_record_height = height;
        }

        if (height < m_min_record_height) {
            m_min_record_height = height;
        }
        xdbg("add height:%llu,new_min_height:%llu,m_bit_record[%u]:0x%" PRIx64 " ", height, new_min_height, recordidx, m_bit_record[recordidx]);
        print();
    }
    bool is_record_height(uint64_t height) const {
        if (height > m_max_record_height || height < m_min_record_height) {
            return false;
        }

        uint64_t new_min_height = (height & 0xFFFFFFFFFFFFFFC0);
        uint32_t recordidx = ((new_min_height - m_min_height) >> 6);

        uint64_t bit_num = (1UL << (height - new_min_height));
        xdbg("m_bit_record size:%u,height:%llu,m_bit_record[%u]=0x%" PRIx64 " ,judge bit:0x%" PRIx64 " ", m_bit_record.size(), height, recordidx, m_bit_record[recordidx], bit_num);
        return m_bit_record[recordidx] & bit_num;
    }

    uint64_t min_record_height() const {
        return m_min_record_height;
    }

    uint64_t max_record_height() const {
        return m_max_record_height;
    }

    bool get_latest_lacking_saction(uint64_t & left_end, uint64_t & right_end) const {
        if (m_max_record_height == 0 || m_max_record_height == m_min_record_height) {
            return false;
        }
        uint64_t min_check_height = (m_min_record_height > 1) ? m_min_record_height : 1;
        bool right_end_found = false;

        // todo:先确定一个u64的记录中有没有0，有0的取反，算出来最高位和最低位，这样速度会快很多。
        for (uint64_t height = m_max_record_height - 1; height >= min_check_height; height--) {
            uint64_t new_min_height = (height & 0xFFFFFFFFFFFFFFC0);
            uint32_t recordidx = ((new_min_height - m_min_height) >> 6);
            if (!right_end_found) {
                if (!(m_bit_record[recordidx] & (1UL << (height - new_min_height)))) {
                    right_end = height;
                    right_end_found = true;
                }
            } else {
                if (m_bit_record[recordidx] & (1UL << (height - new_min_height))) {
                    left_end = height + 1;
                    return true;
                }
            }
        }

        if (right_end_found) {
            left_end = min_check_height;
            return true;
        }
        return false;
    }

    void print() const {
        xdbg("m_min_height:%llu,m_max_height:%llu,m_min_record_height:%llu,m_max_record_height:%llu,m_bit_record size:%u",
             m_min_height,
             m_max_height,
             m_min_record_height,
             m_max_record_height,
             m_bit_record.size());
        // for (auto & record : m_bit_record) {
        //     xdbg("m_bit_record:0x%" PRIx64 " ", record);
        // }
    }

private:
    std::deque<uint64_t> m_bit_record;
    uint64_t m_min_height{0};         // include
    uint64_t m_max_height{0};         // not include
    uint64_t m_min_record_height{0};  // may not include
    uint64_t m_max_record_height{0};  // may not include
};

NS_END2
