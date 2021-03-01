#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include "xcrypto/xckey.h"
#include "xbase/xutl.h"
using namespace std;

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cout << "make_keys num" << std::endl;
        return 0;
    }

    uint32_t num = atoi(argv[1]);

    //top::utl::xecpubkey_t pubkey = prikey.get_public_key();
    for (uint32_t i = 0; i < num; i++) {
        top::utl::xecprikey_t prikey;

        std::string pubkey = prikey.get_compress_public_key();

        std::string pri64 = top::base::xstring_utl::base64_encode((unsigned char const*)prikey.data(), (unsigned int)prikey.size());
        std::string pub64 = top::base::xstring_utl::base64_encode((unsigned char const*)pubkey.c_str(), (unsigned int)pubkey.size());
        std::string client_pub64 = top::base::xstring_utl::base64_encode((unsigned char const*)prikey.get_public_key().data(), (unsigned int)prikey.get_public_key().size());

        std::string pubhex = top::base::xstring_utl::to_hex(pubkey);
        std::cout << "prikey63:" << pri64 << std::endl;
        std::cout << "pubkey33:" << pub64 << std::endl;
        std::cout << "pubkey65:" << client_pub64 << std::endl;
        //std::cout << "pubkeyhex:" << pubhex << std::endl;
        std::cout << "==================" << std::endl;

        std::string initpri = std::string((char *)prikey.data(), prikey.size());
        std::string decodepri = top::base::xstring_utl::base64_decode(pri64);
        assert(initpri == decodepri);
    }


    return 0;
}

