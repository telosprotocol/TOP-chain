#pragma once
#include "xbase/xns_macro.h"
#include "xmutisig/xbignum.h"
#include "xmutisig/xprikey.h"
#include "xmutisig/xrand_pair.h"

NS_BEG2(top, xmutisig)
class xschnorr;
class xsignature : public xbn_face {
public:
    xsignature() = default;
    xsignature(const xsignature &);
    xsignature(const std::string &);
    xsignature(const xsecret_rand & rand, BIGNUM * object, const xprikey & prikey, xschnorr * _schnorr);
    xsignature & operator=(const xsignature &);
};
NS_END2
