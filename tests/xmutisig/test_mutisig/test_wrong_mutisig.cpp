#include "../test_common.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, wrong_mutisig) {

    assert(nullptr != m_data);
    assert(nullptr != m_agg_pub);
    assert(nullptr != m_agg_sign);
    assert(nullptr != m_agg_point);

    xsignature wrong_sign;

    bool check = xmutisig::verify_sign_base(wrong_sign,
                                           *m_agg_pub.get(),
                                           m_data,
                                           *m_agg_point.get(),
                                            m_schnorr);

    EXPECT_TRUE(!check);

}

TEST_F(test_schnorr_mutisig, agg_sign_num_wrong) {

    /*
    * rand select numbers < normal
    * rand select numbers > normal
    */

    std::vector<xsignature*> signs;
    signs.insert(signs.begin(), m_signs.begin(), m_signs.begin() + 100);

    std::shared_ptr<xsignature> wrong_agg_sign = xmutisig::aggregate_signs(signs, m_schnorr);

    bool check = xmutisig::verify_sign_base(*wrong_agg_sign.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                            m_schnorr);
    EXPECT_TRUE(!check);

    signs.clear();

    signs.insert(signs.begin(), m_signs.begin() + 200, m_signs.begin() + 1500);

    std::shared_ptr<xsignature> again_wrong_agg_sign = xmutisig::aggregate_signs(signs, m_schnorr);

    check = xmutisig::verify_sign_base(*again_wrong_agg_sign.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);

    signs.insert(signs.begin(), m_signs.begin() + 300, m_signs.begin() + 1500);

    std::shared_ptr<xsignature> triple_wrong_agg_pub = xmutisig::aggregate_signs(signs, m_schnorr);

    check = xmutisig::verify_sign_base(*triple_wrong_agg_pub.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);

    xsignature wrong_sign;
    signs.clear();
    signs.insert(signs.begin(), m_signs.begin(), m_signs.begin() + 1999);
    signs.emplace_back(&wrong_sign);

    assert(2000 == signs.size());

    std::shared_ptr<xsignature> four_wrong_agg_pub = xmutisig::aggregate_signs(signs, m_schnorr);
    check = xmutisig::verify_sign_base(*four_wrong_agg_pub.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);
}

NS_END2