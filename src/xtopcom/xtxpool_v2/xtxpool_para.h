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
#include "xtxpool_v2/xtxpool_resources_face.h"
#include "xmbus/xmessage_bus.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

class xtxpool_resources : public xtxpool_resources_face {
public:
    xtxpool_resources(const observer_ptr<store::xstore_face_t> & store,
                      const observer_ptr<base::xvblockstore_t> & blockstore,
                      const observer_ptr<base::xvcertauth_t> & certauth,
                      const observer_ptr<mbus::xmessage_bus_face_t> & bus);
    virtual ~xtxpool_resources();

public:
    virtual store::xstore_face_t * get_store() const override;
    virtual base::xvblockstore_t * get_vblockstore() const override;
    virtual base::xvcertauth_t * get_certauth() const override;
    virtual mbus::xmessage_bus_face_t * get_bus() const override;
    virtual xreceiptid_state_cache_t & get_receiptid_state_cache() override;
    virtual void update_send_ids_after_add_rsp_id(const base::xreceiptid_state_ptr_t & receiptid_state, const std::set<base::xtable_shortid_t> & all_table_sids) override;
    virtual bool get_send_id_after_add_rsp_id(base::xtable_shortid_t self_sid, base::xtable_shortid_t peer_sid, uint64_t & send_id) const override;

private:
    observer_ptr<store::xstore_face_t> m_store;
    observer_ptr<base::xvblockstore_t> m_blockstore;
    observer_ptr<base::xvcertauth_t> m_certauth;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    xreceiptid_state_cache_t m_receiptid_state_cache;
    std::map<base::xtable_shortid_t, std::map<base::xtable_shortid_t, uint64_t>> m_send_ids_after_add_rsp_id;
    mutable std::mutex m_mutex;
};

NS_END2
