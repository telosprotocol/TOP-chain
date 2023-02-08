// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include <stdint.h>
#include <string.h>


NS_BEG2(top, ca_auth)

class xtop_ca_auth {
public:
    xtop_ca_auth();
    ~xtop_ca_auth();

public:

    /**
     * @brief  
     * @note   
     * @param  cert_buff: 
     * @param  type: 1:X509_FILETYPE_PEM 2:X509_FILETYPE_ASN1
     * @param  len:  if type = X509_FILETYPE_ASN1, len > 0
     * @retval 
     */
    int add_root_cert(const char* cert_buff, int type = 1, int len = 0);

    /**
     * @brief  
     * @note   
     * @param  other_buffer: 
     * @param  type: 
     * @param  len: 
     * @retval 
     */
    int verify_leaf_cert(const char* other_buffer, int type = 1, int len =0);

    /**
     * @brief  
     * @note   
     * @retval 
     */
    uint64_t get_cert_expiry_time() const { return m_expiry_timer; }

private:
    int   verify_cert(void* leaf_cert);
    void* load_cert_from_buffer(const char* cert_buff, int type, int len =0);
    void  set_cert_expiry_time(void* leaf_cert);

private:
    void* m_ca_store_ctx { NULL };
    void* m_root_certs_stack { NULL };
    void* m_ca_store { NULL };
    uint64_t m_expiry_timer { 0 };
};

NS_END2
