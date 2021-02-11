#include "../test_common.h"

NS_BEG2(top, xmutisig)
const uint32_t NUM = 10000;

TEST_F(test_schnorr_mutisig, test_hash_performance) {
    std::string ctx = "testatataert;jaiopetjapigjadifjapifdjdaspgfjapifjdapifjapfjapjf";

    struct timeval beg, end;
    gettimeofday(&beg, NULL);
    for (uint32_t i = 0; i < NUM; i++) {
        xschnorr::instance()->generate_message_bn(ctx);
    }
    gettimeofday(&end, NULL);
    uint64_t ms_total = (end.tv_sec - beg.tv_sec) * 1000 + (end.tv_usec - beg.tv_usec) / 1000;
    std::cout << ms_total << "ms" << std::endl;
    std::cout << "xmutisign::hash256 performance " << NUM / ms_total << "/ms" << std::endl;
}

NS_END2