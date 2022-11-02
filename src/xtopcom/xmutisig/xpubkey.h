#pragma once

#include "xmutisig/xecpoint.h"
#include "xmutisig/xprikey.h"

NS_BEG2(top, xmutisig)

// public key is a EC Point at Curve through = pk * G
class xpubkey : public xpoint_face {
public:
    xpubkey() = delete;

    xpubkey(const xprikey & prikey);

    xpubkey(const xpubkey & pubkey);

    xpubkey(const std::string & serialize_str);

    xpubkey & operator=(const xpubkey &);
};

NS_END2
