// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * @file xmessage_filter_manager_face.h
 * @brief interface definitions for xmessage_filter_manager class
 */

#pragma once
#include "xbasic/xmemory.hpp"
#include "xbasic/xns_macro.h"
#include "xbasic/xrunnable.h"
#include "xvnetwork/xmessage_filter_base.h"
#include "xvnetwork/xvhost_face_fwd.h"

#include <list>
#include <memory>

NS_BEG2(top, vnetwork)

class xtop_message_filter_manager_face : public xbasic_runnable_t<xtop_message_filter_manager_face> {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_message_filter_manager_face);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_message_filter_manager_face);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xtop_message_filter_manager_face);

    /**
     * @brief Get the vhost ptr object
     * 
     * @return observer_ptr<xvhost_face_t> 
     */
    virtual observer_ptr<xvhost_face_t> get_vhost_ptr() const = 0;

    /**
     * @brief Get the election data accessor ptr object
     * 
     * @return observer_ptr<election::cache::xdata_accessor_face_t> 
     */
    virtual observer_ptr<election::cache::xdata_accessor_face_t> get_election_data_accessor_ptr() const = 0;

    /**
     * @brief filt function implemented by derived class
     * 
     * @param msg The message to be filted
     */
    virtual void filt_message(xvnetwork_message_t & msg) const = 0;
};
using xmessage_filter_manager_face_t = xtop_message_filter_manager_face;

NS_END2