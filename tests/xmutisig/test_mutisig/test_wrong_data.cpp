#include "../test_common.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, wrong_data) {

    assert(nullptr != m_data);
    assert(nullptr != m_agg_pub);
    assert(nullptr != m_agg_sign);
    assert(nullptr != m_agg_point);

    BIGNUM* wrong_data = xschnorr::instance()->generate_nonzero_bn();
    assert(nullptr != wrong_data);

    bool check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                           *m_agg_pub.get(),
                                           wrong_data,
                                           *m_agg_point.get(),
                                            m_schnorr);

    EXPECT_TRUE(!check);

}

NS_END2