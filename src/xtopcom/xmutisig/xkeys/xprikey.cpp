#include "xprikey.h"
#include "xmutisig/xschnorr/xschnorr.h"

using top::xmutisig::xprikey;
//using top::xmutisig::bn_ptr_t;

xprikey::xprikey(const xprikey &pri) : xbn_face((xbn_face)pri)
{
}

xprikey::xprikey(const uint64_t random_seed) :xbn_face()
{
    BIGNUM* new_bn = BN_new();
    BN_set_word(new_bn, (uint32_t)random_seed);
    BN_add_word(new_bn, (uint32_t)(random_seed >> 32));
    
    //add into existing private_bn with more random
    BN_add(m_private_bn, m_private_bn, new_bn);
    BN_free(new_bn);
}

xprikey::xprikey(BIGNUM* bn) : xbn_face(bn)
{
}


xprikey::xprikey(const std::string &serialize_str) : xbn_face(serialize_str) {

}

xprikey& xprikey::operator=(const xprikey&pri) {
    xbn_face::operator=((xbn_face)pri);
    return *this;
}

std::string xprikey::to_string() //convert 32bytes raw data of private key
{
    const int len = BN_num_bytes(m_private_bn);
    xassert(len <= 32);
    
    uint8_t bin_data[32];
    memset(bin_data,0,sizeof(bin_data));
    
    if (BN_bn2bin(m_private_bn, &bin_data[32-len]) != len)
    {
        xassert(0);
        return std::string(); //error
    }
    return std::string((const char*)bin_data,32);
}

