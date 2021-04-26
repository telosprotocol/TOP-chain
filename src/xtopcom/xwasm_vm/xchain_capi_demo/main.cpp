#include <stdio.h>
#include <stdint.h>
#include <string>

struct params
{
    std::string account_from;
    std::string account_to;
    int value;
    params(std::string _f, std::string _t, int _v) : account_from{_f}, account_to{_t}, value{_v} {}
};

extern "C"
{
    /**
     * @brief call erc20 function
     * 
     * @param ptr params ptr
     * @return int32_t cost gas
     */
    int32_t c_call(params *ptr)
    {
        printf("[debug]test c_call %p\n", ptr);
        printf("[debug]%s \n", ptr->account_from.c_str());
        printf("[debug]%s \n", ptr->account_to.c_str());
        printf("[debug]%d \n", ptr->value);
        return 40;
    }

    /**
     * @brief depoly erc20 contract
     * 
     * @param ptr params ptr
     * @return int32_t cost gas
     */
    int32_t c_depoly(params *ptr)
    {
        printf("[debug]test c_depoly %p\n", ptr);
        printf("[debug]%s \n", ptr->account_from.c_str());
        printf("[debug]%s \n", ptr->account_to.c_str());
        printf("[debug]%d \n", ptr->value);
        return 30;
    }
}