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

class xtop_ca_auth {
public:
    xtop_ca_auth(std::string self_path);
    xtop_ca_auth();
    ~xtop_ca_auth();

public:
    int add_root_cert(const char* cert_buff, int type =1);
    int init_self_cert(int type);
    int init_self_cert(const char* cert_buff,int type=1);
    int verify_self_cert();
    int verify_other_cert(const char* other_buffer, int type =1);

private:
    int verify_cert(X509* leaf_cert);
    
    X509* load_cert_from_file(std::string cert_file, int type);
    X509* load_cert_from_buffer(const char* cert_buff, int type);

private:
    X509_STORE_CTX* m_ca_store_ctx { NULL };
    STACK_OF(X509) * m_root_certs_stack { NULL };
       X509_STORE *m_ca_store{NULL};
    X509* m_self_cert { NULL };

    std::string m_self_file;
};

NS_END2
