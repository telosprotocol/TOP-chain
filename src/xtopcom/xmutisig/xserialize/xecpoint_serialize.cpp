#include "xecpoint_serialize.h"
#include "xbignum_serialize.h"
#include "xmutisig/xschnorr/xschnorr.h"

using top::xmutisig::xecpointserialize;
//using top::xmutisig::bn_ptr_t;
//using top::xmutisig::ec_point_ptr_t;

EC_POINT* xecpointserialize::deserialize(const std::string &src) {

    assert(0 != src.size());

    BIGNUM* bn = xbignumserialize::deserialize(src);

    assert(nullptr != bn);

    BN_CTX* bn_ctx = xschnorr::instance()->generate_bn_ctx();
    assert(nullptr != bn_ctx);

    EC_POINT* ec_point_ptr_t = EC_POINT_bn2point(xschnorr::instance()->curve()->ec_group(),bn,NULL,bn_ctx);
 
    BN_free(bn);
    BN_CTX_free(bn_ctx);
    return ec_point_ptr_t;
}


uint32_t xecpointserialize::serialize(std::string &point_str, EC_POINT* ec_point) {
    
    assert(nullptr != ec_point);
    
    point_str.clear();
    BN_CTX* bn_ctx = xschnorr::instance()->generate_bn_ctx();
    assert(nullptr != bn_ctx);

    BIGNUM * bn = EC_POINT_point2bn(xschnorr::instance()->curve()->ec_group(),
                          ec_point,
                          POINT_CONVERSION_COMPRESSED,
                          NULL,bn_ctx);

    assert(nullptr != bn);

    uint32_t result = xbignumserialize::serialize(point_str, bn);
    BN_free(bn);
    BN_CTX_free(bn_ctx);
    return result;
}
