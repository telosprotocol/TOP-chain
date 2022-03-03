#include "xbasic/xuint.hpp"

int32_t operator<<(top::base::xbuffer_t & stream, const uint128_t & value) {
    stream << value.lower();
    stream << value.upper();
    return sizeof(uint128_t);
}

int32_t operator>>(top::base::xbuffer_t & stream, uint128_t & value)  // read out data
{
    uint64_t lower = 0;
    uint64_t upper = 0;
    stream >> lower;
    stream >> upper;
    value = uint128_t(upper, lower);

    return sizeof(uint128_t);
}
