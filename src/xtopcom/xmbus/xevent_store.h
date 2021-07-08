// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <string>
#include "xmbus/xevent.h"
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"

NS_BEG2(top, mbus)

// <editor-fold defaultstate="collapsed" desc="event type store">
// published by store, common to ask missed things

class xevent_store_t : public xevent_t {
public:

    enum _minor_type_ {
        type_block_to_db,   // block store in db
        type_accountblock_queue_ready,
        type_config_update,
        type_block_committed,
    };

    xevent_store_t(_minor_type_ sub_type,
            const std::string& _owner,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_t(xevent_major_type_store,
    (int) sub_type,
    dir,
    _sync),
    owner(_owner) {
    }

    std::string owner;
};

using xevent_store_ptr_t = xobject_ptr_t<xevent_store_t>;

class xevent_store_block_to_db_t : public xevent_store_t {
 public:
    xevent_store_block_to_db_t(const std::string & _owner, 
        base::xvbindex_t* index_ptr,
        bool _new_block,
        direction_type dir = to_listener,
        bool _sync = true) :
        xevent_store_t(type_block_to_db, _owner, dir, _sync),
        blk_hash(index_ptr->get_block_hash()), blk_height(index_ptr->get_height()),  new_block(_new_block),
        blk_viewid(index_ptr->get_viewid()), blk_type(index_ptr->get_block_type()),
        blk_level(index_ptr->get_block_level()), blk_class{index_ptr->get_block_class()} {
    }
    xevent_store_block_to_db_t(
            const data::xblock_ptr_t & _block,
            const std::string& _owner,
            bool _new_block,
            direction_type dir = to_listener,
            bool _sync = true) :
        xevent_store_t(type_block_to_db, _owner, dir, _sync),
        blk_hash{_block->get_block_hash() }, blk_height{_block->get_height()}, new_block(_new_block),
        blk_viewid{_block->get_viewid()}, blk_type{_block->get_block_type()},
        blk_level{_block->get_block_level()}, blk_class{_block->get_block_class()} {
    }
    // data::xblock_ptr_t block;
    std::string blk_hash;
    uint64_t blk_height;
    bool new_block;
    uint64_t blk_viewid;
    top::base::enum_xvblock_type blk_type;
    top::base::enum_xvblock_level blk_level;
    base::enum_xvblock_class blk_class;
};

using xevent_store_block_to_db_ptr_t = xobject_ptr_t<xevent_store_block_to_db_t>;

data::xblock_ptr_t extract_block_from(xevent_store_block_to_db_ptr_t const & ev, const int etag = 0) noexcept;

class xevent_store_accountblock_queue_ready_t : public xevent_store_t {
public:
    xevent_store_accountblock_queue_ready_t(
            const std::string& _owner,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_store_t(type_accountblock_queue_ready, _owner, dir, _sync) {
    }
};

using xevent_store_accountblock_queue_ready_ptr_t = xobject_ptr_t<xevent_store_accountblock_queue_ready_t>;

class xevent_store_version_update_t : public xevent_store_t {
public:
    xevent_store_version_update_t(
            const std::string& _owner,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_store_t(type_config_update, _owner, dir, _sync) {
    }
};

using xevent_store_version_update_ptr_t = xobject_ptr_t<xevent_store_version_update_t>;

class xevent_store_block_committed_t : public xevent_store_t {
 public:
    xevent_store_block_committed_t(const std::string & _owner, 
        base::xvbindex_t* index_ptr,
        bool _new_block,
        direction_type dir = to_listener,
        bool _sync = true) :
        xevent_store_t(type_block_committed, _owner, dir, _sync),
        blk_hash(index_ptr->get_block_hash()), blk_height(index_ptr->get_height()),  new_block(_new_block),
        blk_viewid(index_ptr->get_viewid()), blk_type(index_ptr->get_block_type()),
        blk_level(index_ptr->get_block_level()), blk_class{index_ptr->get_block_class()} {
    }
    std::string blk_hash;
    uint64_t blk_height;
    bool new_block;
    uint64_t blk_viewid;
    top::base::enum_xvblock_type blk_type;
    top::base::enum_xvblock_level blk_level;
    base::enum_xvblock_class blk_class;
};

using xevent_store_block_committed_ptr_t = xobject_ptr_t<xevent_store_block_committed_t>;

data::xblock_ptr_t extract_block_from(xevent_store_block_committed_ptr_t const & ev, const int etag = 0) noexcept;

// </editor-fold>

NS_END2
