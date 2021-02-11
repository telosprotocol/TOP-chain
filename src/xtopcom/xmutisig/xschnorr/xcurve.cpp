#include "xcurve.h"
#include "xbase/xbase.h"
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslv.h>

using top::xmutisig::xcurve;
//using top::xmutisig::ec_group_ptr_t;
//using top::xmutisig::bn_ptr_t;

xcurve::xcurve()
{
    m_ec_group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    xassert(nullptr != m_ec_group);
    
    m_bn_order = BN_new();
    xassert(nullptr != m_bn_order);
    BN_clear(m_bn_order);

    /** Gets the order of a EC_GROUP
    *  \param  group  EC_GROUP object
    *  \param  order  BIGNUM to which the order is copied
    *  \param  ctx    unused
    *  \return 1 on success and 0 if an error occurred
    */
    int ret = EC_GROUP_get_order(m_ec_group, m_bn_order, NULL);
    xassert(0 != ret);
    
    EC_GROUP_precompute_mult(m_ec_group, NULL);
}

xcurve::~xcurve()
{
    EC_GROUP_clear_free(m_ec_group);
}

EC_GROUP* xcurve::ec_group() const {
    return m_ec_group;
}

BIGNUM* xcurve::bn_order() const {
    return m_bn_order;
}
