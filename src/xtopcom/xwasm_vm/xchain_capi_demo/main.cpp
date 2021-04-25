#include <stdio.h>
#include <stdint.h>

extern "C"
{
    /**
     * @brief call erc20 function
     * 
     * @param ptr params ptr
     * @return int32_t cost gas
     */
    int32_t c_call(int32_t ptr)
    {
        printf("test c_call %d\n", ptr);
        return 40;
    }

    /**
     * @brief depoly erc20 contract
     * 
     * @param ptr params ptr
     * @return int32_t cost gas
     */
    int32_t c_depoly(int32_t ptr)
    {
        printf("test c_depoly %d\n", ptr);
        return 30;
    }
}