#include "xca_auth/xca_auth.h"
#include "xbase/xbase.h"
#include <functional>
#include <openssl/pem.h>
#include <string.h>

NS_BEG2(top, ca_auth)

xtop_ca_auth::xtop_ca_auth(std::string root_path, std::string self_path)
    : m_root_file(root_path)
    , m_self_file(self_path)
{
    OpenSSL_add_all_algorithms();
}

xtop_ca_auth::~xtop_ca_auth()
{
    if (m_other_cert) {
        X509_free(m_other_cert);
    }

    if (m_self_cert) {
        X509_free(m_self_cert);
    }

    if (m_root_cert) {
        X509_free(m_root_cert);
    }

    if (m_ca_store_ctx) {
        X509_STORE_CTX_cleanup(m_ca_store_ctx);
        X509_STORE_CTX_free(m_ca_store_ctx);
    }

    if (m_ca_store) {
        X509_STORE_free(m_ca_store);
    }
}

bool xtop_ca_auth::pem_to_der(unsigned char* out, long* out_len, const char* pem)
{
    std::unique_ptr<BIO> bio(BIO_new_mem_buf(pem, strlen(pem)));
    if (!bio) {
        return false;
    }
    char *name, *header;
    if (!PEM_read_bio(bio.get(), &name, &header, &out, out_len)) {
        xwarn("[xtop_ca_auth][pem_to_der] failed to read PEM data.");
        return false;
    }
    OPENSSL_free(name);
    OPENSSL_free(header);
    return true;
}

X509* xtop_ca_auth::load_cert_from_file(std::string cert_file)
{
    BIO* pbio = NULL;

    pbio = BIO_new_file(cert_file.c_str(), "r");
    X509* pCert = PEM_read_bio_X509(pbio, NULL, NULL, NULL);
    if (pCert == NULL) {
        X509_free(pCert);
        xwarn("fopen %s fail\n", cert_file.c_str());
        return NULL;
    }
    BIO_free(pbio);
    pbio = NULL;
    return pCert;
}

int xtop_ca_auth::init_root_cert()
{
    int ret = 1;

    if (NULL == m_root_cert) {
        if (!m_root_file.empty()) {
            m_root_cert = load_cert_from_file(m_root_file);
        } else {
            //todo, read from genesis
        }
        if (NULL == m_root_cert) {
            xwarn("[xtop_ca_auth][int_root_cert] init root cert failed.");
            ret = -1;
        } else {
            xinfo("[xtop_ca_auth][int_root_cert] init  root cert success.");
        }
    }

    return ret;
}

int xtop_ca_auth::init_self_cert()
{
    int ret = 1;
    if (NULL == m_self_cert) {

        if (!m_self_file.empty()) {
            m_self_cert = load_cert_from_file(m_self_file);
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

int xtop_ca_auth::verify_self_cert()
{
    int ret = 0;
    if (NULL == m_ca_store) {
        m_ca_store = X509_STORE_new();
    }

    if (NULL == m_ca_store || NULL == m_self_cert) {
        xwarn("[xtop_ca_auth][cert_verify_self] ca_store or self_cert is null.");
        return -1;
    }

    if (!X509_STORE_add_cert(m_ca_store, m_root_cert)) {
        xwarn("[xtop_ca_auth][int_root_cert] init ca store failed.");
        ret = -2;
    }

    if (NULL == m_ca_store_ctx) {
        m_ca_store_ctx = X509_STORE_CTX_new();
    }
    // no need to free
    STACK_OF(X509)* ca_stack = NULL;

    ret = X509_STORE_CTX_init(m_ca_store_ctx, m_ca_store, m_self_cert, ca_stack);
    if (ret != 1) {
        xwarn("[xtop_ca_auth][cert_verify_self] init failed: ret=%d.", ret);
        return ret;
    }

    ret = X509_verify_cert(m_ca_store_ctx);
    if (ret != 1) {
        xwarn("[xtop_ca_auth][cert_verify_self] verify failed: ret=%d. error:%s", ret, X509_verify_cert_error_string(m_ca_store_ctx->error));
    }
    return ret;
}

NS_END2