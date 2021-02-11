#include "xsha256.h"

using top::xmutisig::_hash256;

_hash256::_hash256() {
    reset();
}

void _hash256::update(const std::string &data) {
    assert(0 != data.size());

    SHA256_Update(&m_sha256_ctx, (const void *)(data.c_str()), data.size());
}

void _hash256::reset() {
    SHA256_Init(&m_sha256_ctx);
}

std::string _hash256::finish() {
    std::vector<uint8_t> output(32);
    SHA256_Final(output.data(), &m_sha256_ctx);
    m_output_hash256 = std::string((const char *)output.data(), output.size());
    return m_output_hash256;
}
