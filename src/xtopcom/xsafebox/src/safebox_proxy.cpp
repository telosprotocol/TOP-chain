// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsafebox/safebox_proxy.h"

#include "openssl/bn.h"
#include "openssl/ec.h"
#include "openssl/obj_mac.h"
#include "openssl/sha.h"
#include "xcrypto/xckey.h"

#include <cassert>
#include <string>
#include <vector>

NS_BEG2(top, safebox)

static std::string hash256(std::string const & message) {
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, (const void *)(message.c_str()), message.size());
    std::vector<uint8_t> output(32);
    SHA256_Final(output.data(), &sha256_ctx);
    return std::string((const char *)output.data(), output.size());
}

class xsafebox_curve {
public:
    xsafebox_curve() {
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
    ~xsafebox_curve() {
        BN_free(m_bn_order);
        EC_GROUP_clear_free(m_ec_group);
    }

    EC_GROUP * ec_group() const {
        return m_ec_group;
    }
    BIGNUM * bn_order() const {
        return m_bn_order;
    }

private:
    // Ellipse Curve Cryptography
    EC_GROUP * m_ec_group{nullptr};
    // order of ec group
    BIGNUM * m_bn_order{nullptr};
};

class xsafebox_schnor {
private:
    xsafebox_curve m_curve;

    xsafebox_schnor() = default;

public:
    static xsafebox_schnor & get_instance() {
        static xsafebox_schnor s;
        return s;
    }

    // generate_nonzero_bn
    //    -> BIGNUM
    BIGNUM * generate_rand_bn() {
        BIGNUM * new_bn = BN_new();
        assert(nullptr != new_bn);
        bool err = false;
        do {
            err = (BN_rand_range(new_bn, m_curve.bn_order()) == 0);
            if (err) {
                break;
            }
        } while (BN_is_zero(new_bn));
        assert(!BN_is_zero(new_bn));
        return new_bn;
    }

    // generate point with certain bn
    //    -> EC_POINT
    EC_POINT * generate_point_with_bn(BIGNUM const * const bn) {
        EC_POINT * new_ec_point = EC_POINT_new(m_curve.ec_group());
        assert(nullptr != new_ec_point);
        assert(!BN_is_zero(bn));
        assert((BN_cmp(bn, m_curve.bn_order()) == -1));

        // bn_face is "k" and new_ec_point is "R", here R = k * G
        // or bn_face is "private_key" and new_ec_point is public Key,so  P = pk * G
#if !defined(NDEBUG)
        XATTRIBUTE_MAYBE_UNUSED int32_t ret =
#endif
        EC_POINT_mul(m_curve.ec_group(), new_ec_point, bn, nullptr, nullptr, nullptr);
        assert(0 != ret);
        return new_ec_point;
    }

    // generate message object's bignum
    // msg string -> BIGNUM
    BIGNUM * generate_message_bn(std::string const & message) {
        assert(!message.empty());
        if (message.empty())
            return nullptr;

        std::string hash_str = hash256(message);

        // BN_bin2bn() converts the positive integer in big-endian form of length len at s into a BIGNUM and places it in ret. If ret is NULL, a new BIGNUM is created.
        // The error codes can be obtained by ERR_get_error
        BIGNUM * new_bn = BN_bin2bn((unsigned char *)hash_str.c_str(), (int)hash_str.size(), NULL);

        BIGNUM * r = new_bn;
        BIGNUM * m = r;
        BN_CTX * ctx = BN_CTX_new();
#if !defined(NDEBUG)
        XATTRIBUTE_MAYBE_UNUSED int ret =
#endif
        BN_nnmod(r, m, m_curve.bn_order(), ctx);
        assert(0 != ret);
        BN_CTX_free(ctx);

        return new_bn;
    }

    // serialize BIGNUM to str
    // BIGNUM -> string
    std::string BN_serialize(BIGNUM const * const bn) {
        assert(nullptr != bn);
        std::string bn_str;

        int len = BN_num_bytes(bn);

        std::vector<uint8_t> arr;
        arr.resize(len);
        fill(arr.begin(), arr.begin() + len, 0x00);

        if (BN_bn2bin(bn, arr.data()) != len) {
            xassert(false);
            return "";  // error
        }

        bn_str.insert(bn_str.begin(), arr.begin(), arr.end());

        return bn_str;
    }

    // serialize BIGNUM to str ( used for private key only)
    // BIGNUM -> string
    std::string pri_BN_to_fixed_string(BIGNUM const * const bn) {
        assert(nullptr != bn);
        const int len = BN_num_bytes(bn);
        xassert(len <= 32);

        uint8_t bin_data[32];
        memset(bin_data, 0, sizeof(bin_data));

        if (BN_bn2bin(bn, &bin_data[32 - len]) != len) {
            xassert(false);
            return std::string();  // error
        }
        return std::string((const char *)bin_data, 32);
    }

    // serialize EC_POINT to str
    // EC_POINT -> string
    std::string EC_POINT_serialize(EC_POINT * ec_point) {
        assert(nullptr != ec_point);
        std::string point_str;

        BN_CTX * bn_ctx = BN_CTX_new();
        assert(nullptr != bn_ctx);

        BIGNUM * bn = EC_POINT_point2bn(m_curve.ec_group(), ec_point, POINT_CONVERSION_COMPRESSED, NULL, bn_ctx);

        assert(nullptr != bn);

        point_str = BN_serialize(bn);
        BN_free(bn);
        BN_CTX_free(bn_ctx);
        return point_str;
    }

    BIGNUM * curve_bn_order() {
        return m_curve.bn_order();
    }
};

// safebox private key
class xsafebox_private_key {
private:
    BIGNUM * m_data;

public:
    xsafebox_private_key() = delete;
    xsafebox_private_key(xsafebox_private_key const &) = delete;
    xsafebox_private_key & operator=(xsafebox_private_key const &) = delete;
    xsafebox_private_key(xsafebox_private_key &&) = default;
    xsafebox_private_key & operator=(xsafebox_private_key &&) = default;
    ~xsafebox_private_key() {
        BN_free(m_data);
    }

    xsafebox_private_key(std::string && sign_key) {
        assert(sign_key.size() != 0);
        m_data = BN_bin2bn((unsigned char *)sign_key.c_str(), (int)sign_key.size(), nullptr);
    }

    BIGNUM const * const prikey_key() const {
        return m_data;
    }
};

// safebox signature
class xsafebox_signature {
private:
    BIGNUM * m_data;
    EC_POINT * m_point;  // rand_data->point src/xtopcom/xmutisig/src/xecpoint.cpp:12

public:
    xsafebox_signature() = delete;
    xsafebox_signature(xsafebox_signature const &) = delete;
    xsafebox_signature & operator=(xsafebox_signature const &) = delete;
    xsafebox_signature(xsafebox_signature &&) = default;
    xsafebox_signature & operator=(xsafebox_signature &&) = default;
    ~xsafebox_signature() {
        EC_POINT_free(m_point);
        BN_free(m_data);
    };

    xsafebox_signature(BIGNUM * object, BIGNUM const * const prikey) {
        BIGNUM * rand{nullptr};
        do {
            rand = xsafebox_schnor::get_instance().generate_rand_bn();

            m_data = xsafebox_schnor::get_instance().generate_rand_bn();
            BIGNUM * curve_bn_order = xsafebox_schnor::get_instance().curve_bn_order();
            std::unique_ptr<BN_CTX, void (*)(BN_CTX *)> ctx(BN_CTX_new(), BN_CTX_free);
            assert(nullptr != ctx);
#ifdef DEBUG
            assert(!BN_is_zero(prikey));
            assert((BN_cmp(prikey, curve_bn_order) == -1));
#endif
            // object * prikey
            // r=(a*b) mod m ---> m_data(signature) = (object * prikey) % bn_order
            int ret = BN_mod_mul(m_data, object, prikey, curve_bn_order, ctx.get());

            xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_mul failed.
            // r = |(a - b)| % m
            ret = BN_mod_sub(m_data, rand, m_data, curve_bn_order, ctx.get());
            xassert(0 != ret);  // TODO: Jimmy fix it when BN_mod_sub failed.

        } while (BN_is_zero(m_data));
        m_point = xsafebox_schnor::get_instance().generate_point_with_bn(rand);
        BN_free(rand);
        BN_free(object);
    }

    std::pair<std::string, std::string> output_data() {
        return std::make_pair(xsafebox_schnor::get_instance().EC_POINT_serialize(m_point), xsafebox_schnor::get_instance().BN_serialize(m_data));
    }
};

xsafebox_proxy & xsafebox_proxy::get_instance() {
    static xsafebox_proxy p;
    return p;
}

void xsafebox_proxy::add_key_pair(top::xpublic_key_t const & public_key, std::string && sign_key) {
    m_key_map.insert(std::make_pair(public_key.to_string(), top::make_unique<xsafebox_private_key>(std::move(sign_key))));
}

std::pair<std::string, std::string> xsafebox_proxy::get_proxy_signature(std::string const & public_key, std::string const & msg) {
    if (m_key_map.find(public_key) == m_key_map.end()) {
        assert(false);
        return std::make_pair("", "");
    }
    auto signature = xsafebox_signature(xsafebox_schnor::get_instance().generate_message_bn(msg), m_key_map.at(public_key)->prikey_key());
    return signature.output_data();
}

std::string xsafebox_proxy::get_proxy_secp256_signature(std::string const & public_key, top::uint256_t const & hash) {
    if (m_key_map.find(public_key) == m_key_map.end()) {
        assert(false);
        return "";
    }

    auto prikey_str = xsafebox_schnor::get_instance().pri_BN_to_fixed_string(m_key_map.at(public_key)->prikey_key());
    uint8_t priv_content[32];
    memcpy(priv_content, prikey_str.data(), prikey_str.size());
    top::utl::xecprikey_t ecpriv(priv_content);

    auto signature = ecpriv.sign(hash);
    std::string signature_str = std::string((char *)signature.get_compact_signature(), signature.get_compact_signature_size());
    return signature_str;
}

NS_END2
