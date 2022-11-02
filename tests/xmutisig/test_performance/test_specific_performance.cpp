#include "../test_common.h"

#include "xmutisig/xmutisig_types/xcurve.h"

NS_BEG2(top, xmutisig)
const uint32_t NUM = 10000;

#if 0
TEST_F(test_schnorr_mutisig, test_EC_POINT_mul_performance) {
    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    rand_pair_t rand_pair = xschnorr::instance()->generate_rand_pair();
    std::string sign;
    std::string point;
    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";
    BIGNUM* data = xmutisig::generate_object_bn(ctx, m_schnorr);

    xmutisig::sign(ctx, keypair.first, sign, rand_pair.first, rand_pair.second, m_schnorr);
    point = rand_pair.second.get_serialize_str();

    BN_CTX* bn_ctx = xschnorr::instance()->generate_bn_ctx();
    assert(nullptr != bn_ctx);

    EC_POINT* new_point = xschnorr::instance()->generate_ec_point();
    assert(nullptr != new_point);

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        int ret = EC_POINT_mul(xschnorr::instance()->curve()->ec_group(), new_point, xsignature(sign).bn_value(), keypair.second.ec_point(), data, bn_ctx);
        assert(0 != ret);
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::EC_POINT_mul performance " << NUM / ms_total << "/ms" << std::endl;
}
#endif
NS_END2
