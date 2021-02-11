#pragma once

#include "xmutisig/xcommon/xcommon.h"

NS_BEG2(top, xmutisig)

class xecpointserialize
{
public:

    /*
    * deserialize to ec_point_ptr_t from src
    * success: not nullptr, failed nullptr
    */

    static EC_POINT* deserialize(const std::string &src);

    /*
    * serialize ec_point_ptr_t to str
    * 0: success, other failed
    */
    static uint32_t serialize(std::string &bn_str, EC_POINT* ec_point);

};

NS_END2
