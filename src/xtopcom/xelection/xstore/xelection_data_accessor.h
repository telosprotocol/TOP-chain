// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xelection/xelection_data_accessor_face.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xstore/xstore_face.h"
#include "xbasic/xlru_cache.h"
NS_BEG3(top, election, store)
using elect_result_t = std::map<common::xslot_id_t, data::election::xelection_info_bundle_t>;
class xtop_election_data_accessor final : public xelection_data_accessor_face_t {

public:
    xtop_election_data_accessor(const observer_ptr<top::store::xstore_face_t>& store);

private:
#if 0
// todo charles
// need refactor
    data::election::xelection_result_store_t
        get_election_result_store(const std::string& elect_addr,
                                  const uint64_t block_height,
                                  std::error_code & ec);
#endif
    std::pair<common::xcluster_id_t, common::xgroup_id_t>
        get_cid_gid_from_xip(const common::xip2_t& xip);

    elect_result_t
        get_elect_result(const data::election::xelection_result_store_t& election_result_store,
                         const common::xip2_t& xip,
                         const common::xip2_t& cache_xip);

    std::string get_elec_blockchain_addr(std::string const& owner);

private:
	observer_ptr<top::store::xstore_face_t> 			    m_store;
	basic::xlru_cache<common::xip2_t, elect_result_t>		m_cache;
};
using xelection_data_accessor_t = xtop_election_data_accessor;

NS_END3
