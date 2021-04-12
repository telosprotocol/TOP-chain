#pragma once

#include "xmutisig/xcommon/xcommon.h"
#include "xcurve.h"
#include "xmutisig/xkeys/xprikey.h"
#include "xmutisig/xkeys/xpubkey.h"
#include "xmutisig/xsignature/xsignature.h"
#include "xmutisig/xrand/xrand_pair.h"

#include "xbase/xns_macro.h"

NS_BEG2(top, xmutisig)

using key_pair_t = std::pair<xprikey, xpubkey>;
using rand_pair_t = std::pair<xsecret_rand, xrand_point>;

class xschnorr
{
public:
    static xschnorr * instance();
public:
    xschnorr();
    ~xschnorr();

public:

    /*
    * generate new key pair, private/public key
    */

    key_pair_t generate_key_pair();

    rand_pair_t generate_rand_pair();

    bool verify_mutisign(const xsignature &mutisign,
                         const xpubkey &agg_pubs,
                         BIGNUM* object,
                         const xrand_point &agg_point);

    uint32_t sign(const std::string &object, const xprikey &prikey, xsignature &signature);

public:

    static BIGNUM* generate_zero_bn();

    BIGNUM*  generate_message_bn(const std::string &message);

    BIGNUM*  generate_nonzero_bn();

    EC_POINT* generate_ec_point();

    BN_CTX* generate_bn_ctx();

public:
    xcurve* curve();

private:
    bool signature_bignum_legal(const xsignature &sign);

private:
    std::shared_ptr<xcurve>   m_curve{ nullptr };
    //std::mutex m_curve_mutex;
};

NS_END2
