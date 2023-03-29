// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xunit_service/xbatch_packer.h"

NS_BEG2(top, xunit_service)

class xpreproposal_msg_t {
public:
    xpreproposal_msg_t() {
    }
    xpreproposal_msg_t(const data::xblock_consensus_para_t & cs_para,
                       const std::vector<data::xcons_transaction_ptr_t> & txs,
                       const std::vector<base::xvproperty_prove_ptr_t> & receiptid_state_proves);
    int32_t serialize_to_string(std::string & _str) const;
    int32_t serialize_from_string(const std::string & _str, const std::string & table_addr, xtxpool_v2::xtxpool_face_t * txpool);

public:
    const std::string & get_last_block_hash() const {
        return m_last_block_hash;
    }
    const std::string & get_justify_cert_hash() const {
        return m_justify_cert_hash;
    }
    uint64_t get_gmtime() const {
        return m_gmtime;
    }
    uint64_t get_drand_height() const {
        return m_drand_height;
    }
    uint64_t get_total_lock_tgas_token_height() const {
        return m_total_lock_tgas_token_height;
    }
    const std::vector<data::xcons_transaction_ptr_t> & get_input_txs() const {
        return m_input_txs;
    }
    const std::vector<base::xvproperty_prove_ptr_t> & get_receiptid_state_proves() const {
        return m_receiptid_state_proves;
    }

    const xvip2_t & get_auditor_xip() const {
        return m_auditor_xip;
    }

    const xvip2_t & get_validator_xip() const {
        return m_validator_xip;
    }

private:
    int32_t do_write(base::xstream_t & stream) const;
    int32_t do_read(base::xstream_t & stream, const std::string & table_addr, xtxpool_v2::xtxpool_face_t * txpool);

private:
    std::string m_last_block_hash;
    std::string m_justify_cert_hash;
    uint64_t m_gmtime{0};
    uint64_t m_drand_height{0};
    uint64_t m_total_lock_tgas_token_height{0};
    xvip2_t m_auditor_xip;
    xvip2_t m_validator_xip;
    std::vector<data::xcons_transaction_ptr_t> m_input_txs;
    std::vector<base::xvproperty_prove_ptr_t> m_receiptid_state_proves;
    uint8_t m_version{0};
};

enum enum_received_msg_state_type {
    enum_received_msg_nil = 0,
    enum_received_msg_preproposal_first,
    enum_received_msg_proposal_first,
};

class xpreproposal_packer : public xbatch_packer {
public:
    explicit xpreproposal_packer(base::xtable_index_t & tableid,
                                 const std::string & account_id,
                                 std::shared_ptr<xcons_service_para_face> const & para,
                                 std::shared_ptr<xblock_maker_face> const & block_maker,
                                 base::xcontext_t & _context,
                                 uint32_t target_thread_id);
    virtual ~xpreproposal_packer();

public:
    virtual bool close(bool force_async = true) override;  // must call close before release object,otherwise object never be cleanup
    virtual bool proc_preproposal(const xvip2_t & leader_xip, uint64_t height, uint64_t viewid, uint64_t clock, uint32_t viewtoken, const std::string & msgdata) override;
    virtual int veriry_proposal_by_preproposal_block(base::xvblock_t * proposal_block) override;

private:
    virtual void clear_for_new_view() override;
    virtual xunit_service::xpreproposal_send_cb get_preproposal_send_cb() override;
    virtual bool process_msg(const xvip2_t & from_addr, const xvip2_t & to_addr, const base::xcspdu_t & packet, int32_t cur_thread_id, uint64_t timenow_ms) override;
    void send_preproposal(const data::xblock_consensus_para_t & cs_para,
                          const std::vector<data::xcons_transaction_ptr_t> & txs,
                          const std::vector<base::xvproperty_prove_ptr_t> & receiptid_state_proves);

private:
    // mutable std::mutex m_mutex;
    enum_received_msg_state_type m_msg_state{enum_received_msg_nil};
    data::xblock_ptr_t m_preproposal_block{nullptr};
};

NS_END2
