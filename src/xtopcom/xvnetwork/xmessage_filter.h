// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvnetwork/xmessage_filter_base.h"
#include "xvnetwork/xmessage_filter_manager.h"

NS_BEG2(top, vnetwork)

class xmsg_filter_message_empty final : public xmessage_filter_base_t {
public:
    xmsg_filter_message_empty(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// filter when the receiver was not the broadcast_network_id && the receiver was not this vhost
class xmsg_filter_wrong_dst final : public xmessage_filter_base_t {
public:
    xmsg_filter_wrong_dst(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// filter when the receiver version empty && time not in range [-6, 2]
class xmsg_filter_local_time final : public xmessage_filter_base_t {
public:
    xmsg_filter_local_time(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// IF the receiver is consensus_validator &&
// receiver and sender is neighbors
// THEN filter when version not match || sender version empty.
class xmsg_filter_validator_neighbors_version_mismatch final : public xmessage_filter_base_t {
public:
    xmsg_filter_validator_neighbors_version_mismatch(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// IF the receiver is consensus_validator &&
// receiver is not neighbor
// THEN the sender must be auditor OR achieve
// filter when sender is auditor
class xmsg_filter_validator_from_auditor final : public xmessage_filter_base_t {
public:
    xmsg_filter_validator_from_auditor(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// IF the receiver is consensus_validator &&
// receiver is not neighbor
// THEN the sender must be auditor OR achieve
// filter when sender is achieve
class xmsg_filter_validator_from_archive final : public xmessage_filter_base_t {
public:
    xmsg_filter_validator_from_archive(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// IF the receiver is consensus_auditor &&
// the sender is consensus_validator
// THEN the sender must be from its associated validator.
class xmsg_filter_auditor_from_validator final : public xmessage_filter_base_t {
public:
    xmsg_filter_auditor_from_validator(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

// IF reveiver version is still empty after above filters, it shouldn't be a consensus node.
// (IF it were, the code must be wrong.)
// use it's group_element version to complete the version.
class xmsg_filter_version_still_empty final : public xmessage_filter_base_t {
public:
    xmsg_filter_version_still_empty(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
    void filt(xvnetwork_message_t & vnetwork_message) override;

private:
    xmessage_filter_manager_t * m_filter_mgr_ptr;
};

NS_END2