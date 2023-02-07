#include "xca_auth/xca_auth.h"
#include "xbase/xbase.h"
//#include <functional>
#include "openssl/pem.h"
#include "openssl/evp.h"
#include "openssl/x509.h"


#include <iostream>

NS_BEG2(top, ca_auth)
#define SECONDS_DAY (60 * 60 * 24)

xtop_ca_auth::xtop_ca_auth()
{
    OpenSSL_add_all_algorithms();
    m_root_certs_stack = static_cast<void*>(sk_X509_new_null());
}

xtop_ca_auth::~xtop_ca_auth()
{
    if (m_root_certs_stack) {
        sk_X509_free(static_cast<STACK_OF(X509)*>(m_root_certs_stack));
        m_root_certs_stack = NULL;
    }

    if (m_ca_store_ctx) {
        X509_STORE_CTX_cleanup(static_cast<X509_STORE_CTX*>(m_ca_store_ctx));
        X509_STORE_CTX_free(static_cast<X509_STORE_CTX*>(m_ca_store_ctx));
        m_ca_store_ctx = NULL;
    }
    if (m_ca_store) {
        X509_STORE_free(static_cast<X509_STORE*>(m_ca_store));
        m_ca_store = NULL;
    }
}


void* xtop_ca_auth::load_cert_from_buffer(const char* leaf_buff, int type, int len)
{
    if (leaf_buff == NULL) {
        xwarn("add_root_cert  type %d error: cert is null", leaf_buff, type);
        return NULL;
    }

    int real_len = strlen(leaf_buff);
    if (type != X509_FILETYPE_PEM) {
        real_len = len;
    }
    BIO* pbio = BIO_new_mem_buf(leaf_buff, real_len);
    X509* pCert = NULL;
    if (type == X509_FILETYPE_PEM) {
        pCert = PEM_read_bio_X509(pbio, NULL, NULL, NULL);
    } else if (type == X509_FILETYPE_ASN1) {
        pCert = d2i_X509_bio(pbio, NULL);
    } else {
        xwarn("cert type %d is error.", type);
    }

    BIO_free(pbio);
    pbio = NULL;
    return pCert;
}

int xtop_ca_auth::add_root_cert(const char* leaf_buff, int type, int len)
{
    void* pCert = load_cert_from_buffer(leaf_buff, type, len);
    if (pCert == NULL || !sk_X509_push(static_cast<STACK_OF(X509)*>(m_root_certs_stack), static_cast<X509*>(pCert))) {
        X509_free(static_cast<X509*>(pCert));
        xwarn("create pCert fail, pCert is %p , root_certs is %p.", pCert, m_root_certs_stack);
        return -1;
    }
    return 1;
}

void xtop_ca_auth::set_cert_expiry_time(void* leaf_cert)
{
    if (NULL == leaf_cert) {
        m_expiry_timer = 0;
        return;
    }

    time_t t;
    time(&t);
    auto cur_timer = ASN1_TIME_set(nullptr, t);
    int out_days = 0, out_seconds = 0;
    ASN1_TIME_diff(&out_days, &out_seconds, cur_timer, X509_get_notAfter(static_cast<X509*>(leaf_cert)));
    if (out_days <= 0 || out_seconds <= 0) {
        m_expiry_timer = 0;
        xwarn("[xtop_ca_auth][set_cert_expiry_time] m_expiry_timer is zero.");
    } else {
        m_expiry_timer = out_days * SECONDS_DAY + out_seconds;
        m_expiry_timer /= 10;
    }
    //std::cout << " out_days " << out_days << " out_seconds " << out_seconds << " m_expiry_timer " << m_expiry_timer << std::endl;
}

int xtop_ca_auth::verify_cert(void* leaf_cert)
{
    int ret = 0;
    m_expiry_timer = 0;
    if (m_root_certs_stack == NULL || leaf_cert == NULL) {
        xwarn("[xtop_ca_auth][init_self_cert] root_certs is %p , leaf_cert is %p.", m_root_certs_stack, leaf_cert);
        return -1;
    }
    X509* _leaf_cert = static_cast<X509*>(leaf_cert);
    X509_STORE_CTX* _ca_store_ctx = static_cast<X509_STORE_CTX*>(m_ca_store_ctx);
    X509_STORE* _ca_store = static_cast<X509_STORE*>(m_ca_store);

    if (NULL != _ca_store_ctx) {
        X509_STORE_CTX_cleanup(_ca_store_ctx);
        X509_STORE_CTX_free(_ca_store_ctx);
        X509_STORE_free(_ca_store);
    }

    _ca_store_ctx = X509_STORE_CTX_new();
    _ca_store = X509_STORE_new();
    X509_STORE_set_flags(_ca_store, X509_V_FLAG_CRL_CHECK_ALL);
    ret = X509_STORE_CTX_init(_ca_store_ctx, _ca_store, _leaf_cert, NULL);

    if (!ret) {
        xwarn("[xtop_ca_auth][init_self_cert] root_certs is %p , leaf_cert is %p.", m_root_certs_stack, _leaf_cert);
        return ret;
    }

    X509_STORE_CTX_trusted_stack(_ca_store_ctx, static_cast<STACK_OF(X509)*>(m_root_certs_stack));
    ret = X509_verify_cert(_ca_store_ctx);
    if (ret != 1) {
        xwarn("[xtop_ca_auth][cert_verify_self] verify failed: ret=%d. error:%s", ret, X509_verify_cert_error_string(_ca_store_ctx->error));
    } else {
        set_cert_expiry_time(leaf_cert);
    }

    return ret;
}

int xtop_ca_auth::verify_leaf_cert(const char* leaf_buffer, int type, int len)
{
    void* leaf_cert = load_cert_from_buffer(leaf_buffer, type, len);
    int ret = verify_cert(leaf_cert);
    if (leaf_cert) {
        X509_free(static_cast<X509*>(leaf_cert));
    }
    return ret;
}

NS_END2