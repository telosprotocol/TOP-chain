#include "../test_common.h"
#include "xmutisig/xserialize/xecpoint_serialize.h"
#include "xmutisig/xmutisig_types/xcurve.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, test_ecpoint_serialize) {

    EC_POINT* point = m_agg_point->ec_point();
    EXPECT_TRUE(nullptr != point);

    std::string point_str;
    uint32_t ret = xecpointserialize::serialize(point_str, point);
    EXPECT_TRUE(0 == ret);

    std::cout << "point_str " << point_str << ",size:" << point_str.size() << std::endl;

    EC_POINT* de_point = xecpointserialize::deserialize(point_str);

    BN_CTX* bn_ctx = xschnorr::instance()->generate_bn_ctx();
    EXPECT_TRUE(nullptr != bn_ctx);

    int cmp = EC_POINT_cmp(xschnorr::instance()->curve()->ec_group(),
                           point,
                           de_point,
                           bn_ctx);

    EXPECT_TRUE(0 == cmp);

}

NS_END2
