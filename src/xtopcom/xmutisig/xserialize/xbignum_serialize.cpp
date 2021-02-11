#include "xbignum_serialize.h"

using top::xmutisig::xbignumserialize;
//using top::xmutisig::bn_ptr_t;

BIGNUM* xbignumserialize::deserialize(const std::string &src) {

    #ifdef DEBUG
    assert(0 != src.size());
    #endif

    //caller respond to call BN_Free() for the returned point(BIGNUM*)
    return BN_bin2bn((unsigned char *)src.c_str(),
                              (int)src.size(),
                              nullptr);

}


uint32_t xbignumserialize::serialize(std::string &bn_str, BIGNUM* bn) {
    assert(nullptr != bn);
    bn_str.clear();

    int len = BN_num_bytes(bn);

    std::vector<uint8_t> arr;
    arr.resize(len);
    fill(arr.begin(), arr.begin() + len, 0x00);

    if (BN_bn2bin(bn, arr.data()) != len) {
        assert(0);
        return 1; //error
    }

    bn_str.insert(bn_str.begin(), arr.begin(), arr.end());

    return 0;
}
