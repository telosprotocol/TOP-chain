#include "../test_common.h"
#include "xmutisig/xserialize/xbignum_serialize.h"

NS_BEG2(top, xmutisig)

TEST_F(test_schnorr_mutisig, test_bignum_serialize) {

    BIGNUM* bn = xschnorr::instance()->generate_nonzero_bn();
    EXPECT_TRUE(nullptr != bn);

    std::string bn_str;
    uint32_t ret =  xbignumserialize::serialize(bn_str, bn);
    EXPECT_TRUE(0 == ret);

    std::cout << "bn_str " << bn_str << ", size: " << bn_str.size() <<  std::endl;

    BIGNUM* de_bn = xbignumserialize::deserialize(bn_str);

    int cmp = BN_cmp(bn, de_bn);

    EXPECT_TRUE(0 == cmp);

    BN_free(de_bn);

}

NS_END2
