
#include "gtest/gtest.h"
#include "xunit_service/xleader_election.h"
#include  "xdata/xelection/xelection_info.h"
#include "../../mock/xmock_network.hpp"

#include <functional>

namespace top {
using namespace xunit_service;

static const std::vector<std::string> test_accounts = {
    "T00000Li18Pb8WuxQUphZPfmN6QCWTL9kwWPTns1",
    "T00000LawC9X7onZ23ftMfsBb8XtCHH7yHPbUWef",
    "T00000LZTVtSXAAHpw5L92Jw4VkatpX64YW6Sona",
    "T00000LfoXb1vXXQiPxyQbnHsUgZKrNfTqLj7Rmr",
    "T00000LUgUM5DcTb4Be4gUEwCWByhUf191Qu9nFr",
    "T00000LPSxv3AVu7Kb4oET1QsVzFDiofZHF2BBH4",
    "T00000LXtwPC88mTkBo5cSBZ6rFFwifnnvSU2sB3",
    "T00000LiYuDqviN9Kzi84t2DZhamN7E6tukogyv8",
    "T00000LewNgnEwG8E1FwxiqY6sb7et7B3VzKmr63",
    "T00000LSbd1EhPLc45TM8FnjFJFbjqUMaoeiiK7L",
};


TEST(xtest_get_leader, _) {

    bool insertFlag = true;
    unsigned int account_num = 10;
    std::vector<xvip2_t>  no_leader_xip;
    top::mock::xmock_addr_generator_t m_addr_generator;
    auto xelection_cache_imp_face = std::make_shared<xunit_service::xelection_cache_imp>();
    
    for (unsigned int j = 0; j < account_num; j++) {
        std::uint64_t version = j+3;
        common::xelection_round_t round_version{version};
        for (unsigned int i = 0; i < account_num; i++) {
            xelection_cache_imp::elect_set  elect_set_self;
            data::xnode_info_t node_info;
            node_info.election_info.joined_epoch(round_version);
            node_info.election_info.stake((100 + i * 10) * 10000);
            node_info.election_info.comprehensive_stake(10000);
            node_info.election_info.miner_type(common::xminer_type_t::validator);
            if ((i%2) != 0) {
                node_info.election_info.raw_credit_score(100000 * i);
            } else {
                node_info.election_info.raw_credit_score(8000 * i + 100);
            }
 
            node_info.address  = m_addr_generator.create_validator_addr(i, 10,test_accounts[i]);
            elect_set_self.emplace_back(node_info);
            auto xip = xunit_service::xcons_utl::to_xip2(node_info.address);
            std::vector<std::uint16_t> tables{1}; 
            xelection_cache_imp_face.get()->add(xip, elect_set_self, tables);
            //there xip not leader
            if (insertFlag == true && (i%2) == 0) {
                no_leader_xip.emplace_back(xip);
            }
        }

        insertFlag = false;
        for (unsigned int i = 0; i < account_num; i++) {
            auto address = m_addr_generator.create_validator_addr(i, 10,test_accounts[i]);
            auto local = xunit_service::xcons_utl::to_xip2(address);
            xelection_cache_face::elect_set elect_set;
            if (xelection_cache_imp_face.get() != nullptr) {
                auto len = xelection_cache_imp_face.get()->get_election(local, &elect_set);
                if (len <= 0) {
                    std::cout  << "error " << std::endl;
                    return ;
                }
            }
            version += 3;
            common::xelection_round_t round_version_tmp{version};
            uint64_t random = 10 + j*10+i + base::xvaccount_t::get_xid_from_account(test_accounts[i]);
            xvip2_t leader_xip  =  get_leader(elect_set,  round_version_tmp, random);
            auto result = std::find_if(no_leader_xip.begin(), no_leader_xip.end(), 
                                         [&](xvip2_t& tmp_xip) { return  xcons_utl::xip_equals(leader_xip, tmp_xip);}); 
            ASSERT_EQ(result == no_leader_xip.end(), 1);
         }
    }
}

}  // namespace top
