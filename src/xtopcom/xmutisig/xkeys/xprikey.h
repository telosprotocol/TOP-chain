#pragma once

#include "xmutisig/xcommon/xcommon.h"
#include "xmutisig/xcommon/xbignum.h"

NS_BEG2(top, xmutisig)

//private key is a random number
class xprikey : public xbn_face
{
public:
    xprikey() = default;//generate a random number as priate key at xbn_face

    xprikey(const uint64_t random_seed);
    
    xprikey(BIGNUM* bn);

    xprikey(const xprikey &);

    xprikey(const std::string &serialize_str);

    xprikey& operator=(const xprikey&);
public:
    std::string to_string();  //convert 32bytes raw data of private key
};

NS_END2
