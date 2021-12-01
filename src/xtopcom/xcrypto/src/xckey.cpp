// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <time.h>
#include <chrono>
#include "xbase/xutl.h"
#include "xcrypto/xckey.h"
#include "xutility/xhash.h"
#include "xvledger/xvaccount.h"
#include "xmetrics/xmetrics.h"

extern "C"
{
    #include "trezor-crypto/rand.h"
    #include "trezor-crypto/secp256k1.h"
    #include "trezor-crypto/ecdsa.h"
}
#include "secp256k1/secp256k1.h"
#include "secp256k1/secp256k1_recovery.h"

namespace top
{
    namespace utl
    {
        static int  split_string_impl(const std::string & input,const char split_char,std::vector<std::string> & values)
        {
            if(input.empty())
                return 0;

            std::string::size_type begin_pos = 0;
            std::string::size_type pos_of_split = input.find_first_of(split_char,begin_pos);
            while(pos_of_split != std::string::npos)
            {
                if(pos_of_split != begin_pos)
                    values.push_back(input.substr(begin_pos,pos_of_split - begin_pos)); //[)
                begin_pos = pos_of_split + 1; //skip boundary
                pos_of_split = input.find_first_of(split_char,begin_pos);
                if(pos_of_split == std::string::npos) //not find the last split-char
                {
                    if(begin_pos < input.size())
                    {
                        values.push_back(input.substr(begin_pos)); //put the remaining section
                    }
                }
            }
            if(values.empty())
                values.push_back(input);

            return (int)values.size();
        }

        void*    xsecp256k1_t::static_secp256k1_context_sign = NULL;
        void*    xsecp256k1_t::static_secp256k1_context_verify = NULL;
        xsecp256k1_t::xsecp256k1_t()
        {
            if(NULL == static_secp256k1_context_sign)
            {
                static_secp256k1_context_sign = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
                xassert(static_secp256k1_context_sign != NULL);
            }

            if(NULL == static_secp256k1_context_verify)
            {
                static_secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
                xassert(static_secp256k1_context_verify != NULL);
            }
        }

        //return true when verify successful
        bool           xsecp256k1_t::verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest,uint8_t verify_publickey[65], bool compress)
        {
            secp256k1_ecdsa_recoverable_signature recover_sigature;
            if(signature.get_recover_id() > 3)  // the recovery id (0, 1, 2 or 3)
            {
                return false;
            }

            if(secp256k1_ecdsa_recoverable_signature_parse_compact((secp256k1_context*)static_secp256k1_context_verify, &recover_sigature, signature.get_raw_signature(), signature.get_recover_id()) != 1)
            {
                return false;
            }

            secp256k1_pubkey native_pubkey;
            if(secp256k1_ecdsa_recover((secp256k1_context*)static_secp256k1_context_verify, &native_pubkey, &recover_sigature, msg_digest.data()) != 1)
            {
                return false;
            }

            secp256k1_ecdsa_signature normal_signature;
            if(secp256k1_ecdsa_recoverable_signature_convert((secp256k1_context*)static_secp256k1_context_verify,&normal_signature,&recover_sigature) != 1)
            {
                return false;
            }

            if(secp256k1_ecdsa_verify((secp256k1_context*)static_secp256k1_context_verify, &normal_signature, msg_digest.data(), &native_pubkey) != 1)
            {
                return false;
            }

            /*
             * a pointer to a 65-byte (if compressed==0) or 33-byte (if compressed==1) byte array to place the serialized key
             * we always use SECP256K1_EC_UNCOMPRESSED
             */
            size_t  serialize_pubkey_size = 65;
            uint8_t serialize_pubkey_data[65] = {0};
            if (compress) {
                const int ret = secp256k1_ec_pubkey_serialize((secp256k1_context*)static_secp256k1_context_sign, serialize_pubkey_data, &serialize_pubkey_size, &native_pubkey, SECP256K1_EC_COMPRESSED);
                xassert(ret == 1);
                xassert(serialize_pubkey_size == 33);
            }else {
                const int ret = secp256k1_ec_pubkey_serialize((secp256k1_context*)static_secp256k1_context_sign, serialize_pubkey_data, &serialize_pubkey_size, &native_pubkey, SECP256K1_EC_UNCOMPRESSED);
                xassert(ret == 1);
                xassert(serialize_pubkey_size == 65);
            }

            if(memcmp(verify_publickey,serialize_pubkey_data,serialize_pubkey_size) == 0)
            {
                return true;
            }
            return false;
        }

        bool    xsecp256k1_t::get_publickey_from_signature(xecdsasig_t & signature,const uint256_t & msg_digest,uint8_t out_publickey_data[65])
        {
            secp256k1_ecdsa_recoverable_signature recover_sigature;
            if(signature.get_recover_id() > 3)  // the recovery id (0, 1, 2 or 3)
            {
                return false;
            }

            if(secp256k1_ecdsa_recoverable_signature_parse_compact((secp256k1_context*)static_secp256k1_context_verify, &recover_sigature, signature.get_raw_signature(), signature.get_recover_id()) != 1)
            {
                return false;
            }

            secp256k1_pubkey native_pubkey;
            if(secp256k1_ecdsa_recover((secp256k1_context*)static_secp256k1_context_verify, &native_pubkey, &recover_sigature, msg_digest.data()) != 1)
            {
                return false;
            }

            secp256k1_ecdsa_signature normal_signature;
            if(secp256k1_ecdsa_recoverable_signature_convert((secp256k1_context*)static_secp256k1_context_verify,&normal_signature,&recover_sigature) != 1)
            {
                return false;
            }

            if(secp256k1_ecdsa_verify((secp256k1_context*)static_secp256k1_context_verify, &normal_signature, msg_digest.data(), &native_pubkey) != 1)
            {
                return false;
            }

            /*
             * a pointer to a 65-byte (if compressed==0) or 33-byte (if compressed==1) byte array to place the serialized key
             * we always use SECP256K1_EC_UNCOMPRESSED
             */
            size_t  serialize_pubkey_size = 65;
            uint8_t serialize_pubkey_data[65] = {0};
            const int ret = secp256k1_ec_pubkey_serialize((secp256k1_context*)static_secp256k1_context_sign, serialize_pubkey_data, &serialize_pubkey_size, &native_pubkey,SECP256K1_EC_UNCOMPRESSED);
            xassert(ret == 1);
            xassert(serialize_pubkey_size == 65);

            memcpy(out_publickey_data,serialize_pubkey_data,serialize_pubkey_size);
            return true;
        }

        xkeyaddress_t::xkeyaddress_t(const std::string & account_address)
        {
            m_account_address = account_address;
        }

        xkeyaddress_t::xkeyaddress_t(const xkeyaddress_t & obj)
        {
            *this = obj;
        }

        xkeyaddress_t & xkeyaddress_t::operator = (const xkeyaddress_t & obj)
        {
            m_account_address = obj.m_account_address;
            return *this;
        }

        bool   xkeyaddress_t::get_type_and_netid(uint8_t & addr_type,uint16_t & net_id)
        {
            return base::xvaccount_t::get_type_and_ledgerid_from_account(addr_type,net_id,m_account_address);
        }

        bool  xkeyaddress_t::is_valid()
        {
            uint8_t addr_type;
            uint16_t ledger_id;
            bool ret = base::xvaccount_t::get_type_and_ledgerid_from_account(addr_type, ledger_id, m_account_address);
            if (ret) {
                if (addr_type == base::enum_vaccount_addr_type_secp256k1_eth_user_account)
                    return is_eth_valid();                
                std::string public_address;
                ret =  base::xvaccount_t::get_public_address_from_account(m_account_address, public_address);
                if (ret) {
                    uint32_t version = (((uint32_t)ledger_id) << 8) | ((uint32_t)addr_type);
                    uint8_t out[64];
                    try {
                        return ecdsa_address_decode(public_address.c_str(), version, HASHER_SHA2D, out) == 1;
                    } catch (...) {
                        return false;
                    }
                }
            }
            return ret;
        }
        bool  xkeyaddress_t::is_eth_valid()
        {
            std::string public_address;
            bool ret = base::xvaccount_t::get_public_address_from_account(m_account_address, public_address);
            if (ret)
            {
                if (public_address.size() == 40)
                    return true;
                else
                    return false;
            }
            return ret;
        }
        bool   xkeyaddress_t::get_type(uint8_t& addr_type) {
            uint16_t net_id;
            return get_type_and_netid(addr_type, net_id);
        }

        bool   xkeyaddress_t::verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest)
        {
            uint8_t  addr_type = 0;
            uint16_t net_id = 0;
            if(false == get_type_and_netid(addr_type,net_id))
                return false;

            uint8_t out_publickey_data[65] = {0};
            if(xsecp256k1_t::get_publickey_from_signature(signature,msg_digest,out_publickey_data))//signature is valid
            {
                xecpubkey_t verify_key(out_publickey_data);
                if(verify_key.to_address(addr_type, net_id) == m_account_address)//then check whether is from this address
                {
                    return true;
                }
            }
            return false;
        }

        bool   xkeyaddress_t::verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest,const std::string & parent_addr)
        {
            uint8_t  addr_type = 0;
            uint16_t net_id = 0;
            if(false == get_type_and_netid(addr_type,net_id))
                return false;

            uint8_t out_publickey_data[65] = {0};
            if(xsecp256k1_t::get_publickey_from_signature(signature,msg_digest,out_publickey_data))//signature is valid
            {
                xecpubkey_t verify_key(out_publickey_data);
                if(verify_key.to_address(parent_addr,addr_type, net_id) == m_account_address)//then check whether is from this address
                {
                    return true;
                }
            }
            return false;
        }

        bool   xkeyaddress_t::verify_signature(
                    xecdsasig_t & signature,
                    const uint256_t & msg_digest,
                    uint8_t out_publickey_data[65])
        {
            uint8_t  addr_type = 0;
            uint16_t net_id = 0;
            if(false == get_type_and_netid(addr_type,net_id))
                return false;
            if(xsecp256k1_t::verify_signature(signature,msg_digest,out_publickey_data))
            {
                xecpubkey_t verify_key(out_publickey_data);
                if(verify_key.to_address(addr_type, net_id) == m_account_address)//then check whether is from this address
                {
                    return true;
                }
            }
            return false;
        }

        bool   xkeyaddress_t::verify_signature(
                    xecdsasig_t & signature,
                    const uint256_t & msg_digest,
                    const std::string & parent_addr,
                    uint8_t out_publickey_data[65])
        {
            uint8_t  addr_type = 0;
            uint16_t net_id = 0;
            if(false == get_type_and_netid(addr_type,net_id))
                return false;
            if(xsecp256k1_t::verify_signature(signature,msg_digest,out_publickey_data))
            {
                xecpubkey_t verify_key(out_publickey_data);
                if(verify_key.to_address(parent_addr,addr_type, net_id) == m_account_address)//then check whether is from this address
                {
                    return true;
                }
            }
            return false;
        }
    
        xecpubkey_t::xecpubkey_t(const std::string pub_key_data)//it support compressed/uncompressed key
        {
            init((const uint8_t *)pub_key_data.data(),(int32_t)pub_key_data.size());
        }
    
        xecpubkey_t::xecpubkey_t(const uint8_t * pubkey_ptr,const int32_t pubkey_len)//it support compressed/uncompressed key
        {
            init(pubkey_ptr,pubkey_len);
        }
    
        void xecpubkey_t::init(const uint8_t * pubkey_ptr,const int32_t pubkey_len)//it support convert compressed key to uncompressed
        {
            memset(m_publickey_data, 0, sizeof(m_publickey_data));//reset first
            xassert(pubkey_ptr != NULL);
            if(NULL != pubkey_ptr)
            {
                if(pubkey_len == 65) //un-compressed public key with type
                {
                    memcpy(m_publickey_data, pubkey_ptr, pubkey_len);
                    xassert(0x04 == m_publickey_data[0]);
                }
                else if(pubkey_len == 64) //raw public key is 64byte(512bits) without type(0x04)
                {
                    m_publickey_data[0] = 0x04; //0x04 means uncompressed public key
                    memcpy(m_publickey_data + 1, pubkey_ptr,pubkey_len);
                }
                else if(pubkey_len == 33) //compressed public key
                {
                    size_t  serialize_pubkey_size = 65;
                    uint8_t serialize_pubkey_data[65] = {0};
                    
                    secp256k1_pubkey  secppubkey;
                    int ret = secp256k1_ec_pubkey_parse((secp256k1_context*)static_secp256k1_context_sign, &secppubkey, pubkey_ptr,pubkey_len);//parse compressed public key
                    xassert(1 == ret);
                    if(1 == ret)
                    {
                        ret = secp256k1_ec_pubkey_serialize((secp256k1_context*)static_secp256k1_context_sign, serialize_pubkey_data, &serialize_pubkey_size, &secppubkey,SECP256K1_EC_UNCOMPRESSED);
                        xassert(1 == ret);//convert to uncompressed public key
                        xassert(serialize_pubkey_size == 65);
                        memcpy(m_publickey_data,serialize_pubkey_data,serialize_pubkey_size);
                    }
                }
                else
                {
                    xassert(0);
                }
            }
        }

        uint256_t         xecpubkey_t::to_hash()       //convert public key to hash by sha256+ripemd160
        {
            uint256_t pub_hash;
            ecdsa_get_pubkeyhash(m_publickey_data, HASHER_SHA2_RIPEMD, pub_hash.data());
            return pub_hash;
        }

        std::string       xecpubkey_t::to_address(const char addr_type,const uint16_t ledger_id)       //convert public key to string as based58 encode
        {
            return to_address(m_publickey_data, addr_type, ledger_id);
        }

        //child address is generated by (public key +  parent_addr)
        std::string      xecpubkey_t::to_address(const std::string & parent_addr,const char addr_type,const uint16_t ledger_id)
        {
            if(parent_addr.empty())
                return to_address(addr_type,ledger_id);

            uint8_t     temp_publickey_data[65];
            memcpy(temp_publickey_data, m_publickey_data, sizeof(temp_publickey_data));
            const int parent_addr_size = std::min((int)parent_addr.size(),65);
            for(int i = 0; i < parent_addr_size; ++i)
            {
                temp_publickey_data[i] += parent_addr[i];
            }

            return to_address(temp_publickey_data, addr_type, ledger_id);
        }

        std::string       xecpubkey_t::to_address(const uint8_t* publickey, const char addr_type,const uint16_t ledger_id)
        {
            if(addr_type == base::enum_vaccount_addr_type_secp256k1_eth_user_account)
                return to_eth_address(publickey,addr_type,ledger_id);
                
            char address[128] = {0};
            const uint32_t version_uint32 = (((uint32_t)ledger_id) << 8) | ((uint32_t)addr_type);
            ecdsa_get_address(publickey, version_uint32, HASHER_SHA2_RIPEMD, HASHER_SHA2D, address, sizeof(address));
            const std::string pubkey_sub_addr(address);

            return base::xvaccount_t::make_account_address((base::enum_vaccount_addr_type)addr_type, ledger_id, pubkey_sub_addr,-1);
        }

        std::string       xecpubkey_t::to_eth_address(const uint8_t* publickey, const char addr_type,const uint16_t ledger_id)
        {
            xassert(addr_type == base::enum_vaccount_addr_type_secp256k1_eth_user_account);
            
            const uint256_t hash_value = xkeccak256_t::digest(publickey + 1, size() - 1);//remove frist byte of type from public key
            const std::string raw_eth_address((const char *)hash_value.data() + 12, hash_value.size() - 12);//drop first 12 bytes of total 32,as Ethereum just use the last 20 bytes of hash(keccak256)
            const std::string hex_eth_address = base::xstring_utl::to_hex(raw_eth_address);//convert to Hex codec
            
            return base::xvaccount_t::make_account_address((base::enum_vaccount_addr_type)addr_type, ledger_id, hex_eth_address,-1);
        }

        bool     xecpubkey_t::verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest, bool compress)
        {
            if(xsecp256k1_t::verify_signature(signature,msg_digest,m_publickey_data, compress))
            {
                return true;
            }
            return false;
        }

        static uint32_t xrandom32()
        {
            //get_sys_random_number might be replaced by std::random_device without xbase lib
            const uint64_t seed = base::xsys_utl::get_sys_random_number() + base::xtime_utl::get_fast_random();
            return (uint32_t)(seed >> 8);
        }

        static void xrandom_buffer(uint8_t *buf, size_t len)
        {
            uint32_t r = 0;
            for (size_t i = 0; i < len; i++) {
                if (i % 4 == 0) {
                    r = xrandom32();
                }
                buf[i] = (r >> ((i % 4) * 8)) & 0xFF;
            }
        }


        xecprikey_t::xecprikey_t()  //sha256(32bytes random)->private key
        {
            memset(m_publickey_key,0,sizeof(m_publickey_key));

            xrandom_buffer(m_private_key,sizeof(m_private_key));
            uint256_t   hash_value;
            xsha2_256_t hasher;

            auto now = std::chrono::system_clock::now();
            auto now_nano = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
            int64_t time_seed = now_nano.time_since_epoch().count();
            hasher.update(&time_seed,sizeof(time_seed));
            hasher.update(m_private_key, sizeof(m_private_key));
            hasher.get_hash(hash_value);
            XMETRICS_GAUGE(metrics::cpu_hash_256_xecprikey_calc, 1);

            const int over_size = std::min((int)hash_value.size(),(int)sizeof(m_private_key));
            for(int i = 0; i < over_size; ++i)
            {
                m_private_key[i] += ((uint8_t*)hash_value.data())[i];
            }
            //paired with BN_bin2bn() that converts the positive integer in big-endian from binary
            m_private_key[0]  &= 0x7F; //ensure it is a positve number since treat is big-endiam format for big-number
            m_private_key[31] &= 0x7F; //ensure it is a positve number
            generate_public_key();
        }
        xecprikey_t::xecprikey_t(const std::string rand_seed) //sha256(rand_seed.32bytes random)->private key
        {
            memset(m_publickey_key,0,sizeof(m_publickey_key));

            xrandom_buffer(m_private_key,sizeof(m_private_key));
            uint256_t   hash_value;
            xsha2_256_t hasher;

            auto now = std::chrono::system_clock::now();
            auto now_nano = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
            int64_t time_seed = now_nano.time_since_epoch().count();
            hasher.update(&time_seed,sizeof(time_seed));
            hasher.update(rand_seed);
            hasher.update(m_private_key, sizeof(m_private_key));
            hasher.get_hash(hash_value);
            XMETRICS_GAUGE(metrics::cpu_hash_256_xecprikey_calc, 1);

            const int over_size = std::min((int)hash_value.size(),(int)sizeof(m_private_key));
            for(int i = 0; i < over_size; ++i)
            {
                m_private_key[i] += ((uint8_t*)hash_value.data())[i];
            }
            m_private_key[0]  &= 0x7F; //ensure it is a positve number
            m_private_key[31] &= 0x7F; //ensure it is a positve number
            generate_public_key();
        }

        /*
         * 65-byte (if uncompressed) or 33-byte (if compressed) byte for public key
         * we always use SECP256K1_EC_UNCOMPRESSED
         */
        void  xecprikey_t::generate_public_key()  //generate related public key
        {
            size_t  serialize_pubkey_size = 65;
            uint8_t serialize_pubkey_data[65] = {0};

            secp256k1_pubkey  secppubkey;
            int ret = secp256k1_ec_pubkey_create((secp256k1_context*)static_secp256k1_context_sign, &secppubkey, m_private_key);
            xassert(1 == ret);
            ret = secp256k1_ec_pubkey_serialize((secp256k1_context*)static_secp256k1_context_sign, serialize_pubkey_data, &serialize_pubkey_size, &secppubkey,SECP256K1_EC_UNCOMPRESSED);
            xassert(1 == ret);
            xassert(serialize_pubkey_size == 65);
            memcpy(m_publickey_key,serialize_pubkey_data,serialize_pubkey_size);
        }

        xecpubkey_t  xecprikey_t::get_public_key()  //generate related public key
        {
            return xecpubkey_t(m_publickey_key);
        }

        std::string  xecprikey_t::get_compress_public_key() {
            size_t  serialize_pubkey_size = 33;
            uint8_t serialize_pubkey_data[33] = { 0 };

            secp256k1_pubkey  secppubkey;
            int ret = secp256k1_ec_pubkey_create((secp256k1_context*)static_secp256k1_context_sign, &secppubkey, m_private_key);
            xassert(1 == ret);
            ret = secp256k1_ec_pubkey_serialize((secp256k1_context*)static_secp256k1_context_sign, serialize_pubkey_data, &serialize_pubkey_size, &secppubkey, SECP256K1_EC_COMPRESSED);
            xassert(1 == ret);
            xassert(serialize_pubkey_size == 33);
            return std::string((char *)serialize_pubkey_data, serialize_pubkey_size);
        }

        std::string   xecprikey_t::to_account_address(const char addr_type,const uint16_t ledger_id) //build account address based on pub key
        {
            return xecpubkey_t(m_publickey_key).to_address(addr_type, ledger_id);
        }

        //signature
        xecdsasig_t  xecprikey_t::sign(const uint256_t & msg_digest) const
        {
            int        recover_id = 0;
            uint8_t    serialize_signature_data[65] = {0};

            secp256k1_ecdsa_recoverable_signature signature;
            int ret = secp256k1_ecdsa_sign_recoverable((secp256k1_context*)static_secp256k1_context_sign, &signature, msg_digest.data(), m_private_key, secp256k1_nonce_function_rfc6979, nullptr);
            xassert(1 == ret);

            ret = secp256k1_ecdsa_recoverable_signature_serialize_compact((secp256k1_context*)static_secp256k1_context_sign, serialize_signature_data + 1, &recover_id, &signature);
            assert(1 == ret);
            serialize_signature_data[0] = recover_id;

            return xecdsasig_t(serialize_signature_data);
        }

        //return true when verify successful
        bool   xecprikey_t::verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest)
        {
            return xsecp256k1_t::verify_signature(signature,msg_digest,m_publickey_key);
        }
    }
} //end of namespace top

