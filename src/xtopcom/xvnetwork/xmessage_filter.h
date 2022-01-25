// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xelection/xcache/xdata_accessor_face.h"
#include "xvnetwork/xmessage_filter_base.h"
#include "xvnetwork/xvhost_face_fwd.h"

NS_BEG2(top, vnetwork)
//
//class xmsg_filter_message_empty final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_message_empty(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// filter when the receiver was not the broadcast_network_id && the receiver was not this vhost
//class xmsg_filter_wrong_dst final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_wrong_dst(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// filter when the receiver version empty && time not in range [-6, 2]
//class xmsg_filter_local_time final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_local_time(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// IF the receiver is consensus_validator &&
//// receiver and sender is neighbors
//// THEN filter when version not match || sender version empty.
//class xmsg_filter_validator_neighbors_version_mismatch final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_validator_neighbors_version_mismatch(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// IF the receiver is consensus_validator &&
//// receiver is not neighbor
//// THEN the sender must be auditor OR achieve
//// filter when sender is auditor
//class xmsg_filter_validator_from_auditor final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_validator_from_auditor(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// IF the receiver is consensus_validator &&
//// receiver is not neighbor
//// THEN the sender must be auditor OR achieve
//// filter when sender is achieve
//class xmsg_filter_validator_from_archive final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_validator_from_archive(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// IF the receiver is consensus_auditor &&
//// the sender is consensus_validator
//// THEN the sender must be from its associated validator.
//class xmsg_filter_auditor_from_validator final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_auditor_from_validator(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};
//
//// IF reveiver version is still empty after above filters, it shouldn't be a consensus node.
//// (IF it were, the code must be wrong.)
//// use it's group_element version to complete the version.
//class xmsg_filter_version_still_empty final : public xmessage_filter_base_t {
//public:
//    xmsg_filter_version_still_empty(top::vnetwork::xmessage_filter_manager_t * filter_mgr_ptr);
//    void filt(xvnetwork_message_t & vnetwork_message) override;
//
//private:
//    xmessage_filter_manager_t * m_filter_mgr_ptr;
//};

class xtop_message_filter_message_id final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_message_id(xtop_message_filter_message_id const &) = delete;
    xtop_message_filter_message_id(xtop_message_filter_message_id &&) = default;
    xtop_message_filter_message_id & operator=(xtop_message_filter_message_id const &) = delete;
    xtop_message_filter_message_id & operator=(xtop_message_filter_message_id &&) = delete;
    ~xtop_message_filter_message_id() override = default;

    explicit xtop_message_filter_message_id(observer_ptr<vnetwork::xvhost_face_t> const &,
                                            observer_ptr< election::cache::xdata_accessor_face_t> const &) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;
};

class xtop_message_filter_sender final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_sender(xtop_message_filter_sender const &) = delete;
    xtop_message_filter_sender(xtop_message_filter_sender &&) = default;
    xtop_message_filter_sender & operator=(xtop_message_filter_sender const &) = delete;
    xtop_message_filter_sender & operator=(xtop_message_filter_sender &&) = delete;
    ~xtop_message_filter_sender() override = default;

    explicit xtop_message_filter_sender(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                        observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;
};

class xtop_message_filter_recver final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_recver(xtop_message_filter_recver const &) = delete;
    xtop_message_filter_recver(xtop_message_filter_recver &&) = default;
    xtop_message_filter_recver & operator=(xtop_message_filter_recver const &) = delete;
    xtop_message_filter_recver & operator=(xtop_message_filter_recver &&) = delete;
    ~xtop_message_filter_recver() override = default;

    explicit xtop_message_filter_recver(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                        observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;
};

class xtop_message_filter_recver_is_validator final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_recver_is_validator(xtop_message_filter_recver_is_validator const &) = delete;
    xtop_message_filter_recver_is_validator(xtop_message_filter_recver_is_validator &&) = default;
    xtop_message_filter_recver_is_validator & operator=(xtop_message_filter_recver_is_validator const &) = delete;
    xtop_message_filter_recver_is_validator & operator=(xtop_message_filter_recver_is_validator &&) = delete;
    ~xtop_message_filter_recver_is_validator() override = default;

    explicit xtop_message_filter_recver_is_validator(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                     observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;

private:
    bool filter_sender_from_consensus_group(xvnetwork_message_t & xvnetwork_message, std::error_code & ec) const;
    bool filter_sender_from_nonconsensus_group(xvnetwork_message_t & xvnetwork_message, std::error_code & ec) const;

    /// @brief filter messages sent from edge directly.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_edge(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter messages sent from fullnode directly.
    /// @param vnetwork_message message to be verified.
    /// @param ec error code.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_fullnode(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from archive or exchange directly.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_storage(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from rec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from zec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_validator(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from auditor.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_auditor(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator from receiver's group.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_same_validator_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator from different group.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_different_validator_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator from different group but both sender and receiver have the same associated auditor group.
    /// @param vnetwork_message message to be verified.
    /// @param sender_associated_auditor message's sender associated auditor group element.
    /// @param recver_associated_auditor message's recver associated auditor group element.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_different_validator_with_same_associated_auditor_group(xvnetwork_message_t & vnetwork_message,
                                                                                   std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                                                   std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                                                   std::error_code & ec) const;

    /// @brief filter message sent from validator from different group but sender and receiver have different associated auditor group.
    /// @param vnetwork_message message to be verified.
    /// @param sender_associated_auditor message's sender associated auditor group element.
    /// @param recver_associated_auditor message's recver associated auditor group element.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_different_validator_without_same_associated_auditor_group(xvnetwork_message_t & vnetwork_message,
                                                                                      std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                                                      std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                                                      std::error_code & ec) const;

    /// @brief filter message sent from associated auditor.
    /// @param vnetwork_message message to be verified.
    /// @param sender_auditor auditor group element object message sender located in.
    /// @param recver_associated_auditor message's recver associated auditor group element.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_associated_auditor(xvnetwork_message_t & vnetwork_message,
                                               std::shared_ptr<election::cache::xgroup_element_t> const & sender_auditor,
                                               std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                               std::error_code & ec) const;

    /// @brief filter message sent from non-associated auditor.
    /// @param vnetwork_message message to be verified.
    /// @param sender_auditor auditor group element object message sender located in.
    /// @param recver_associated_auditor message's recver associated auditor group element.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_non_associated_auditor(xvnetwork_message_t & vnetwork_message,
                                                   std::shared_ptr<election::cache::xgroup_element_t> const & sender_auditor,
                                                   std::shared_ptr<election::cache::xgroup_element_t> const & recver_associated_auditor,
                                                   std::error_code & ec) const;
};

class xtop_message_filter_recver_is_auditor final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_recver_is_auditor(xtop_message_filter_recver_is_auditor const &) = delete;
    xtop_message_filter_recver_is_auditor(xtop_message_filter_recver_is_auditor &&) = default;
    xtop_message_filter_recver_is_auditor & operator=(xtop_message_filter_recver_is_auditor const &) = delete;
    xtop_message_filter_recver_is_auditor & operator=(xtop_message_filter_recver_is_auditor &&) = delete;
    ~xtop_message_filter_recver_is_auditor() override = default;

    explicit xtop_message_filter_recver_is_auditor(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                                   observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;

private:
    bool filter_sender_from_consensus_group(xvnetwork_message_t & xvnetwork_message, std::error_code & ec) const;
    bool filter_sender_from_nonconsensus_group(xvnetwork_message_t & xvnetwork_message, std::error_code & ec) const;

    /// @brief filter messages sent from edge directly.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_edge(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter messages sent from fullnode directly.
    /// @param vnetwork_message message to be verified.
    /// @param ec error code.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_fullnode(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from archive or exchange directly.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_storage(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from rec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from zec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_validator(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from auditor.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_auditor(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator from receiver's group.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_same_auditor_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from validator from different group.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_different_auditor_group(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from associated auditor.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_associated_validator(xvnetwork_message_t & vnetwork_message,
                                                 std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                 std::shared_ptr<election::cache::xgroup_element_t> const & recver_auditor,
                                                 std::error_code & ec) const;

    /// @brief filter message sent from non-associated auditor.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_non_associated_validator(xvnetwork_message_t & vnetwork_message,
                                                     std::shared_ptr<election::cache::xgroup_element_t> const & sender_associated_auditor,
                                                     std::shared_ptr<election::cache::xgroup_element_t> const & recver_auditor,
                                                     std::error_code & ec) const;
};

class xtop_message_filter_recver_is_rec final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_recver_is_rec(xtop_message_filter_recver_is_rec const &) = delete;
    xtop_message_filter_recver_is_rec(xtop_message_filter_recver_is_rec &&) = default;
    xtop_message_filter_recver_is_rec & operator=(xtop_message_filter_recver_is_rec const &) = delete;
    xtop_message_filter_recver_is_rec & operator=(xtop_message_filter_recver_is_rec &&) = delete;
    ~xtop_message_filter_recver_is_rec() override = default;

    explicit xtop_message_filter_recver_is_rec(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                               observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;

private:
    /// @brief filter message sent from rec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from other group except rec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_non_rec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;
};

class xtop_message_filter_recver_is_zec final : public xmessage_filter_base_t {
private:
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr< election::cache::xdata_accessor_face_t> m_election_data_accessor;

public:
    xtop_message_filter_recver_is_zec(xtop_message_filter_recver_is_zec const &) = delete;
    xtop_message_filter_recver_is_zec(xtop_message_filter_recver_is_zec &&) = default;
    xtop_message_filter_recver_is_zec & operator=(xtop_message_filter_recver_is_zec const &) = delete;
    xtop_message_filter_recver_is_zec & operator=(xtop_message_filter_recver_is_zec &&) = delete;
    ~xtop_message_filter_recver_is_zec() override = default;

    explicit xtop_message_filter_recver_is_zec(observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                                               observer_ptr< election::cache::xdata_accessor_face_t> const & election_data_accessor) noexcept;

    xfilter_result_t filter(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const override;

private:
    /// @brief filter message sent from zec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;

    /// @brief filter message sent from other group except zec.
    /// @param vnetwork_message message to be verified.
    /// @param ec log the verification error if the filter captures violence.
    /// @return true if vnetwork_message need further verification, otherwise returns false.
    bool filter_sender_from_non_zec(xvnetwork_message_t & vnetwork_message, std::error_code & ec) const;
};

NS_END2
