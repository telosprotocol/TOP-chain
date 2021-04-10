// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvledger/xvcertauth.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvblockstore.h"

namespace top
{
    namespace store
    {
        //reserved for sync module as a cache layer with function of persist storage
        class xsyncvstore_t : public base::xobject_t
        {
        public:
            xsyncvstore_t(base::xvcertauth_t & _certauth,base::xvblockstore_t & _xdb);
        protected:
            virtual ~xsyncvstore_t();
        private:
            xsyncvstore_t();
            xsyncvstore_t(const xsyncvstore_t &);
            xsyncvstore_t & operator = (const xsyncvstore_t &);
            
        public:
            inline base::xvcertauth_t*     get_vcertauth()   const {return m_vcertauth_ptr;}
            inline base::xvblockstore_t*   get_vblockstore() const {return m_vblockstore_ptr;}
            
        public://mutipl-threads safe
            virtual bool    store_block(base::xvblock_t* target_block);  //cache and hold block
            virtual bool    store_block(base::xvaccount_t & target_account,base::xvblock_t* target_block);//better performance if have account object
            
        private://below are accessed by muliple threads
            base::xvcertauth_t  *           m_vcertauth_ptr;  //point to related certauth
            base::xvblockstore_t*           m_vblockstore_ptr;//point to related blockstore
        };
    };//end of namespace of store
};//end of namespace of top
