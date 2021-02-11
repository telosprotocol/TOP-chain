#include <memory>
#include "xschnorr.h"
#include "xbase/xbase.h"
#include "xmutisig/xhash256/xsha256.h"

using top::xmutisig::xschnorr;
using top::xmutisig::xprikey;
using top::xmutisig::xpubkey;
using top::xmutisig::xcurve;
//using top::xmutisig::bn_ptr_t;
//using top::xmutisig::ec_point_ptr_t;
using top::xmutisig::xrand_point;
using top::xmutisig::xsecret_rand;
//using top::xmutisig::bn_ctx_ptr_t;
using top::xmutisig::key_pair_t;

xschnorr::xschnorr() : m_curve(std::make_shared<xcurve>()) {
    xassert(nullptr != m_curve);
}

xschnorr::~xschnorr()
{
    
}

xcurve* xschnorr::curve() {
    return m_curve.get();
}

xschnorr * xschnorr::instance() {
    static xschnorr static_schnorr;
    return &static_schnorr;
}

key_pair_t xschnorr::generate_key_pair() {

    //std::lock_guard<std::mutex> g(m_curve_mutex);

    xprikey prikey;
    xpubkey pubkey(prikey);

    return std::make_pair(xprikey(prikey), xpubkey(pubkey));
}

std::pair<xsecret_rand, xrand_point> xschnorr::generate_rand_pair() {

    //std::lock_guard<std::mutex> g(m_curve_mutex);

    xsecret_rand rand;
    xrand_point point(rand);

    return std::make_pair(rand, point);
}

bool xschnorr::signature_bignum_legal(const xsignature &sign) {
    bool irlegal = (BN_is_zero(sign.bn_value()) ||
                    BN_is_negative(sign.bn_value()) ||
                    (BN_cmp(sign.bn_value(), m_curve->bn_order()) != -1));
    return !irlegal;
}

bool xschnorr::verify_mutisign(const xsignature &mutisign,
                               const xpubkey &agg_pubs,
                               BIGNUM* object,
                               const xrand_point &agg_point) {
    //std::lock_guard<std::mutex> g(m_curve_mutex);
    xassert(nullptr != object);

    bool legal = signature_bignum_legal(mutisign);
    xassert(legal);

    BN_CTX* bn_ctx = generate_bn_ctx();
    xassert(nullptr != bn_ctx);

    EC_POINT* new_point = generate_ec_point();
    xassert(nullptr != new_point);
    if (!legal || nullptr == object || nullptr == bn_ctx || nullptr == new_point || m_curve == nullptr) {
        return false;
    }
    /* sign_base(signature) = rand - object * private-key
     ->s*G = rand* G - object * private-key*G  -->signature process
     ->s*G = point   - object * Public-Key     -->verification process
     
    * new_point = sign_base * G + object * agg_pubs
    * 1. sign_base = rand - object * prikey;
    * 2. rand * G = point
    * 3. prikey * G = pubs
    * so, new_point = point - object * pub + object * agg_pubs
    * if pub == aggpubs, get that new_point == point
    */

    //EC_POINT_mul calculates the value generator * n + q * m and stores the result in r. The value n may be NULL in which case the result is just q * m
    //int EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *n,const EC_POINT *q, const BIGNUM *m, BN_CTX *ctx);
    
    int ret = EC_POINT_mul(m_curve->ec_group(), new_point, mutisign.bn_value(),
                 agg_pubs.ec_point(), object, bn_ctx);
    
    //return 1 on success and 0 if an error occured
    xassert(0 != ret);
    if(ret != 1) //fail for point-muti
    {
        EC_POINT_free(new_point);
        BN_CTX_free(bn_ctx);
        return false;
    }

    bool result = (EC_POINT_cmp(m_curve->ec_group(),
                         new_point,
                         agg_point.ec_point(),
                         bn_ctx) == 0);
    
    EC_POINT_free(new_point);
    BN_CTX_free(bn_ctx);
    return result;
}

BIGNUM* xschnorr::generate_nonzero_bn() {

    BIGNUM* new_bn = BN_new();

    xassert(nullptr != new_bn);
    bool err = false;
    do {
        err = (BN_rand_range(new_bn, m_curve->bn_order()) == 0);
        if (err) {
            break;
        }
    } while (BN_is_zero(new_bn));

    xassert(!BN_is_zero(new_bn));

    return new_bn;
}

EC_POINT* xschnorr::generate_ec_point() {
    EC_POINT* new_ec_point = EC_POINT_new(m_curve->ec_group());
    xassert(nullptr != new_ec_point);
    return new_ec_point;
}

BN_CTX* xschnorr::generate_bn_ctx() {
    BN_CTX* bn_ctx = BN_CTX_new();
    xassert(nullptr != bn_ctx);

    return bn_ctx;
}

BIGNUM* xschnorr::generate_zero_bn() {
    return BN_new();
}

BIGNUM* xschnorr::generate_message_bn(const std::string &message) {
    
    if(message.empty())
        return nullptr;
    
    xassert(0 != message.size());

    xmutisig::_hash256 hash_obj;
    hash_obj.update(message);

    std::string hash_str = hash_obj.finish();

    //BN_bin2bn() converts the positive integer in big-endian form of length len at s into a BIGNUM and places it in ret. If ret is NULL, a new BIGNUM is created.
    //The error codes can be obtained by ERR_get_error
    BIGNUM* new_bn = BN_bin2bn((unsigned char*)hash_str.c_str(), (int)hash_str.size(), NULL);
 
    BIGNUM * r = new_bn;
    BIGNUM * m = r;
    BN_CTX * ctx = BN_CTX_new();
    int ret = BN_nnmod(r,
                       m,
                       m_curve->bn_order(),
                       ctx);
    xassert(0 != ret);
    BN_CTX_free(ctx);

    return new_bn;
}



uint32_t xschnorr::sign(const std::string &, const xprikey &, xsignature &) {
    // TODO (ernest), first do multisig
    return 0;
}
