#include "xca_auth/xca_auth.h"
#include "xbase/xbase.h"
#include <functional>
#include <openssl/pem.h>
#include <string.h>

NS_BEG2(top, ca_auth)

xtop_ca_auth::xtop_ca_auth(std::string self_path): m_self_file(self_path)
{
    OpenSSL_add_all_algorithms();
    m_root_certs_stack = sk_X509_new_null();
}

xtop_ca_auth::xtop_ca_auth()
{
    OpenSSL_add_all_algorithms();
    m_root_certs_stack = sk_X509_new_null();
}

xtop_ca_auth::~xtop_ca_auth()
{
    if (m_self_cert) {
        X509_free(m_self_cert);
    }

    if (m_root_certs_stack) {
        sk_X509_free(m_root_certs_stack);
    }

    if (m_ca_store_ctx) {
        X509_STORE_CTX_cleanup(m_ca_store_ctx);
        X509_STORE_CTX_free(m_ca_store_ctx);
    }
        if (m_ca_store) {
        X509_STORE_free(m_ca_store);
    }
}

X509* xtop_ca_auth::load_cert_from_file(std::string cert_file, int type)
{
    BIO* pbio = NULL;
    X509* pCert = NULL;
    pbio = BIO_new_file(cert_file.c_str(), "r");
    xdbg("fopen %s type %d ", cert_file.c_str(), type);
    if (type == X509_FILETYPE_PEM) {
        pCert = PEM_read_bio_X509(pbio, NULL, NULL, NULL);
    } else if (type == X509_FILETYPE_ASN1) {
        pCert = d2i_X509_bio(pbio, NULL);
    } else {
        xwarn("fopen %s type %d is error.", cert_file.c_str(), type);
    }

    if (pCert == NULL) {
        X509_free(pCert);
        xwarn("fopen %s fail\n", cert_file.c_str());
    }
    BIO_free(pbio);
    pbio = NULL;
    return pCert;
}

X509* xtop_ca_auth::load_cert_from_buffer(const char* cert_buff, int type)
{
    if (cert_buff == NULL) {
        xwarn("add_root_cert  type %d error: cert is null", cert_buff, type);
        return NULL;
    }

    BIO* pbio = BIO_new_mem_buf(cert_buff, strlen(cert_buff));
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

int xtop_ca_auth::add_root_cert(const char* cert_buff, int type)
{
    X509* pCert = load_cert_from_buffer(cert_buff, type);
    if (pCert == NULL || !sk_X509_push(m_root_certs_stack, pCert)) {
        X509_free(pCert);
        xwarn("create pCert fail, pCert is %p , root_certs is %p.", pCert, m_root_certs_stack);
        return -1;
    }
    return 1;
}

int xtop_ca_auth::init_self_cert(const char* cert_buff, int type)
{
    if (NULL == m_self_cert) {
        m_self_cert = load_cert_from_buffer(cert_buff, type);
        if (NULL == m_self_cert) {
            X509_free(m_self_cert);
            m_self_cert = NULL;
            xwarn("create pCert fail, pCert is %p .", m_self_cert);
            return -1;
        }
    }
    return 1;
}

int xtop_ca_auth::init_self_cert(int type)
{
    int ret = 1;
    if (NULL == m_self_cert) {

        if (!m_self_file.empty()) {
            m_self_cert = load_cert_from_file(m_self_file, type);
        } else {
            // read from genesis
        }

        if (NULL == m_self_cert) {
            xwarn("[xtop_ca_auth][init_self_cert] init self cert failed.");
            ret = -1;
        } else {
            xinfo("[xtop_ca_auth][int_root_cert] init self success.");
        }
    }

    return ret;
}

int xtop_ca_auth::verify_cert(X509* leaf_cert)
{
    int ret = 0;
    if (m_root_certs_stack == NULL || leaf_cert == NULL) {
        xwarn("[xtop_ca_auth][init_self_cert] root_certs is %p , leaf_cert is %p.", m_root_certs_stack, leaf_cert);
        return -1;
    }

    if (NULL != m_ca_store_ctx) {
        X509_STORE_CTX_cleanup(m_ca_store_ctx);
        X509_STORE_CTX_free(m_ca_store_ctx);
         X509_STORE_free(m_ca_store);
    }

    m_ca_store_ctx = X509_STORE_CTX_new();
    m_ca_store = X509_STORE_new();
    X509_STORE_set_flags(m_ca_store, X509_V_FLAG_CRL_CHECK_ALL);
    ret = X509_STORE_CTX_init(m_ca_store_ctx, m_ca_store, leaf_cert, NULL);

    if (!ret) {
        xwarn("[xtop_ca_auth][init_self_cert] root_certs is %p , leaf_cert is %p.", m_root_certs_stack, leaf_cert);
        return ret;
    }

    X509_STORE_CTX_trusted_stack(m_ca_store_ctx, m_root_certs_stack);
  //  X509_STORE_CTX_set0_trusted_stack(m_ca_store_ctx, m_root_certs_stack);
    ret = X509_verify_cert(m_ca_store_ctx);
    if (ret != 1) {
        xwarn("[xtop_ca_auth][cert_verify_self] verify failed: ret=%d. error:%s", ret, X509_verify_cert_error_string(m_ca_store_ctx->error));
    }
    return ret;
}

int xtop_ca_auth::verify_self_cert()
{
    return verify_cert(m_self_cert);
}

int xtop_ca_auth::verify_other_cert(const char* other_buffer, int type)
{
    X509* other_cert = load_cert_from_buffer(other_buffer, type);
    return verify_cert(other_cert);
}

NS_END2