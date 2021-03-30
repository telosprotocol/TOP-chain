// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xtxpool_face.h"

#include <string>

NS_BEG2(top, xtxpool)

class xtxpool_resources : public xtxpool_resources_face {
public:
    xtxpool_resources(const observer_ptr<store::xstore_face_t> & store,
                      const xobject_ptr_t<base::xvblockstore_t> & blockstore,
                      const observer_ptr<mbus::xmessage_bus_face_t> &bus,
                      const xobject_ptr_t<base::xvcertauth_t> & certauth,
                      const observer_ptr<time::xchain_time_face_t> & clock,
                      enum_xtxpool_order_strategy strategy);
    virtual ~xtxpool_resources();

public:
    virtual store::xstore_face_t * get_store() const override;
    virtual base::xvblockstore_t * get_vblockstore() const override;
    virtual mbus::xmessage_bus_face_t * get_mbus() const override;
    virtual base::xvcertauth_t * get_certauth() const override;
    virtual time::xchain_time_face_t * get_chain_timer() const override;
    virtual enum_xtxpool_order_strategy get_order_strategy() const override;
    virtual uint64_t get_receipt_valid_window() const override;
    virtual void set_receipt_valid_window_day(uint64_t day) override;

private:
    observer_ptr<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    xobject_ptr_t<base::xvcertauth_t> m_certauth;
    observer_ptr<time::xchain_time_face_t> m_clock;
    enum_xtxpool_order_strategy m_strategy;
    uint64_t m_receipt_valid_window;
};

NS_END2
