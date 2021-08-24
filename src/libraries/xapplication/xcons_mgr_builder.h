// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
// TODO(jimmy) #include "xbase/xvledger.h"

#include "xstore/xstore_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xunit_service/xcons_face.h"
#include "xmbus/xmessage_bus.h"
#include "xrouter/xrouter_face.h"

#include <string>

NS_BEG2(top, application)

// TODO(justin): remove use real cert auth object
class xschnorrcert_t : public base::xvcertauth_t {
public:
    xschnorrcert_t(const uint32_t total_nodes);

protected:
    virtual ~xschnorrcert_t();

private:
    xschnorrcert_t();
    xschnorrcert_t(const xschnorrcert_t &);
    xschnorrcert_t & operator=(const xschnorrcert_t &);

public:
    const uint32_t get_total_nodes() const { return m_total_nodes; }  // just for test purpose ,we put total nodes here

    virtual const std::string get_signer(const xvip2_t & signer);  // query account address of xvip2_t
    // all returned information build into a xvip_t structure
    virtual xvip_t get_validator_addr(const std::string & account_addr);                                    // mapping account to target group
    virtual bool verify_validator_addr(const base::xvblock_t * test_for_block);                             // verify validator and account
    virtual bool verify_validator_addr(const std::string & for_account, const base::xvqcert_t * for_cert);  // verify validator and account

public:
    virtual const std::string do_sign(const xvip2_t & signer, const base::xvqcert_t * sign_for_cert, const uint64_t random_seed) override;
    virtual const std::string do_sign(const xvip2_t & signer, const base::xvblock_t * sign_for_block, const uint64_t random_seed) override;

    virtual base::enum_vcert_auth_result verify_sign(const xvip2_t & signer, const base::xvqcert_t * test_for_cert, const std::string & block_account) override;
    virtual base::enum_vcert_auth_result verify_sign(const xvip2_t & signer, const base::xvblock_t * test_for_block) override;

    virtual const std::string merge_muti_sign(const std::vector<xvip2_t> & muti_nodes, const std::vector<std::string> & muti_signatures, const base::xvqcert_t * for_cert) override;
    virtual const std::string merge_muti_sign(const std::map<xvip2_t, std::string, xvip2_compare> & muti_nodes_signatures, const base::xvqcert_t * for_cert) override;
    virtual const std::string merge_muti_sign(const std::map<xvip2_t, std::string, xvip2_compare> & muti_nodes_signatures, const base::xvblock_t * for_block) override;

    virtual base::enum_vcert_auth_result verify_muti_sign(const base::xvqcert_t * test_for_cert, const std::string & block_account) override;
    virtual base::enum_vcert_auth_result verify_muti_sign(const base::xvblock_t * test_for_block) override;

private:
    uint32_t m_total_nodes;
};

class xdispatcher_builder : public xunit_service::xcons_dispatcher_builder_face {
public:
    xunit_service::xcons_dispatcher_ptr build(observer_ptr<mbus::xmessage_bus_face_t> const &mb,
            std::shared_ptr<xunit_service::xcons_service_para_face> const &, xunit_service::e_cons_type cons_type) override;
};

class xcons_mgr_builder {
public:
    static xunit_service::xcons_service_mgr_ptr build(std::string const & node_account,
                                                      observer_ptr<store::xstore_face_t> const & store,
                                                      observer_ptr<base::xvblockstore_t> const & blockstore,
                                                      observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                                                      observer_ptr<time::xchain_time_face_t> const & tx_timer,
                                                      xobject_ptr_t<base::xvcertauth_t> const & certauth,
                                                      observer_ptr<election::cache::xdata_accessor_face_t> const & accessor,
                                                      observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                                                      observer_ptr<router::xrouter_face_t> const & router);
};

NS_END2
