#pragma once

#include "xmutisig/xcommon/xcommon.h"
#include "xmutisig/xcommon/xbignum.h"
#include "xmutisig/xcommon/xecpoint.h"
#include "xbase/xobject.h"

NS_BEG2(top, xmutisig)

class xsecret_rand : public xbn_face
{
public:
    xsecret_rand() = default;

    xsecret_rand(const xsecret_rand &);

    xsecret_rand(const std::string &serialize_str);

    xsecret_rand& operator=(const xsecret_rand&);
    
};

class xrand_point : public xpoint_face
{
public:
    xrand_point() = delete;

    xrand_point(const xsecret_rand &secret_rand);

    xrand_point(const xrand_point &rand_point);

    xrand_point(const std::string &serialize_str);

    xrand_point& operator=(const xrand_point&);

};

//generate a pair : random->point, ECPoint = random(k) * G(generateor) 
class xecc_rand_t : public base::xrefcount_t
{
public:
    xecc_rand_t();
protected:
    virtual ~xecc_rand_t();
public:
    xmutisig::xsecret_rand        _ecc_secret;
    xmutisig::xrand_point         _ecc_point;
public:
    int                           used_count;   //how many times are reused
};

NS_END2
