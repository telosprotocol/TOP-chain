#include "xmutisig/xschnorr/xschnorr.h"
#include "xmutisig/xsignature/xsignature.h"


//using top::xmutisig::bn_ptr_t;
using top::xmutisig::xsignature;

using top::xmutisig::xsignature;

xsignature::xsignature(const std::string &serialize_str) : xbn_face(serialize_str) {

}

xsignature::xsignature(const xsignature &sign) : xbn_face((xbn_face)sign) {
}

xsignature& xsignature::operator=(const xsignature&sign) {
    xbn_face::operator=((xbn_face)sign);
    return *this;
}

xsignature::xsignature(const xsecret_rand &rand,
                       BIGNUM* object,
                       const xprikey &prikey,
                       xschnorr * _schnorr) {

    // sign = rand - object * prikey
    std::unique_ptr<BN_CTX, void(*)(BN_CTX*)> ctx(BN_CTX_new(), BN_CTX_free);
    assert(nullptr != ctx);

#ifdef DEBUG
    assert(!BN_is_zero(prikey.bn_value()));
    assert((BN_cmp(prikey.bn_value(), _schnorr->curve()->bn_order())== -1));
#endif

    // object * prikey
    //r=(a*b) mod m ---> m_private_bn(signature) = (object * prikey) % bn_order
    int ret = BN_mod_mul(m_private_bn,
                         object,
                         prikey.bn_value(),
                         _schnorr->curve()->bn_order(),
                         ctx.get());

    xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_mul failed.
    //r = |(a - b)| % m
    ret = BN_mod_sub(m_private_bn,
                     rand.bn_value(),
                     m_private_bn,
                     _schnorr->curve()->bn_order(),
                     ctx.get());
    xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_sub failed.

}

