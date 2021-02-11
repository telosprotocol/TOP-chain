#pragma once

#include "xmutisig/xcommon/xcommon.h"
#include "xmutisig/xcommon/xbignum.h"
#include "xmutisig/xrand/xrand_pair.h"
#include "xmutisig/xkeys/xprikey.h"

NS_BEG2(top, xmutisig)
class xschnorr;
class xsignature : public xbn_face
{
public:
    xsignature() = default;
    xsignature(const xsignature &);
    xsignature(const std::string &);
    xsignature(const xsecret_rand &rand,
               BIGNUM* object,
               const xprikey &prikey,
               xschnorr * _schnorr);
    xsignature& operator=(const xsignature&);

};
NS_END2


