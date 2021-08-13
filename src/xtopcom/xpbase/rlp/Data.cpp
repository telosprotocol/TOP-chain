#include "Data.h"

Data subData(const Data& indata, size_t index, size_t length) {
    size_t subLength = length;
    if (index + subLength > indata.size()) { subLength = indata.size() - index; } 
    return data(indata.data() + index, subLength);
}


