#include "xbignum.h"
#include "xmutisig/xschnorr/xschnorr.h"
#include "xmutisig/xserialize/xbignum_serialize.h"

using top::xmutisig::xbn_face;
//using top::xmutisig::bn_ptr_t;

xbn_face::xbn_face() {
    m_private_bn = nullptr;
    m_private_bn = xschnorr::instance()->generate_nonzero_bn();
    assert(nullptr != m_private_bn);
}


xbn_face::xbn_face(BIGNUM* bn) {
    assert(nullptr != bn);
    m_private_bn = bn;
}


xbn_face::xbn_face(const xbn_face &pri) {
    m_private_bn = nullptr;
    m_private_bn = BN_new();
    assert(nullptr != m_private_bn);

    BIGNUM *bn = BN_copy(m_private_bn, pri.bn_value());
    assert(nullptr != bn);
}

xbn_face& xbn_face::operator=(const xbn_face&pri) {
    BN_copy(m_private_bn, pri.bn_value());
    return *this;
}

xbn_face::xbn_face(const std::string &serialize_str)
{
    m_private_bn = nullptr;
    assert(0 != serialize_str.size());
    m_private_bn = xbignumserialize::deserialize(serialize_str);
    assert(nullptr != m_private_bn);
}

xbn_face::~xbn_face()
{
    if(m_private_bn != nullptr)
        BN_free(m_private_bn);
}

BIGNUM* xbn_face::bn_value() const {
    return m_private_bn;
}

std::string xbn_face::get_serialize_str() const {
    
    assert(nullptr != m_private_bn);

    std::string serialize_str;
    xbignumserialize::serialize(serialize_str, m_private_bn);
    assert(0 != serialize_str.size());

    return serialize_str;
}
