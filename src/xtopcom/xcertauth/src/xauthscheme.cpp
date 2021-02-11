// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xauthscheme.h"
#include "xschnorrsig.h"
#include "xblssig.h"

namespace top
{
    namespace auth
    {
        xauthscheme_t*  xauthscheme_t::create_auth_scheme(base::enum_xvchain_sign_scheme scheme)
        {
            if(base::enum_xvchain_threshold_sign_scheme_schnorr == scheme)
                return new xschnorrsig_t();
            
            return NULL;
        }
    
        xauthscheme_t::xauthscheme_t()
        {
        }
        xauthscheme_t::~xauthscheme_t()
        {
        }
        
    }; //end of namespace of auth
};//end of namesapce of top
