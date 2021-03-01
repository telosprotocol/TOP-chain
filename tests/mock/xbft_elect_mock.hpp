// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>
#include <map>
#include "xunit_service/xcons_face.h"

namespace top {
namespace mock {
class xelection_mock : public xunit_service::xelection_cache_face {
public:
    explicit xelection_mock(const elect_set & elec_data) {
        m_elec_data.insert(m_elec_data.begin(), elec_data.begin(), elec_data.end());
    }
public:
    virtual int32_t get_election(const xvip2_t &xip, elect_set *elect_data) {
        elect_data->insert(elect_data->begin(), m_elec_data.begin(), m_elec_data.end());
        return m_elec_data.size();
    }

    // load manager tables
    virtual int32_t get_tables(const xvip2_t & xip, std::vector<uint16_t> * tables) {
        tables->push_back(1);
        return 0;
    }

    // add elect data
    virtual bool add(const xvip2_t & xip, const elect_set & elect_data, const std::vector<uint16_t> & tables) {
        return false;
    }

    // erase cached elect data
    virtual bool erase(const xvip2_t & xip) {
        return false;
    }


private:
    elect_set m_elec_data;
};
}  // namespace mock
}  // namespace top
