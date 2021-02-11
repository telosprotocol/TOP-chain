#include "../test_common.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, agg_pub_use_pubkey_wrong) {
    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    xpubkey wrong_pub = keypair.second;

    bool check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                           wrong_pub,
                                           m_data,
                                           *m_agg_point.get(),
                                            m_schnorr);

    EXPECT_TRUE(!check);

}

TEST_F(test_schnorr_mutisig, agg_pub_num_wrong) {

    /*
    * rand select numbers < normal
    * rand select numbers > normal
    */

    std::vector<xpubkey *> pubs;
    pubs.insert(pubs.begin(), m_pubkeys.begin(), m_pubkeys.begin() + 100);

    std::shared_ptr<xpubkey> wrong_agg_pub = xmutisig::aggregate_pubkeys(pubs, m_schnorr);

    bool check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                           *wrong_agg_pub.get(),
                                           m_data,
                                           *m_agg_point.get(),
                                            m_schnorr);
    EXPECT_TRUE(!check);

    pubs.clear();

    pubs.insert(pubs.begin(), m_pubkeys.begin() + 200, m_pubkeys.begin() + 1500);

    std::shared_ptr<xpubkey> again_wrong_agg_pub = xmutisig::aggregate_pubkeys(pubs, m_schnorr);

    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                      *again_wrong_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);

    pubs.insert(pubs.begin(), m_pubkeys.begin() + 300, m_pubkeys.begin() + 1500);

    std::shared_ptr<xpubkey> triple_wrong_agg_pub = xmutisig::aggregate_pubkeys(pubs, m_schnorr);

    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                      *triple_wrong_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);

    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    xpubkey wrong_pub = keypair.second;
    pubs.clear();
    pubs.insert(pubs.begin(), m_pubkeys.begin(), m_pubkeys.begin() + 1999);
    pubs.emplace_back(&wrong_pub);

    assert(2000 == pubs.size());

    std::shared_ptr<xpubkey> four_wrong_agg_pub = xmutisig::aggregate_pubkeys(pubs, m_schnorr);
    check = xmutisig::verify_sign_base(*m_agg_sign.get(),
                                      *four_wrong_agg_pub.get(),
                                      m_data,
                                      *m_agg_point.get(),
                                       m_schnorr);
    EXPECT_TRUE(!check);
}

NS_END2