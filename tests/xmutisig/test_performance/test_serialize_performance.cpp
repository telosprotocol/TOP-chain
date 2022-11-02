#include "../test_common.h"

#include "xmutisig/xmutisig_types/xcurve.h"

NS_BEG2(top, xmutisig)

const uint32_t NUM = 200000;

TEST_F(test_schnorr_mutisig, test_rand_serialize_performance) {
    xsecret_rand rrand;
    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    std::string str;
    for (uint32_t i = 0; i < NUM; i++) {
        str = rrand.get_serialize_str();
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xsecret_rand serialize performance " << NUM / ms_total << "/ms" << std::endl;

    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xsecret_rand tmp = xsecret_rand(str);
    }
    gettimeofday(&end, NULL);
    ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xsecret_rand deserialize performance " << NUM / ms_total << "/ms" << std::endl;
}

TEST_F(test_schnorr_mutisig, test_point_serialize_performance) {
    xsecret_rand rrand;
    xrand_point point(rrand);
    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    std::string str;
    for (uint32_t i = 0; i < NUM; i++) {
        str = point.get_serialize_str();
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xrand_point serialize performance " << NUM / ms_total << "/ms" << std::endl;

    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xrand_point tmp = xrand_point(str);
    }
    gettimeofday(&end, NULL);
    ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xrand_point deserialize performance " << NUM / ms_total << "/ms" << std::endl;
}

TEST_F(test_schnorr_mutisig, test_signature_serialize_performance) {
    xsignature sign;
    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    std::string str;
    for (uint32_t i = 0; i < NUM; i++) {
        str = sign.get_serialize_str();
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xsignature serialize performance " << NUM / ms_total << "/ms" << std::endl;

    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xsignature tmp = xsignature(str);
    }
    gettimeofday(&end, NULL);
    ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xsignature deserialize performance " << NUM / ms_total << "/ms" << std::endl;
}

NS_END2