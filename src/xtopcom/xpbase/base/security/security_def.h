#pragma once

#include "xkad/third_party/cryptopp/include/xcryptopp/osrng.h"
#include "xkad/third_party/cryptopp/include/xcryptopp/aes.h"
#include "xkad/third_party/cryptopp/include/xcryptopp/modes.h"

typedef CryptoPP::SecByteBlock AESKey;

namespace top {

const char* AES_IV = "123456";

}