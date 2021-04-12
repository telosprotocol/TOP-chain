// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvcnode.h"
#include "xvledger/xvcertauth.h"
#include "xbase/xobject_ptr.h"

namespace top
{
    namespace auth
    {
        class xauthcontext_t : public base::xvcertauth_t
        {
        public:
            //use global instance even for case of simulation of mutiple nodes
            static base::xvcertauth_t&  instance(base::xvnodesrv_t & node_service);

            //create new object even for case of simulation of mutiple nodes
            static xobject_ptr_t<base::xvcertauth_t>  create(base::xvnodesrv_t & node_service);

            static std::vector<base::xvoter>  query_validators(base::xvblock_t & block,base::xvnodesrv_t & node_service);
            static std::vector<base::xvoter>  query_auditors(base::xvblock_t & block,base::xvnodesrv_t & node_service);
            
        protected: //used to debug purpose and not open for public access,so let ckey.h/cpp is the only way to generate private key and public key of ECC.
            //return pair of <private-key,public-key>
            //private-key must = 32bytes of raw private key of ECC(secp256k1)
            //public-key  must = 33 bytes of the compressed public key of ECC(secp256k1)
            static std::pair<std::string, std::string>  create_secp256k1_keypair(const uint64_t random_seed); //return empty if fail
        protected:
            xauthcontext_t();
            virtual ~xauthcontext_t();
        private:
            xauthcontext_t(const xauthcontext_t &);
            xauthcontext_t & operator = (const xauthcontext_t &);
        };
    };
};
