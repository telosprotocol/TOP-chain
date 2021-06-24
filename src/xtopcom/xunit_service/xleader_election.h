// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_utl.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvblock.h"
#include "xelection/xcache/xdata_accessor_face.h"

NS_BEG2(top, xunit_service)

class xelection_cache_imp : public xelection_cache_face {
public:

    // load manager tables
    virtual int32_t get_tables(const xvip2_t & xip, std::vector<table_index> * tables);

    // load election data from db
    virtual int32_t get_election(const xvip2_t & xip, elect_set * elect_data, bool bself = true);
    // load group election data
    virtual int32_t get_group_election(const xvip2_t & xip, int32_t groupid, elect_set * elect_data);
    // load parent election data
    virtual int32_t get_parent_election(const xvip2_t & xip, elect_set * elect_data);
    // add elect data
    virtual bool add(const xvip2_t & xip, const elect_set & elect_data, const std::vector<uint16_t> & tables, const elect_set & parent_data = {}, std::map<int32_t, elect_set> children = {}) ;

    // erase cached elect data
    virtual bool erase(const xvip2_t & xip);

protected:
    xvip2_t get_group_xip2(xvip2_t const& xip);

protected:
    struct xelect_item
    {
        std::vector<uint16_t> tables;
        elect_set self;
        elect_set parent;
        std::map<int32_t, elect_set> children;

        xelect_item& operator=(const xelect_item & data) {
            if (this == &data) {
                return *this;
            } else {
                tables.insert(tables.begin(), data.tables.begin(), data.tables.end());
                self.insert(self.begin(), data.self.begin(), data.self.end());
                parent.insert(parent.begin(), data.parent.begin(), data.parent.end());
                children = data.children;
                return *this;
            }
        }
    };

    std::map<xvip2_t, xelect_item, xvip2_compare> m_elect_data;
    std::mutex m_mutex;
};

class xelection_wrapper {
 public:
    static bool on_network_start(xelection_cache_face *                                       p_election,
                                         const xvip2_t &                                              xip,
                                         const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &   network,
                                         const election::cache::xdata_accessor_face_t * elect_cache_ptr,
                                         uint64_t chain_time);

    static bool on_network_destory(xelection_cache_face * p_election, const xvip2_t & xip);
};

class xrandom_leader_election : public xleader_election_face {
 public:
    explicit xrandom_leader_election(const xobject_ptr_t<base::xvblockstore_t>& block_store, const std::shared_ptr<xelection_cache_face> & face);
 public:
    // judge is leader from view id for the address
    virtual const xvip2_t get_leader_xip(uint64_t viewId, const std::string &account, base::xvblock_t* prev_block, const xvip2_t & local, const xvip2_t & candidate, const common::xversion_t& version, uint16_t rotate_mode = enum_rotate_mode_rotate_by_last_block);
    // get election face which manager elect datas
    virtual xelection_cache_face * get_election_cache_face();
 private:
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    std::shared_ptr<xelection_cache_face> m_elector;
};

class xrotate_leader_election : public xleader_election_face {
    public:
    explicit xrotate_leader_election(const observer_ptr<base::xvblockstore_t>& block_store, const std::shared_ptr<xelection_cache_face> & face);
 public:
    static bool is_rotate_xip(const xvip2_t & local);

 public:
    // judge is leader from view id for the address
    virtual const xvip2_t get_leader_xip(uint64_t viewId, const std::string &account, base::xvblock_t* prev_block, const xvip2_t & local, const xvip2_t & candidate, const common::xversion_t& version, uint16_t rotate_mode = enum_rotate_mode_rotate_by_last_block);
    // get election face which manager elect datas
    virtual xelection_cache_face * get_election_cache_face();
 private:
     observer_ptr<base::xvblockstore_t> m_blockstore;
    std::shared_ptr<xelection_cache_face> m_elector;
};
NS_END2
