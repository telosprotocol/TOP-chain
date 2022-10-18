#include "../test_common.h"

NS_BEG2(top, xmutisig)
const uint32_t NUM = 10000;
#if 0
TEST_F(test_schnorr_mutisig, test_sign_face_performance) {
    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    rand_pair_t rand_pair = xschnorr::instance()->generate_rand_pair();
    std::string sign;
    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xmutisig::sign(ctx, keypair.first, sign, rand_pair.first, rand_pair.second, m_schnorr);
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::sign performance " << NUM / ms_total << "/ms" << std::endl;
}

TEST_F(test_schnorr_mutisig, test_verify_face_performance) {
    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    rand_pair_t rand_pair = xschnorr::instance()->generate_rand_pair();
    std::string sign;
    std::string point;
    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";
    BIGNUM* data = xmutisig::generate_object_bn(ctx, m_schnorr);

    xmutisig::sign(ctx, keypair.first, sign, rand_pair.first, rand_pair.second, m_schnorr);
    point = rand_pair.second.get_serialize_str();

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        bool check = xmutisig::verify_sign(ctx, keypair.second, sign, point, m_schnorr);
        assert(check);
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::verify_sign performance " << NUM / ms_total << "/ms" << std::endl;
}

TEST_F(test_schnorr_mutisig, test_verify_base_face_performance) {
    key_pair_t keypair = xschnorr::instance()->generate_key_pair();
    rand_pair_t rand_pair = xschnorr::instance()->generate_rand_pair();
    std::string sign;
    std::string point;
    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";
    BIGNUM* data = xmutisig::generate_object_bn(ctx, m_schnorr);

    xmutisig::sign(ctx, keypair.first, sign, rand_pair.first, rand_pair.second, m_schnorr);
    point = rand_pair.second.get_serialize_str();

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        bool check = xmutisig::verify_sign_base(sign, keypair.second, data, point, m_schnorr);
        assert(check);
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::verify_sign_base performance " << NUM / ms_total << "/ms" << std::endl;
}
#endif
NS_END2