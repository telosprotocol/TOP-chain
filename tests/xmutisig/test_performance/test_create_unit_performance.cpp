#include "../test_common.h"

NS_BEG2(top, xmutisig)
const uint32_t NUM = 10000;

TEST_F(test_schnorr_mutisig, test_create_rand_performance) {

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xsecret_rand rand;
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xsecret_rand performance " << NUM / ms_total << "/ms" << std::endl;

}

TEST_F(test_schnorr_mutisig, test_create_point_performance) {

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xsecret_rand rand;
        xrand_point point(rand);
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::xrand_point performance " << NUM / ms_total << "/ms" << std::endl;

}

NS_END2