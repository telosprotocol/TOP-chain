// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include "xbase/xint.h"

namespace top
{
    namespace utl
    {
        constexpr int UNCOMPRESSED_PUBLICKEY_SIZE = 65;
        constexpr int COMPRESSED_PUBLICKEY_SIZE = 33;
        constexpr uint32_t ETH_ADDRESS_FLAG = 1 << 7;

        class xecdsasig_t; //forward declare
        class xecpubkey_t; //forward declare

        //manage crypto PrivateKey/PublicKey/Signature/Verify
        class xckey_t
        {
        protected:
            xckey_t(){};
            virtual ~xckey_t(){};
        private:
            xckey_t(const xckey_t &);
            xckey_t & operator = (const xckey_t &);
        };

        //ECC-secp256k1 key
        class xsecp256k1_t : public xckey_t
        {
        public:
            //note:verify_publickey is used to check whether signature is signed by the private key related with verify_publickey
            //return true when verify successful,
            static bool           verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest,uint8_t verify_publickey[65], bool compress = false);
            //retreive public key from signature
            static bool           get_publickey_from_signature(xecdsasig_t & signature,const uint256_t & msg_digest,uint8_t out_publickey_data[65]);
        protected:
            xsecp256k1_t();
            virtual ~xsecp256k1_t(){};
        private:
            xsecp256k1_t(const xsecp256k1_t &);
            xsecp256k1_t & operator = (const xsecp256k1_t &);
        protected:
            static void*    static_secp256k1_context_sign;
            static void*    static_secp256k1_context_verify;
        };

        //ecdsa signature
        class xecdsasig_t
        {
        public:
            xecdsasig_t(const uint8_t compact_signature[65])
            {
                memcpy(signature_data, compact_signature, 65);
            }
            xecdsasig_t(const uint8_t raw_signature[64],const uint8_t input_recover_id)
            {
                signature_data[0] = input_recover_id;
                memcpy(signature_data+1, raw_signature, 64);
            }
            xecdsasig_t(const xecdsasig_t & obj)
            {
                *this = obj;
            }
            xecdsasig_t & operator = (const xecdsasig_t & obj)
            {
                memcpy(signature_data, obj.signature_data, sizeof(signature_data));
                return *this;
            }
            ~xecdsasig_t(){};
        private:
            xecdsasig_t();
        public:
            uint8_t*   get_compact_signature() {return (signature_data);}
            int        get_compact_signature_size() const {return 65;}

            uint8_t*   get_raw_signature() {return (signature_data + 1);}
            int        get_raw_signature_size() const {return 64;}
            uint8_t    get_recover_id() const {return signature_data[0];}
        private:
            uint8_t    signature_data[65]; //first byte reserved for recover id
        };

        //account address generated from public key(secp256k1 curve)
        class xkeyaddress_t : public xsecp256k1_t
        {
        public:
            explicit xkeyaddress_t(const std::string & account_address);
            xkeyaddress_t(const xkeyaddress_t & obj);
            xkeyaddress_t & operator = (const xkeyaddress_t & obj);
            ~xkeyaddress_t(){};
        private:
            xkeyaddress_t();
        public:
            bool         verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest); //for normal account address
            bool         verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest,const std::string & parent_addr);//for child account address

            bool         verify_signature(
                            xecdsasig_t & signature,
                            const uint256_t & msg_digest,
                            uint8_t out_publickey_data[65]); //for normal account address
            bool         verify_signature(
                            xecdsasig_t & signature,
                            const uint256_t & msg_digest,
                            const std::string & parent_addr,
                            uint8_t out_publickey_data[65]);//for child account address
            bool         get_type_and_netid(uint8_t & addr_type,uint16_t & net_id);
            bool         get_type(uint8_t& addr_type);
            bool         is_valid();
            bool         is_eth_valid();
        private:
            std::string  m_account_address;//m_account_address = xecpubkey_t.to_address(m_parent_address,addr_type,net_id);
        };

        //ecc public key(1byte[type] + 64bytes raw public key) based on secp256k1 curve
        /*
         * 65-byte (if uncompressed) or 33-byte (if compressed = [sign type][32byte data]) byte for public key
         * we always use SECP256K1_EC_UNCOMPRESSED
         *
         */
        class xecpubkey_t : public xsecp256k1_t
        {
        public:
            xecpubkey_t(const uint512_t & pubkey)//raw public key is 64byte(512bits) without type(0x04)
            {
                m_publickey_data[0] = 0x04; //0x04 means uncompressed public key
                memcpy(m_publickey_data + 1, pubkey.data(), pubkey.size());
            }

            xecpubkey_t(const uint8_t pubkey[65])
            {
                memcpy(m_publickey_data, pubkey, 65);
            }
            
            xecpubkey_t(const std::string pub_key_data); //it support compressed/uncompressed key
            xecpubkey_t(const uint8_t * pubkey_ptr,const int32_t pubkey_len);//it support compressed/uncompressed key

            xecpubkey_t(const xecpubkey_t & obj)
            {
                *this = obj;
            }
            xecpubkey_t & operator = (const xecpubkey_t & obj)
            {
                memcpy(m_publickey_data, obj.m_publickey_data, sizeof(m_publickey_data));
                return *this;
            }
            ~xecpubkey_t(){};
        private:
            xecpubkey_t();
        public:
            inline uint8_t*   data(){return m_publickey_data;}
            inline int        size() const {return 65;}
            bool              is_valid() const;
            bool              verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest, bool compress = false);
        public: //following can only be used at TOP
            uint256_t         to_hash();                                //convert public key to hash by sha256+ripemd160
            std::string       to_address(const char addr_type,const uint16_t ledger_id);   //convert public key to string as based58 encode
            //child address is generated by (public key +  parent_addr)
            std::string       to_address(const std::string & parent_addr,const char addr_type,const uint16_t ledger_id);
        protected:
            std::string       to_address(const uint8_t* publickey, const char addr_type,const uint16_t ledger_id);
        private:
            std::string       to_eth_address(const uint8_t* publickey, const char addr_type,const uint16_t ledger_id);
            void              init(const uint8_t * pubkey_ptr,const int32_t pubkey_len);
        private:
            uint8_t           m_publickey_data[65]; //first byte for type
        };

        //ecc private key(must be 256bit) based on secp256k1 curve
        class xecprikey_t : public xsecp256k1_t
        {
        public:
            xecprikey_t(); //sha256(32bytes random)->private key
            xecprikey_t(const std::string rand_seed);//sha256(rand_seed.32bytes random)->private key
            xecprikey_t(const uint256_t & prikey)
            {
                memcpy(m_private_key, prikey.data(), prikey.size());
                generate_public_key();
            }
            xecprikey_t(uint8_t prikey[32])
            {
                memcpy(m_private_key, prikey, 32);
                generate_public_key();
            }
            xecprikey_t(const xecprikey_t & obj)
            {
                *this = obj;
            }
            xecprikey_t & operator = (const xecprikey_t & obj)
            {
                memcpy(m_private_key, obj.m_private_key, sizeof(m_private_key));
                generate_public_key();
                return *this;
            }
            ~xecprikey_t(){};
        public:
            inline uint8_t*        data(){return m_private_key;}
            inline int             size() const {return 32;}
            xecpubkey_t            get_public_key(); //generate related public key
            std::string            get_compress_public_key(); // get related compress public key string

            std::string            to_account_address(const char addr_type,const uint16_t ledger_id); //build account address based on pub key
        public: //signature
            xecdsasig_t     sign(const uint256_t & msg_digest) const;
            bool            verify_signature(xecdsasig_t & signature,const uint256_t & msg_digest);
        protected:
            void            generate_public_key(); //generate related public key
        private:
            uint8_t         m_private_key[32];   //ecc private key is 256bit
            uint8_t         m_publickey_key[65]; //first byte for type
        };
    }
} //end of namespace top

