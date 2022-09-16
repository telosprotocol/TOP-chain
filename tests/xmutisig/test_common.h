#include <gtest/gtest.h>
//#include <gmock/gmock.h>
#include "xmutisig/xschnorr.h"
#include "xmutisig/xmutisig.h"
#include "xcrypto/xckey.h"
#include "xdata/xdatautil.h"
#include "xbase/xutl.h"
#include <sys/time.h>
#include <iostream>

#include "openssl/bn.h"

NS_BEG2(top, xmutisig)

class test_schnorr_mutisig : public testing::Test
{
public:
    test_schnorr_mutisig(int num = 2000) {
        m_node_num = num;
        m_data = xschnorr::instance()->generate_nonzero_bn();
        assert(nullptr != m_data);
    }
protected:
    void SetUp() override {
        m_schnorr = new xschnorr();
        init_keys();
        init_rand();
        init_signs();
        agg_pub();
        agg_sign();
        agg_point();
    }
    void TearDown() override {
        clear();
    }

private:
    void init_keys() {
        for (int i = 0; i < m_node_num; i++) {
            xprikey *pri = new xprikey();
            xpubkey *pub = new xpubkey(*pri);
            m_privkeys.emplace_back(pri);
            m_pubkeys.emplace_back(pub);
        }
    }

    void init_rand() {
        for (int i = 0; i < m_node_num; i++) {
            xsecret_rand *rand = new xsecret_rand();
            xrand_point *point = new xrand_point(*rand);
            m_secrets.emplace_back(rand);
            m_points.emplace_back(point);
        }
    }

    void init_signs() {

        assert(nullptr != m_data);
        for (int i = 0; i < m_node_num; i++) {
            xsignature *sign =  new xsignature();
            xmutisig::sign_base(*m_secrets.at(i),
                               m_data,
                               *m_privkeys.at(i),
                               *sign,
                               m_schnorr);
            assert(nullptr != sign->bn_value());
            if (BN_is_zero(sign->bn_value())) {
                assert(0);
            }
            m_signs.emplace_back(sign);
        }
    }

    void agg_pub() {
        m_agg_pub = xmutisig::aggregate_pubkeys(m_pubkeys, m_schnorr);
    }

    void agg_sign() {
        m_agg_sign = xmutisig::aggregate_signs(m_signs, m_schnorr);
    }

    void agg_point() {
        m_agg_point = xmutisig::aggregate_rand_points(m_points, m_schnorr);
    }

    void clear() {
        m_privkeys.clear();
        m_pubkeys.clear();
        m_secrets.clear();
        m_points.clear();

        m_agg_pub.reset();
        m_agg_point.reset();
        m_agg_sign.reset();
    }

public:
    int m_node_num{ 2000 };
    std::vector<xprikey *> m_privkeys;
    std::vector<xpubkey *> m_pubkeys;

    std::vector<xsecret_rand *> m_secrets;
    std::vector<xrand_point *> m_points;

    std::vector<xsignature *> m_signs;

    std::shared_ptr<xpubkey> m_agg_pub;

    std::shared_ptr<xsignature> m_agg_sign;

    std::shared_ptr<xrand_point> m_agg_point;

    BIGNUM* m_data{ nullptr };

    xschnorr *m_schnorr{ nullptr };
};

NS_END2
