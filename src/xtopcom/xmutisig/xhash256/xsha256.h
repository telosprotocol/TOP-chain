#pragma once

#include <string>
#include "xmutisig/xcommon/xcommon.h"

NS_BEG2(top, xmutisig)

class _hash256
{
public:
    _hash256();

public:

    /*
    * update data to sha256
    */

    void update(const std::string &data);

    /*
    * reset hash256 context
    */

    void reset();

    /*
    * generate hash256
    * and return hash256_str
    */

    std::string finish();

private:
    std::string m_output_hash256;
    SHA256_CTX  m_sha256_ctx;
};

NS_END2
