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

DEFINE_SHARED_PTR(xevent_store);

class xevent_store_block_to_db_t : public xevent_store_t {
 public:
    xevent_store_block_to_db_t(
            const data::xblock_ptr_t & _block,
            const std::string& _owner,
            bool _new_block,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_store_t(type_block_to_db, _owner, dir, _sync),
    block(_block),
    new_block(_new_block) {
    }
    data::xblock_ptr_t block;
    bool new_block;
};

DEFINE_SHARED_PTR(xevent_store_block_to_db);

class xevent_store_accountblock_queue_ready_t : public xevent_store_t {
public:
    xevent_store_accountblock_queue_ready_t(
            const std::string& _owner,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_store_t(type_accountblock_queue_ready, _owner, dir, _sync) {
    }
};

DEFINE_SHARED_PTR(xevent_store_accountblock_queue_ready);

class xevent_store_version_update_t : public xevent_store_t {
public:
    xevent_store_version_update_t(
            const std::string& _owner,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_store_t(type_config_update, _owner, dir, _sync) {
    }
};

DEFINE_SHARED_PTR(xevent_store_version_update);

// </editor-fold>

NS_END2
