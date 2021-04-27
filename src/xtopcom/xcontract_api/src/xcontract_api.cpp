#include "xcontract_common/xcontract_api_params.h"


extern "C" {
int32_t c_call(params * ptr) {
    printf("[debug]test c_call %p\n", ptr);
    printf("[debug]%s \n", ptr->account_from.c_str());
    printf("[debug]%s \n", ptr->account_to.c_str());
    printf("[debug]%d \n", ptr->value);
    return 40;
}

int32_t c_depoly(params * ptr) {
    printf("[debug]test c_depoly %p\n", ptr);
    printf("[debug]%s \n", ptr->account_from.c_str());
    printf("[debug]%s \n", ptr->account_to.c_str());
    printf("[debug]%d \n", ptr->value);
    return 30;
}
}