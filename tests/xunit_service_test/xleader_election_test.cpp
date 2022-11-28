
// #include "../mock/xnetwork_mock.h"
// #include "gtest/gtest.h"
// #include "xunit_service/xleader_election.h"

// #include <functional>
// namespace top {
// using namespace xunit_service;

// class xelection_mock: public xelection {
// public:
//     xelection_mock() {
//         // mock data
//     }
// }

// class xrotate_leader_election_test : public testing::Test {
// protected:
//     void SetUp() override {}

//     void TearDown() override {}

// public:
// };

// TEST_F(xrotate_leader_election_test, rotate) {
//     xvip2_t xip;
//     set_zone_id_to_xip2(xip, base::enum_chain_zone_zec_index);
//     EXPECT_FALSE(xrotate_leader_election::is_rotate_xip(xip));
//     reset_zone_id_to_xip2(xip);
//     set_zone_id_to_xip2(xip, base::enum_chain_zone_beacon_index);
//     EXPECT_FALSE(xrotate_leader_election::is_rotate_xip(xip));
//     reset_zone_id_to_xip2(xip);
//     set_zone_id_to_xip2(xip, base::enum_chain_zone_consensus_index);
//     EXPECT_TRUE(xrotate_leader_election::is_rotate_xip(xip));
// }

// TEST_F(xrotate_leader_election_test, is_leader) {
//     // xrotate_leader_election election;
//     // election.is_leader(1, "T_xxxx", )
//     // EXPECT_TRUE(false);
// }
// }  // namespace top
