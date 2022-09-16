#include "xmutisig/xpubkey.h"

#include "xmutisig/xbignum.h"
#include "xmutisig/xschnorr.h"

using top::xmutisig::xpubkey;
// using top::xmutisig::ec_point_ptr_t;

xpubkey::xpubkey(const xprikey & prikey) : xpoint_face((xbn_face)prikey) {
}

xpubkey::xpubkey(const xpubkey & pubkey) : xpoint_face((xpoint_face)pubkey) {
}

xpubkey::xpubkey(const std::string & serialize_str) : xpoint_face(serialize_str) {
}

xpubkey & xpubkey::operator=(const xpubkey & src) {
    xpoint_face::operator=((xpoint_face)src);
    return *this;
}
