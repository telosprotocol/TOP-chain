#include "../test_common.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, wrong_point) {

    assert(nullptr != m_data);
    assert(nullptr != m_agg_pub);
    assert(nullptr != m_agg_sign);
    assert(nullptr != m_agg_point);

    rand_pair_t rands = xschnorr::instance()->generate_rand_pair();

    xrand_point wrong_point = rands.second;

    bool check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                           *m_agg_pub.get(),
                                           m_data,
                                           wrong_point,
                                            m_schnorr);

    EXPECT_TRUE(!check);

}

TEST_F(test_schnorr_mutisig, agg_point_num_wrong) {

    /*
    * rand select numbers < normal
    * rand select numbers > normal
    */

    std::vector<xrand_point *> points;
    points.insert(points.begin(), m_points.begin(), m_points.begin() + 100);

    std::shared_ptr<xrand_point> wrong_agg_point = xmutisig::aggregate_rand_points(points, m_schnorr);

    bool check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                           *m_agg_pub.get(),
                                           m_data,
                                           *wrong_agg_point.get(),
                                            m_schnorr);
    EXPECT_TRUE(!check);

    points.clear();

    points.insert(points.begin(), m_points.begin() + 200, m_points.begin() + 1500);

    std::shared_ptr<xrand_point> again_wrong_agg_point = xmutisig::aggregate_rand_points(points, m_schnorr);

    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *again_wrong_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);

    points.insert(points.begin(), m_points.begin() + 300, m_points.begin() + 1500);

    std::shared_ptr<xrand_point> triple_wrong_agg_point = xmutisig::aggregate_rand_points(points, m_schnorr);

    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *triple_wrong_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);

    rand_pair_t rands = xschnorr::instance()->generate_rand_pair();
    xrand_point wrong_point = rands.second;

    points.clear();
    points.insert(points.begin(), m_points.begin(), m_points.begin() + 1999);
    points.emplace_back(&wrong_point);

    assert(2000 == points.size());

    std::shared_ptr<xrand_point> four_wrong_agg_point = xmutisig::aggregate_rand_points(points, m_schnorr);
    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                      *m_agg_pub.get(),
                                      m_data,
                                      *four_wrong_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);
}

NS_END2