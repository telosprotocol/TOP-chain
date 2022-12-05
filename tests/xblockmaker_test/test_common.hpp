#include "gtest/gtest.h"
#include "xvledger/xvcertauth.h"
#include "xvledger/xvledger.h"

// #include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "xtxpool_v2/xtxpool_face.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xtestdb.hpp"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::blockmaker;
using namespace top::store;
using namespace top::base;
using namespace top::mock;
using namespace top::xtxpool_v2;

class test_xmock_auth_t : public base::xvcertauth_t {
public:
    test_xmock_auth_t() {  }
    virtual ~test_xmock_auth_t() {}

    const std::string get_signer(const xvip2_t & signer) override {
        return base::xstring_utl::tostring(signer.low_addr);
    }

    xvip_t get_validator_addr(const std::string & account_addr) override {
        return 0;
    }
    bool verify_validator_addr(const base::xvblock_t * test_for_block) override {
        return true;
    }
    bool verify_validator_addr(const std::string & for_account,const base::xvqcert_t * for_cert) override {
        return true;
    }

    const std::string do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed) override {
        std::string random_seed_string = base::xstring_utl::tostring(base::xtime_utl::get_fast_random64());
        if (random_seed != 0)
            random_seed_string += base::xstring_utl::tostring(random_seed);

        std::string bin;
        ((base::xvqcert_t *) sign_for_cert)->serialize_to_string(bin);
        return bin + random_seed_string;
    }

    const std::string do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,
                              const uint64_t random_seed, const std::string sign_hash) override {
        return do_sign(signer, sign_for_cert, random_seed);
    }

    const std::string do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed) override {
        return do_sign(signer, sign_for_block->get_cert(), random_seed);
    }

    base::enum_vcert_auth_result verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,const std::string & block_account) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    base::enum_vcert_auth_result   verify_sign(const xvip2_t & signer,const xvqcert_t * test_for_cert,
                                               const std::string & block_account, const std::string sign_hash) override {
        return base::enum_vcert_auth_result::enum_successful;
    } 

    base::enum_vcert_auth_result verify_sign(const xvip2_t & signer,const base::xvblock_t * test_for_block) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    base::enum_vcert_auth_result verify_muti_sign(const base::xvqcert_t * test_for_cert, const std::string & block_account) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    base::enum_vcert_auth_result verify_muti_sign(const base::xvblock_t * test_for_block) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    const std::string merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const base::xvqcert_t * for_cert) override {
        std::string bin;
        ((base::xvqcert_t *)for_cert)->serialize_to_string(bin);
        for(auto const& s : muti_signatures) {
            bin += s;
        }
        return bin;
    }

    const std::string merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_nodes_signatures,const base::xvqcert_t * for_cert) override {
        std::string bin;
        ((base::xvqcert_t *)for_cert)->serialize_to_string(bin);
        for(auto const& pair : muti_nodes_signatures) {
            bin += pair.second;
        }
        return bin;
    }

    const std::string merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_nodes_signatures,const base::xvblock_t * for_block) override {
        return merge_muti_sign(muti_nodes_signatures, for_block->get_cert());
    }

    const std::string get_pubkey(const xvip2_t & signer) override {
        return {};
    }
};

class test_xblockmaker_resources_t : public xblockmaker_resources_t {
 public:
    static xblockmaker_resources_ptr_t create() {
        xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();
        return resources;
    }

 public:
    test_xblockmaker_resources_t() {
        m_ca = make_object_ptr<test_xmock_auth_t>();
        m_bus = make_object_ptr<mbus::xmessage_bus_t>(true, 1000);
        m_txpool = xtxpool_instance::create_xtxpool_inst(make_observer(get_blockstore()), make_observer(m_ca.get()), make_observer(m_bus.get()));
    }

    virtual base::xvblockstore_t*       get_blockstore() const {return m_creator.get_blockstore();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_txpool.get();}
    virtual mbus::xmessage_bus_face_t*  get_bus() const {return m_bus.get();}

 private:
    xvchain_creator                 m_creator{true};
    xobject_ptr_t<base::xvcertauth_t>   m_ca{nullptr};
    xobject_ptr_t<mbus::xmessage_bus_face_t> m_bus{nullptr};
    xobject_ptr_t<xtxpool_face_t>   m_txpool{nullptr};
};


