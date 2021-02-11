#include "xrand_pair.h"
#include "xmutisig/xschnorr/xschnorr.h"
#include "xmutisig/xcommon/xecpoint.h"

using top::xmutisig::xsecret_rand;
using top::xmutisig::xrand_point;
using top::xmutisig::xecc_rand_t;
//using top::xmutisig::bn_ptr_t;
//using top::xmutisig::ec_point_ptr_t;

xsecret_rand::xsecret_rand(const xsecret_rand &rand) : xbn_face((xbn_face)rand) {
}

xsecret_rand& xsecret_rand::operator=(const xsecret_rand&src) {
    xbn_face::operator=((xbn_face)src);
    return *this;
}

xsecret_rand::xsecret_rand(const std::string &serialize_str) : xbn_face(serialize_str) {
}


xrand_point::xrand_point(const xsecret_rand &rand) : xpoint_face((xbn_face)rand) {
}

xrand_point::xrand_point(const xrand_point &point) : xpoint_face((xpoint_face)point) {

}

xrand_point::xrand_point(const std::string &serialize_str) : xpoint_face(serialize_str) {

}

xrand_point& xrand_point::operator=(const xrand_point&src) {
    xpoint_face::operator=((xpoint_face)src);
    return *this;
}

xecc_rand_t::xecc_rand_t()
: _ecc_secret(),
  _ecc_point(_ecc_secret)
{
    used_count = 0;
}

xecc_rand_t::~xecc_rand_t()
{
}

