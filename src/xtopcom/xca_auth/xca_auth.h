// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include <string>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <memory>

NS_BEG2(top, ca_auth)


class xtop_ca_auth
{
public:
xtop_ca_auth(std::string rot_path, std::string self_path);
~xtop_ca_auth();


public:
    int  init_root_cert();
    int  init_self_cert();
    int  read_self_cert_config(){ return false;}; //todo

    int verify_self_cert();
    int verify_root_cert() { return false;}; //todo
    int verify_other_cert(std::string cert_buff){ return false;}; //todo

private:
     bool pem_to_der(unsigned char *out, long *out_len, const char *pem);
     X509* load_cert_from_file(std::string cert_file);

private:
    
    X509_STORE *m_ca_store{NULL};
    X509_STORE_CTX *m_ca_store_ctx{NULL};


    X509 *m_root_cert{NULL};
    X509 *m_self_cert{NULL};
    X509 *m_other_cert{NULL};

    std::string  m_root_file;
    std::string  m_self_file;
    

};



NS_END2


