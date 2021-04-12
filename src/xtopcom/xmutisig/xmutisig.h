#pragma once

#include "xcommon/xcommon.h"
#include "xrand/xrand_pair.h"
#include "xkeys/xprikey.h"
#include "xkeys/xpubkey.h"
#include "xsignature/xsignature.h"


#include "xbase/xns_macro.h"

NS_BEG2(top, xmutisig)
class xschnorr;
class xmutisig
{
private:
    xmutisig() = default;
    xmutisig(xmutisig const &) = delete;
    void operator=(xmutisig const &) = delete;

public:

    /*
    * get sign pair<sign, point>
    * msg: biz message
    * prikey: node private key
    * sign: signature serialize str
    * point: point serialize str
    */

    static uint32_t sign(const std::string &msg,
                         const xprikey &prikey,
                         std::string &sign,
                         //std::string &point,
                         xsecret_rand &rand,
                         xrand_point &point,
                         xschnorr * _schnorr);

    /*
    * verify sign pair<sign, point>, include muti_sign/muti_point
    */

    static bool verify_sign(const std::string &msg,
                            const xpubkey &pubkey,
                            const std::string &sign,
                            const std::string &point,
                            xschnorr * _schnorr);

public:

    /*
    * verify signature/muti_signature, true: ok, false: failed
    * params:
    * 1. sign: signature/muti_signature
    * 2. pubkey: pubkey/aggegrate pubkeys in bitmap
    * 3. bn: signature bn, hash256 by biz msg
    * 4. agg_point: rand_point/aggegrate rand_points, be parallel to signature
    */

    static bool verify_sign_base(const xsignature &sign,
                                const xpubkey &pubkey,
                                BIGNUM* bn,
                                const xrand_point &point,
                                xschnorr * _schnorr);

public:
    /*
    * biz face, get ecc bn from biz_str object
    */
    static BIGNUM* generate_object_bn(const std::string &object,xschnorr * _schnorr);

public:
    static void aggregate_sign_points(const std::vector<xrand_point *> &points,
                                      const std::vector<xsignature *> &signs,
                                      std::shared_ptr<xrand_point> &point_ptr,
                                      std::shared_ptr<xsignature> &sign_ptr,
                                      xschnorr * _schnorr);
    static void aggregate_sign_points_2(const std::vector<xrand_point> &points,
                                      const std::vector<xsignature> &signs,
                                      std::shared_ptr<xrand_point> &point_ptr,
                                      std::shared_ptr<xsignature> &sign_ptr,
                                      xschnorr * _schnorr);
    static void append_sign_points(xsignature *sign,
                                   xrand_point *point,
                                   std::shared_ptr<xrand_point> &point_ptr,
                                   std::shared_ptr<xsignature> &sign_ptr,
                                   xschnorr * _schnorr);

    static std::shared_ptr<xrand_point> aggregate_rand_points(const std::vector<xrand_point *> &points,xschnorr * _schnorr);

    static std::shared_ptr<xpubkey> aggregate_pubkeys(const std::vector<xpubkey *> &pubkeys,xschnorr * _schnorr);
    static std::shared_ptr<xpubkey> aggregate_pubkeys_2(const std::vector<xpubkey> &pubkeys,xschnorr * _schnorr);

    static std::shared_ptr<xsignature> aggregate_signs(const std::vector<xsignature *> &signs,xschnorr * _schnorr);

private:
    /*
     * sign_base face, 0:success, other:failed
     * param
     * 1.rand: random BIGNUM in signature, protect privatekey, maybe created by biz
     * 2.object: signature object, BIGNUM;
     *         can call function 'generate_object_bn', convert string to BIGNUM
     * 3.prikey: private key
     * 4.sign: out xsignature
     */

public:
    static uint32_t sign_base(const xsecret_rand &rand,
                              BIGNUM* object,
                              const xprikey &prikey,
                              xsignature &sign,
                              xschnorr * _schnorr);
private:
    std::mutex m_mutex;
};

NS_END2
