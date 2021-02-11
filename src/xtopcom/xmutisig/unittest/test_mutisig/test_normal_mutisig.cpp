#include "../test_common.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, first_normal_mutisign) {
    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    rand_pair_t rand_pair = xschnorr::instance()->generate_rand_pair();
    xsignature sign;
    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";
    BIGNUM * data = xmutisig::generate_object_bn(ctx, m_schnorr);

    xmutisig::sign_base(rand_pair.first,
                        data,
                        keypair.first,
                        sign,
                        m_schnorr);
    assert(nullptr != sign.bn_value());

    bool check = false;
    check = xmutisig::verify_sign_base(sign,
                                       keypair.second,
                                       data,
                                       rand_pair.second,
                                       m_schnorr);
    EXPECT_TRUE(check);
}

TEST_F(test_schnorr_mutisig, normal_mutisign) {

    assert(nullptr != m_data);
    assert(nullptr != m_agg_pub);
    assert(nullptr != m_agg_sign);
    assert(nullptr != m_agg_point);

    bool check;

    for (int i = 0; i < m_node_num; i++) {
        check = xmutisig::verify_sign_base(*m_signs[i],
                                           *m_pubkeys[i],
                                           m_data,
                                           *m_points[i],
                                           m_schnorr);
        EXPECT_TRUE(check);
    }

    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                       *m_agg_pub.get(),
                                       m_data,
                                       *m_agg_point.get(),
                                       m_schnorr);

    EXPECT_TRUE(check);

}

NS_END2
