#include "xschnorr.h"
#include "xbase/xbase.h"
#include "../xmutisig.h"
#include "xschnorr.h"

#ifdef DEBUG
    #include <iostream>
#endif

using top::xmutisig::xmutisig;
using top::xmutisig::xrand_point;
using top::xmutisig::xpubkey;
using top::xmutisig::xsignature;
using top::xmutisig::xschnorr;
//using top::xmutisig::bn_ptr_t;


uint32_t xmutisig::sign(const std::string &msg,
                        const xprikey &prikey,
                        std::string &sign,
                        //std::string &point,
                        xsecret_rand &rand,
                        xrand_point &point,
                        xschnorr * _schnorr) {
#ifdef DEBUG
    xassert(!BN_is_zero(prikey.bn_value()));
    xassert((BN_cmp(prikey.bn_value(), _schnorr->curve()->bn_order()) == -1));
#endif

    BIGNUM* bn = generate_object_bn(msg,_schnorr);
    xassert(nullptr != bn);

    //xsignature tmp_sign;

    for (uint32_t i = 0; ; i++)
    {
        xsignature tmp_sign(rand, bn, prikey,_schnorr);
        //sign_base(rand,bn,prikey,tmp_sign,_schnorr);

        sign = tmp_sign.get_serialize_str();
        if (i > 3)
        {
            #ifdef DEBUG
            std::cout << "sign time beyond 3" << std::endl;
            #endif
        }
        if (BN_is_zero(tmp_sign.bn_value())) {
            rand = xsecret_rand();
            point = xrand_point(rand);
        }else {
            break;
        }
    }

    BN_free(bn);
    return 0;
}

bool xmutisig::verify_sign(const std::string &msg,
                           const xpubkey &pubkey,
                           const std::string &sign_str,
                           const std::string &point_str,
                           xschnorr * _schnorr) {
    xsignature sign(sign_str);
    xrand_point point(point_str);

    BIGNUM* bn = generate_object_bn(msg,_schnorr);
    xassert(nullptr != bn);

    bool result = verify_sign_base(sign, pubkey, bn, point,_schnorr);

    BN_free(bn);
    return result;

}

uint32_t xmutisig::sign_base(const xsecret_rand &rand,
							 BIGNUM* object,
							 const xprikey &prikey,
							 xsignature &sign,
                             xschnorr * _schnorr) {

#ifdef DEBUG
    xassert(nullptr != object);
    xassert(!BN_is_zero(prikey.bn_value()));
    xassert((BN_cmp(prikey.bn_value(), _schnorr->curve()->bn_order()) == -1));
#endif

    sign = xsignature(rand, object, prikey,_schnorr);
    return 0;
}

BIGNUM* xmutisig::generate_object_bn(const std::string &object,xschnorr * _schnorr) {

    xassert(0 != object.size());

    return _schnorr->generate_message_bn(object);

}

bool xmutisig::verify_sign_base(const xsignature &mutisign,
                                const xpubkey &agg_pubs,
                                BIGNUM* object,
                                const xrand_point &agg_point,
                                xschnorr * _schnorr) {
    xassert(nullptr != object);

    return _schnorr->verify_mutisign(mutisign, agg_pubs, object, agg_point);
}

void xmutisig::append_sign_points(xsignature *sign,
                                  xrand_point *point,
                                  std::shared_ptr<xrand_point> &point_ptr,
                                  std::shared_ptr<xsignature> &sign_ptr,
                                  xschnorr * _schnorr) {
    xassert(nullptr != sign);
    xassert(nullptr != point);
    xassert(nullptr != _schnorr);
    xassert(nullptr != point_ptr);
    xassert(nullptr != sign_ptr);

    auto ret = EC_POINT_add(_schnorr->curve()->ec_group(),
                            point_ptr->ec_point(),
                            point_ptr->ec_point(),
                            point->ec_point(),
                            NULL);
    xassert(0 != ret);  // TODO: Jimmy fix it when EC_POINT_add failed.

    std::unique_ptr<BN_CTX, void(*)(BN_CTX*)> ctx(BN_CTX_new(), BN_CTX_free);
    xassert(nullptr != ctx);

    ret = BN_mod_add(sign_ptr->bn_value(),
                     sign_ptr->bn_value(),
                     sign->bn_value(),
                     _schnorr->curve()->bn_order(),
                     ctx.get());
    xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_add failed.
}

void xmutisig::aggregate_sign_points(const std::vector<xrand_point *> &points,
                                     const std::vector<xsignature *> &signs,
                                     std::shared_ptr<xrand_point> &point_ptr,
                                     std::shared_ptr<xsignature> &sign_ptr,
                                     xschnorr * _schnorr) {
    xassert(!points.empty());
    xassert(points.size() == signs.size());
    xassert(nullptr != _schnorr);

    point_ptr = std::make_shared<xrand_point>(*points.at(0));
    xassert(nullptr != point_ptr);

    sign_ptr = std::make_shared<xsignature>(*signs.at(0));
    xassert(nullptr != sign_ptr);

    std::unique_ptr<BN_CTX, void(*)(BN_CTX*)> ctx(BN_CTX_new(), BN_CTX_free);
    xassert(nullptr != ctx);

    int ret = 0;
    for (size_t index = 1; index < points.size(); index++) {
        ret = EC_POINT_add(_schnorr->curve()->ec_group(),
                           point_ptr->ec_point(),
                           point_ptr->ec_point(),
                           points.at(index)->ec_point(),
                           NULL);
        xassert(0 != ret);  // TODO: Jimmy fix it when EC_POINT_add failed.

        ret = BN_mod_add(sign_ptr->bn_value(),
                         sign_ptr->bn_value(),
                         signs.at(index)->bn_value(),
                         _schnorr->curve()->bn_order(),
                         ctx.get());
        xassert(0 != ret); // TODO: Jimmy fix it when BN_mod_add failed.
    }
    return;
}

void xmutisig::aggregate_sign_points_2(const std::vector<xrand_point> &points,
                                     const std::vector<xsignature> &signs,
                                     std::shared_ptr<xrand_point> &point_ptr,
                                     std::shared_ptr<xsignature> &sign_ptr,
                                     xschnorr * _schnorr) {
    xassert(!points.empty());
    xassert(points.size() == signs.size());
    xassert(nullptr != _schnorr);

    point_ptr = std::make_shared<xrand_point>(points.at(0));
    xassert(nullptr != point_ptr);

    sign_ptr = std::make_shared<xsignature>(signs.at(0));
    xassert(nullptr != sign_ptr);

    std::unique_ptr<BN_CTX, void(*)(BN_CTX*)> ctx(BN_CTX_new(), BN_CTX_free);
    xassert(nullptr != ctx);

    int ret = 0;
    for (size_t index = 1; index < points.size(); index++) {
        ret = EC_POINT_add(_schnorr->curve()->ec_group(),
                           point_ptr->ec_point(),
                           point_ptr->ec_point(),
                           points.at(index).ec_point(),
                           NULL);
        xassert(0 != ret);  // TODO: Jimmy fix it when EC_POINT_add failed.

        ret = BN_mod_add(sign_ptr->bn_value(),
                         sign_ptr->bn_value(),
                         signs.at(index).bn_value(),
                         _schnorr->curve()->bn_order(),
                         ctx.get());
        xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_add failed.
    }
    return;
}

std::shared_ptr<xsignature> xmutisig::aggregate_signs(const std::vector<xsignature *> &signs, xschnorr * _schnorr) {
    xassert(0 != signs.size());

    std::shared_ptr<xsignature> agg_sign = std::make_shared<xsignature>(*signs.at(0));
    xassert(nullptr != agg_sign);

    std::unique_ptr<BN_CTX, void(*)(BN_CTX*)> ctx(BN_CTX_new(), BN_CTX_free);
    xassert(nullptr != ctx);

    int ret = 0;
    for (size_t index = 1; index < signs.size(); index++) {
        ret = BN_mod_add(agg_sign->bn_value(),
                         agg_sign->bn_value(),
                         signs.at(index)->bn_value(),
                         _schnorr->curve()->bn_order(),
                         ctx.get());
        xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_add failed.
    }

    return agg_sign;
}

std::shared_ptr<xrand_point> xmutisig::aggregate_rand_points(const std::vector<xrand_point *> &points,xschnorr * _schnorr) {
    xassert(0 != points.size());

    std::shared_ptr<xrand_point> agg_point = std::make_shared<xrand_point>(*points.at(0));
    xassert(nullptr != agg_point);

    int ret = 0;
    for (size_t index = 1; index < points.size(); index++) {
        ret = EC_POINT_add(_schnorr->curve()->ec_group(),
                           agg_point->ec_point(),
                           agg_point->ec_point(),
                           points.at(index)->ec_point(),
                           NULL);
        xassert(0 != ret);  // TODO: Jimmy fix it when EC_POINT_add failed.
    }

    return agg_point;
}

std::shared_ptr<xpubkey> xmutisig::aggregate_pubkeys(const std::vector<xpubkey *> &pubkeys,xschnorr * _schnorr) {
    xassert(0 != pubkeys.size());

    std::shared_ptr<xpubkey> agg_pubkey = std::make_shared<xpubkey>(*pubkeys.at(0));
    xassert(nullptr != agg_pubkey);

    int ret = 0;
    for (size_t index = 1; index < pubkeys.size(); index++) {
        ret = EC_POINT_add(_schnorr->curve()->ec_group(),
                           agg_pubkey->ec_point(),
                           agg_pubkey->ec_point(),
                           pubkeys.at(index)->ec_point(),
                           NULL);
        xassert(0 != ret);   // TODO: Jimmy fix it when EC_POINT_add failed.
    }

    return agg_pubkey;

}

std::shared_ptr<xpubkey> xmutisig::aggregate_pubkeys_2(const std::vector<xpubkey> &pubkeys,xschnorr * _schnorr) {
    xassert(0 != pubkeys.size());

    std::shared_ptr<xpubkey> agg_pubkey = std::make_shared<xpubkey>(pubkeys.at(0));
    xassert(nullptr != agg_pubkey);

    int ret = 0;
    for (size_t index = 1; index < pubkeys.size(); index++) {
        ret = EC_POINT_add(_schnorr->curve()->ec_group(),
                           agg_pubkey->ec_point(),
                           agg_pubkey->ec_point(),
                           pubkeys.at(index).ec_point(),
                           NULL);
        //return 1 on success and 0 if an error occured
        xassert(0 != ret);  // TODO: Jimmy fix it when EC_POINT_add failed.
        if (ret != 1) //force to stop it
            return nullptr;
    }
    return agg_pubkey;

}

