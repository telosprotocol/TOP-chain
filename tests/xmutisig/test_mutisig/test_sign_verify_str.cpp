#include "../test_common.h"

NS_BEG2(top, xmutisig)
#if 0
TEST_F(test_schnorr_mutisig, sign_verfiy_str) {

    assert(nullptr != m_data);
    assert(nullptr != m_agg_pub);
    assert(nullptr != m_agg_sign);
    assert(nullptr != m_agg_point);

    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";
    std::string sign_str;
    std::string point_str;

    xmutisig::xmutisig::sign(ctx,
                             *m_privkeys[0],
                             sign_str,
                             *m_secrets[0],
                             *m_points[0],
                             m_schnorr);

    point_str = m_points[0]->get_serialize_str();

    bool check = xmutisig::xmutisig::verify_sign(ctx,
                                                 *m_pubkeys[0],
                                                 sign_str,
                                                 point_str,
                                                 m_schnorr);
    EXPECT_TRUE(check);

}
#endif
NS_END2